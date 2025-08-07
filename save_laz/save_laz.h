#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct Point
{
    double x;
    double y;
    double z;
    std::uint8_t intensity;
    std::uint8_t tag;
    double gps_time;
};

struct ImuData
{
    double timestamp;
    float gyro_x;
    float gyro_y;
    float gyro_z;
    float acc_x;
    float acc_y;
    float acc_z;
    std::uint32_t imu_id;
    std::uint64_t timestamp_unix;
};

struct LazStats
{
    std::size_t point_count;
    std::size_t decimation_step;
    double capture_duration;  // seconds
    double write_duration;    // seconds
    std::uint64_t file_size;  // bytes
};

struct CsvWriter; // forward declaration

LazStats saveLaz(const std::string& laz_file,
                 const std::vector<Point>& points,
                 double capture_duration,
                 CsvWriter* csv_writer = nullptr);
