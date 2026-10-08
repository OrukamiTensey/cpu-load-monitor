#include "CpuStatParser.hpp"
#include <charconv>

size_t CpuStatParser::parseBuffer(std::string_view content, CpuTicks* out_ticks, size_t max_cores) noexcept {
    size_t cores_found = 0;
    size_t pos = 0;

    while (pos < content.size() && cores_found < max_cores) {
        size_t next_line = content.find('\n', pos);
        std::string_view line = (next_line == std::string_view::npos) 
                                ? content.substr(pos) 
                                : content.substr(pos, next_line - pos);

        pos = (next_line == std::string_view::npos) ? content.size() : next_line + 1;

        if (line.size() < 4 || line[0] != 'c' || line[1] != 'p' || line[2] != 'u' || line[3] < '0' || line[3] > '9') {
            continue;
        }

        size_t idx = 3;
        size_t core_id = 0;
        auto [ptr_id, ec_id] = std::from_chars(line.data() + idx, line.data() + line.size(), core_id);
        if (ec_id != std::errc() || core_id >= max_cores) {
            continue;
        }

        idx = ptr_id - line.data();

        uint64_t ticks[10] = {0};
        size_t count = 0;

        while (idx < line.size() && count < 10) {
            while (idx < line.size() && line[idx] == ' ') {
                ++idx;
            }
            if (idx >= line.size()) break;

            auto [ptr, ec] = std::from_chars(line.data() + idx, line.data() + line.size(), ticks[count]);
            if (ec == std::errc()) {
                ++count;
                idx = ptr - line.data();
            } else {
                break;
            }
        }

        if (count >= 4) {
            uint64_t idle_all = ticks[3] + (count > 4 ? ticks[4] : 0); // idle + iowait
            uint64_t total_all = 0;
            for (size_t i = 0; i < count; ++i) {
                total_all += ticks[i];
            }

            out_ticks[core_id].idle = idle_all;
            out_ticks[core_id].total = total_all;

            if (core_id >= cores_found) {
                cores_found = core_id + 1;
            }
        }
    }

    return cores_found;
}

double CpuStatParser::calculateLoad(const CpuTicks& prev, const CpuTicks& curr) noexcept {
    if (curr.total <= prev.total) {
        return 0.0;
    }

    uint64_t total_delta = curr.total - prev.total;
    uint64_t idle_delta = (curr.idle >= prev.idle) ? (curr.idle - prev.idle) : 0;

    if (idle_delta > total_delta) {
        idle_delta = total_delta;
    }

    double load = 100.0 * static_cast<double>(total_delta - idle_delta) / static_cast<double>(total_delta);
    return (load < 0.0) ? 0.0 : ((load > 100.0) ? 100.0 : load);
}