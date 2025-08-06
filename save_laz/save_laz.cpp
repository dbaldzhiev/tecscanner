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

<<<<<<< HEAD
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
        const char* msg = nullptr;
        laszip_get_error_message(writer, &msg);
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
=======
void signalHandler(int)
{
	running = false;
}

void PointCloudCallback(uint32_t, const uint8_t, LivoxLidarEthernetPacket* data, void*)
{
	if(!data || data->data_type != kLivoxLidarCartesianCoordinateHighData)
	{
		return;
	}
	LivoxLidarCartesianHighRawPoint* pts = reinterpret_cast<LivoxLidarCartesianHighRawPoint*>(data->data);
	laszip_F64 coords[3];
	std::lock_guard<std::mutex> lk(writer_mutex);

	uint64_t ts = 0;
	std::memcpy(&ts, data->timestamp, sizeof(ts));
	const double gps_time = static_cast<double>(ts) * 1e-9;
	for(uint32_t i = 0; i < data->dot_num; ++i)
	{
		laz_point->intensity = pts[i].reflectivity;
		laz_point->gps_time = gps_time;
		laz_point->user_data = 0;
		laz_point->classification = pts[i].tag;
		laz_point->point_source_ID = 0;
		coords[0] = 0.001 * pts[i].x;
		coords[1] = 0.001 * pts[i].y;
		coords[2] = 0.001 * pts[i].z;
		laszip_set_coordinates(writer, coords);
		laszip_write_point(writer);
		csv_writer << coords[0] << ',' << coords[1] << ',' << coords[2] << ',' << static_cast<int>(pts[i].reflectivity) << ',' << gps_time << ',' << 0
				   << ',' << static_cast<int>(pts[i].tag) << ',' << 0 << '\n';
	}

	frame_done = true;
	running = false;
}

void LidarInfoChangeCallback(const uint32_t handle, const LivoxLidarInfo* info, void*)
{
	if(info)
	{
		SetLivoxLidarWorkMode(handle, kLivoxLidarNormal, nullptr, nullptr);
	}
}
} // namespace

int main(int argc, char** argv)
{
	if(argc < 2)
	{
		std::cerr << "Usage: " << argv[0] << " output.laz" << std::endl;
		return 1;
	}
	if(std::strcmp(argv[1], "--check") == 0)
	{
		std::string cfg = "mid360_config.json";
		if(const char* env = std::getenv("LIVOX_SDK_CONFIG"))
		{
			cfg = env;
		}
		if(!LivoxLidarSdkInit(cfg.c_str()))
		{
			return 1;
		}
		std::atomic_bool lidar_found(false);
		auto cb = [](const uint32_t, const LivoxLidarInfo* info, void* client_data) {
			if(info)
			{
				*static_cast<std::atomic_bool*>(client_data) = true;
			}
		};
		SetLivoxLidarInfoChangeCallback(cb, &lidar_found);
		LivoxLidarSdkStart();
		for(int i = 0; i < 50 && !lidar_found; ++i)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
		LivoxLidarSdkUninit();
		return lidar_found ? 0 : 1;
	}
	std::string output = argv[1];
	std::string cfg = "mid360_config.json";
	if(const char* env = std::getenv("LIVOX_SDK_CONFIG"))
	{
		cfg = env;
	}
	if(!LivoxLidarSdkInit(cfg.c_str()))
	{
		std::cerr << "Livox Init Failed" << std::endl;
		return 1;
	}
	signal(SIGINT, signalHandler);
	SetLivoxLidarPointCloudCallBack(PointCloudCallback, nullptr);
	SetLivoxLidarInfoChangeCallback(LidarInfoChangeCallback, nullptr);
	LivoxLidarSdkStart();
	struct LaszipWriterGuard
	{
		laszip_POINTER ptr;
		bool opened;
		explicit LaszipWriterGuard(laszip_POINTER p)
			: ptr(p)
			, opened(false)
		{ }
		~LaszipWriterGuard()
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
	};

	laszip_POINTER tmp_writer = nullptr;
	if(laszip_create(&tmp_writer))
	{
		const char* msg = nullptr;
		laszip_get_error(tmp_writer, &msg);
		std::cerr << "Failed to create laszip writer: " << (msg ? msg : "unknown error") << std::endl;
		LivoxLidarSdkUninit();
		if(tmp_writer)
		{
			laszip_destroy(tmp_writer);
		}
		return 1;
	}
	writer = tmp_writer;
	LaszipWriterGuard writer_guard(writer);
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
		LivoxLidarSdkUninit();
		return 1;
	}
	writer_guard.opened = true;
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
	csv_writer.open(csv_output);
	if(!csv_writer)
	{
		std::cerr << "Failed to open CSV writer" << std::endl;
		LivoxLidarSdkUninit();
		return 1;
	}
	csv_writer << "x,y,z,intensity,gps_time,line_id,tag,laser_id\n";

	auto start_time = std::chrono::steady_clock::now();
	while(running && !frame_done)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		auto now = std::chrono::steady_clock::now();
		if(now - start_time > std::chrono::seconds(5))
		{
			break;
		}
	}

	LivoxLidarSdkUninit();
	csv_writer.close();
	return frame_done ? 0 : 1;
>>>>>>> 53485b8 (Fix laszip error message retrieval in save_laz.cpp)
}
