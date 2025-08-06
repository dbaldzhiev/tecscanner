#include "save_laz.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <laszip_api.h>

LazStats saveLaz(const std::string& output,
                 const std::vector<Point>& points,
                 double capture_duration)
{
    LazStats stats{};
    stats.point_count = points.size();
    stats.capture_duration = capture_duration;

    laszip_POINTER writer = nullptr;
    if(laszip_create(&writer))
    {
        const char* msg = nullptr;
        laszip_get_error_message(writer, &msg);
        std::cerr << "Failed to create laszip writer: " << (msg ? msg : "unknown error") << std::endl;
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

    if(laszip_open_writer(writer, output.c_str(), 1))
    {
        std::cerr << "Failed to open LAZ writer" << std::endl;
        return stats;
    }
    guard.opened = true;

    laszip_point* laz_point = nullptr;
    laszip_get_point_pointer(writer, &laz_point);

    std::string csv_output = output;
    auto dot = csv_output.find_last_of('.');
    if(dot != std::string::npos)
    {
        csv_output.replace(dot, std::string::npos, ".csv");
    }
    else
    {
        csv_output += ".csv";
    }
    std::ofstream csv_writer(csv_output);
    if(!csv_writer)
    {
        std::cerr << "Failed to open CSV writer" << std::endl;
        return stats;
    }
    csv_writer << "x,y,z,intensity,gps_time,line_id,tag,laser_id\n";

    auto write_start = std::chrono::steady_clock::now();
    laszip_F64 coords[3];
    for(const auto& p : points)
    {
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
        csv_writer << coords[0] << ',' << coords[1] << ',' << coords[2] << ','
                   << static_cast<int>(p.intensity) << ',' << p.gps_time << ','
                   << 0 << ',' << static_cast<int>(p.tag) << ',' << 0 << '\n';
    }
    auto write_end = std::chrono::steady_clock::now();

    stats.write_duration =
        std::chrono::duration<double>(write_end - write_start).count();

    csv_writer.close();

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
