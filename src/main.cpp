#include "CpuMonitor.hpp"
#include <csignal>
#include <cstdlib>
#include <unistd.h>
#include <cstring>

namespace {

void signalHandler(int /*signum*/) noexcept {
    CpuMonitor::requestStop();
}

void setupSignalHandlers() noexcept {
    struct sigaction sa{};
    sa.sa_handler = signalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    ::sigaction(SIGINT, &sa, nullptr);
    ::sigaction(SIGTERM, &sa, nullptr);
}

void printUsage(const char* prog_name) noexcept {
    const char msg1[] = "Usage: ";
    const char msg2[] = " [-i interval_sec] [-o output_file]\n"
                        "Options:\n"
                        "  -i <seconds>   Interval in seconds to print CPU load to file (Nice to have)\n"
                        "  -o <file>      Target file path for periodic CPU metrics logging\n"
                        "  -h             Show this help message\n";
    safeWrite(STDERR_FILENO, msg1, sizeof(msg1) - 1);
    if (prog_name) {
        safeWrite(STDERR_FILENO, prog_name, ::strlen(prog_name));
    }
    safeWrite(STDERR_FILENO, msg2, sizeof(msg2) - 1);
}

} // namespace

int main(int argc, char* argv[]) {
    setupSignalHandlers();

    MonitorConfig config{};
    int opt = 0;

    while ((opt = ::getopt(argc, argv, "i:o:h")) != -1) {
        switch (opt) {
            case 'i': {
                int val = std::atoi(optarg);
                if (val > 0) {
                    config.interval_sec = val;
                }
                break;
            }
            case 'o':
                config.output_filepath = optarg;
                break;
            case 'h':
            default:
                printUsage(argv[0]);
                return (opt == 'h') ? 0 : 1;
        }
    }

    if ((config.interval_sec > 0 && config.output_filepath == nullptr) ||
        (config.interval_sec == 0 && config.output_filepath != nullptr)) {
        const char warn[] = "Warning: Both -i <seconds> and -o <file> must be provided for file logging. Periodic logging disabled.\n";
        safeWrite(STDERR_FILENO, warn, sizeof(warn) - 1);
        config.interval_sec = 0;
        config.output_filepath = nullptr;
    }

    CpuMonitor monitor(config);

    if (!monitor.init()) {
        return 1;
    }

    monitor.run();

    return 0;
}