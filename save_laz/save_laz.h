#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

struct Point
{
    double x;
    double y;
    double z;
    std::uint8_t intensity;
    std::uint8_t tag;
    std::uint8_t line_id;
    std::uint16_t laser_id;
    double gps_time;
};

struct ImuData
{
    std::uint64_t timestamp;
    float gyro_x;
    float gyro_y;
    float gyro_z;
    float acc_x;
    float acc_y;
    float acc_z;
    std::uint16_t imu_id;
    std::uint64_t timestamp_unix;
};

struct LazStats
{
    std::string filename;
    std::size_t point_count;
    std::size_t decimation_step;
    double capture_duration;  // seconds
    double write_duration;    // seconds
    double file_size;         // MB

    nlohmann::json produceStatus() const;
};

LazStats saveLaz(const std::string& laz_file,
                 const std::vector<Point>& points,
                 double capture_duration);
