#pragma once
#include <cstdint>
#include <deque>
#include <memory>
#include <nlohmann/json.hpp>

namespace mandeye {

struct LivoxLidarCartesianHighRawPoint {
    int32_t x;
    int32_t y;
    int32_t z;
    uint8_t reflectivity;
    uint8_t tag;
};

struct LivoxPoint {
    LivoxLidarCartesianHighRawPoint point;
    uint64_t timestamp;
    uint8_t line_id;
    uint16_t laser_id;
};

using LivoxPointsBuffer = std::deque<LivoxPoint>;
using LivoxPointsBufferPtr = std::shared_ptr<LivoxPointsBuffer>;
using LivoxPointsBufferConstPtr = std::shared_ptr<const LivoxPointsBuffer>;

} // namespace mandeye
