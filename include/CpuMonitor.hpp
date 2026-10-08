#pragma once

#include "FdWrapper.hpp"
#include "CpuStatParser.hpp"
#include <string_view>
#include <atomic>
#include <cstdint>

enum class AppState {
    Init,
    Run,
    Terminated
};

struct MonitorConfig {
    int interval_sec{0};
    const char* output_filepath{nullptr};
};

class CpuMonitor {
public:
    explicit CpuMonitor(const MonitorConfig& config) noexcept;
    ~CpuMonitor() noexcept = default;

    bool init() noexcept;

    void run() noexcept;

    static void requestStop() noexcept;

private:
    bool sampleCpuTicks(CpuTicks* target_ticks) noexcept;

    void printToStdout() noexcept;

    void writeToFile(uint64_t timestamp_sec) noexcept;

    static size_t formatCoreLoad(char* dst, size_t max_len, size_t core_id, double load) noexcept;

private:
    AppState state_{AppState::Init};
    MonitorConfig config_;

    FdWrapper proc_stat_fd_;
    FdWrapper output_file_fd_;

    size_t active_cores_{0};

    CpuTicks prev_ticks_[CpuStatParser::MAX_CORES]{};
    CpuTicks current_ticks_[CpuStatParser::MAX_CORES]{};
    char read_buffer_[CpuStatParser::BUFFER_SIZE]{};

    static inline std::atomic<bool> stop_requested_{false};
};