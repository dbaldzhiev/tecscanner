#include "save_laz.h"
#include "csv_writer.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <limits>
#include <laszip_api.h>

LazStats saveLaz(const std::string& output,
                 const std::vector<Point>& points,
                 double capture_duration,
                 CsvWriter* csv_writer)
{
    LazStats stats{};
    stats.point_count = points.size();
    stats.capture_duration = capture_duration;

    double min_x = std::numeric_limits<double>::max();
    double max_x = std::numeric_limits<double>::lowest();
    double min_y = std::numeric_limits<double>::max();
    double max_y = std::numeric_limits<double>::lowest();
    double min_z = std::numeric_limits<double>::max();
    double max_z = std::numeric_limits<double>::lowest();
    for(const auto& p : points)
    {
        min_x = std::min(min_x, p.x);
        max_x = std::max(max_x, p.x);
        min_y = std::min(min_y, p.y);
        max_y = std::max(max_y, p.y);
        min_z = std::min(min_z, p.z);
        max_z = std::max(max_z, p.z);
    }

    std::size_t step = points.size() / 2000000;
    if(step == 0)
    {
        step = 1;
    }
    stats.decimation_step = step;

    laszip_POINTER writer = nullptr;
    if(laszip_create(&writer))
    {
        char* msg = nullptr;
        laszip_get_error(writer, &msg);
        std::cerr << "Failed to create laszip writer: " << (msg ? msg : "unknown error")
                  << std::endl;
        if(writer)
        {
            laszip_destroy(writer);
        }
        return stats;
    }

    struct WriterGuard
    {
        laszip_POINTER ptr;
        bool opened;
        explicit WriterGuard(laszip_POINTER p) : ptr(p), opened(false) {}
        ~WriterGuard()
        {
            if(ptr)
            {
                if(opened)
                {
                    laszip_close_writer(ptr);
                }
                laszip_destroy(ptr);
            }
        }
    } guard(writer);

    laszip_header* header = nullptr;
    laszip_get_header_pointer(writer, &header);
    header->file_source_ID = 4711;
    header->version_major = 1;
    header->version_minor = 2;
    header->point_data_format = 1;
    header->point_data_record_length = 28;
    header->x_scale_factor = 0.0001;
    header->y_scale_factor = 0.0001;
    header->z_scale_factor = 0.0001;
    header->min_x = min_x;
    header->max_x = max_x;
    header->min_y = min_y;
    header->max_y = max_y;
    header->min_z = min_z;
    header->max_z = max_z;
    header->number_of_point_records =
        static_cast<laszip_U32>((points.size() + step - 1) / step);

    if(laszip_open_writer(writer, output.c_str(), 1))
    {
        std::cerr << "Failed to open LAZ writer" << std::endl;
        return stats;
    }
    guard.opened = true;

    laszip_point* laz_point = nullptr;
    laszip_get_point_pointer(writer, &laz_point);

    auto write_start = std::chrono::steady_clock::now();
    laszip_F64 coords[3];
    for(std::size_t i = 0; i < points.size(); i += step)
    {
        const auto& p = points[i];
        laz_point->intensity = p.intensity;
        laz_point->gps_time = p.gps_time;
        laz_point->user_data = 0;
        laz_point->classification = p.tag;
        laz_point->point_source_ID = 0;
        coords[0] = p.x;
        coords[1] = p.y;
        coords[2] = p.z;
        laszip_set_coordinates(writer, coords);
        laszip_write_point(writer);
        if(csv_writer)
        {
            writePoint(*csv_writer, p);
        }
    }
    auto write_end = std::chrono::steady_clock::now();

    stats.write_duration =
        std::chrono::duration<double>(write_end - write_start).count();

    try
    {
        stats.file_size = std::filesystem::file_size(output);
    }
    catch(const std::filesystem::filesystem_error&)
    {
        stats.file_size = 0;
    }

    return stats;
}
