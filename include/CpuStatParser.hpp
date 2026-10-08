#pragma once

#include <cstdint>
#include <cstddef>
#include <string_view>

struct CpuTicks {
    uint64_t idle{0};
    uint64_t total{0};
};

struct CoreLoad {
    size_t core_id{0};
    double load_percentage{0.0};
};

class CpuStatParser {
public:
    static constexpr size_t BUFFER_SIZE = 16384; 
    static constexpr size_t MAX_CORES = 256;

    static size_t parseBuffer(std::string_view content, CpuTicks* out_ticks, size_t max_cores) noexcept;
    static double calculateLoad(const CpuTicks& prev, const CpuTicks& curr) noexcept;
};