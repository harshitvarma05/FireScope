#pragma once
#include <algorithm>
#include <cstdint>
namespace firescope {
inline uint32_t mixBits(uint32_t v) {
    v ^= v >> 16; v *= 0x7feb352du;
    v ^= v >> 15; v *= 0x846ca68bu;
    return v ^ (v >> 16);
}
inline uint32_t hashCell(uint32_t seed, uint32_t x, uint32_t y, uint32_t step) {
    return mixBits(seed ^ mixBits(x + 0x9e3779b9u) ^
                   mixBits(y + 0x85ebca6bu) ^ mixBits(step + 0xc2b2ae35u));
}
inline bool ignitionSucceeds(uint32_t random, float probability) {
    // uint64_t represents the exclusive threshold 2^32 when p == 1.
    const auto threshold = static_cast<uint64_t>(
        static_cast<double>(std::clamp(probability, 0.0f, 1.0f)) * 4294967296.0);
    return static_cast<uint64_t>(random) < threshold;
}
}
