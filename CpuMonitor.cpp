#include "CpuMonitor.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <time.h>
#include <cstdio>
#include <cstring>

CpuMonitor::CpuMonitor(const MonitorConfig& config) noexcept 
    : config_(config) {}

void CpuMonitor::requestStop() noexcept {
    stop_requested_.store(true, std::memory_order_relaxed);
}

bool CpuMonitor::init() noexcept {
    state_ = AppState::Init;

    int stat_fd = ::open("/proc/stat", O_RDONLY);
    if (stat_fd < 0) {
        const char err[] = "Error: Failed to open /proc/stat\n";
        safeWrite(STDERR_FILENO, err, sizeof(err) - 1);
        return false;
    }
    proc_stat_fd_.reset(stat_fd);

    if (config_.output_filepath != nullptr && config_.interval_sec > 0) {
        int log_fd = ::open(config_.output_filepath, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (log_fd < 0) {
            const char err[] = "Error: Failed to open output file\n";
            safeWrite(STDERR_FILENO, err, sizeof(err) - 1);
            return false;
        }
        output_file_fd_.reset(log_fd);
    }

    if (!sampleCpuTicks(prev_ticks_)) {
        const char err[] = "Error: Failed to read initial CPU stats\n";
        safeWrite(STDERR_FILENO, err, sizeof(err) - 1);
        return false;
    }

    const char msg[] = "Init state completed successfully. Moving to Run state...\n"
                       "Press [ENTER] to print CPU core load to stdout. Press Ctrl+C to exit.\n\n";
    safeWrite(STDOUT_FILENO, msg, sizeof(msg) - 1);

    state_ = AppState::Run;
    return true;
}

bool CpuMonitor::sampleCpuTicks(CpuTicks* target_ticks) noexcept {
    if (!proc_stat_fd_.isValid()) {
        return false;
    }

    if (::lseek(proc_stat_fd_.get(), 0, SEEK_SET) == -1) {
        return false;
    }

    ssize_t bytes_read = ::read(proc_stat_fd_.get(), read_buffer_, sizeof(read_buffer_) - 1);
    if (bytes_read <= 0) {
        return false;
    }
    read_buffer_[bytes_read] = '\0';

    std::string_view content(read_buffer_, static_cast<size_t>(bytes_read));
    size_t cores = CpuStatParser::parseBuffer(content, target_ticks, CpuStatParser::MAX_CORES);

    if (cores > 0) {
        active_cores_ = cores;
        return true;
    }
    return false;
}

size_t CpuMonitor::formatCoreLoad(char* dst, size_t max_len, size_t core_id, double load) noexcept {
    int written = ::snprintf(dst, max_len, "Core %2zu: %6.2f%%\n", core_id, load);
    if (written < 0 || static_cast<size_t>(written) >= max_len) {
        return 0;
    }
    return static_cast<size_t>(written);
}

void CpuMonitor::printToStdout() noexcept {
    if (!sampleCpuTicks(current_ticks_)) {
        return;
    }

    const char header[] = "--- CPU Load by Request ---\n";
    safeWrite(STDOUT_FILENO, header, sizeof(header) - 1);

    char line_buf[64];
    for (size_t i = 0; i < active_cores_; ++i) {
        double load = CpuStatParser::calculateLoad(prev_ticks_[i], current_ticks_[i]);
        size_t len = formatCoreLoad(line_buf, sizeof(line_buf), i, load);
        if (len > 0) {
            safeWrite(STDOUT_FILENO, line_buf, len);
        }
        prev_ticks_[i] = current_ticks_[i];
    }

    const char separator[] = "---------------------------\n\n";
    safeWrite(STDOUT_FILENO, separator, sizeof(separator) - 1);
}

void CpuMonitor::writeToFile(uint64_t timestamp_sec) noexcept {
    if (!output_file_fd_.isValid()) {
        return;
    }

    if (!sampleCpuTicks(current_ticks_)) {
        return;
    }

    char log_buf[128];
    int header_len = ::snprintf(log_buf, sizeof(log_buf), "[Timestamp: %lu s]\n", timestamp_sec);
    if (header_len > 0) {
        safeWrite(output_file_fd_.get(), log_buf, static_cast<size_t>(header_len));
    }

    for (size_t i = 0; i < active_cores_; ++i) {
        double load = CpuStatParser::calculateLoad(prev_ticks_[i], current_ticks_[i]);
        size_t len = formatCoreLoad(log_buf, sizeof(log_buf), i, load);
        if (len > 0) {
            safeWrite(output_file_fd_.get(), log_buf, len);
        }
        prev_ticks_[i] = current_ticks_[i];
    }

    const char nl[] = "\n";
    safeWrite(output_file_fd_.get(), nl, sizeof(nl) - 1);
}

void CpuMonitor::run() noexcept {
    if (state_ != AppState::Run) {
        return;
    }

    struct pollfd fds[1];
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;

    struct timespec last_file_write{};
    ::clock_gettime(CLOCK_MONOTONIC, &last_file_write);

    while (!stop_requested_.load(std::memory_order_relaxed)) {
        int ret = ::poll(fds, 1, 250);

        if (ret > 0 && (fds[0].revents & POLLIN)) {
            char discard[64];
            ssize_t n = ::read(STDIN_FILENO, discard, sizeof(discard));
            (void)n;

            printToStdout();
        }

        if (output_file_fd_.isValid() && config_.interval_sec > 0) {
            struct timespec now{};
            ::clock_gettime(CLOCK_MONOTONIC, &now);

            int64_t elapsed_sec = now.tv_sec - last_file_write.tv_sec;
            if (elapsed_sec >= config_.interval_sec) {
                writeToFile(static_cast<uint64_t>(now.tv_sec));
                last_file_write = now;
            }
        }
    }

    state_ = AppState::Terminated;
    const char exit_msg[] = "\nApplication terminated cleanly.\n";
    safeWrite(STDOUT_FILENO, exit_msg, sizeof(exit_msg) - 1);
}