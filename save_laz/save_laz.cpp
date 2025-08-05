#include <atomic>
#include <chrono>
#include <csignal>
#include <cstring>
#include <fstream>
#include <iostream>
#include <laszip/laszip_api.h>
#include <livox_lidar_api.h>
#include <livox_lidar_def.h>
#include <mutex>
#include <thread>
#include <vector>

// Simple utility that streams Livox MID360 data into a LAZ file.
// The program runs until it receives SIGINT (Ctrl+C) and mirrors the
// recording format used by the mandeye_controller project.

namespace
{
std::atomic_bool running(true);
std::atomic_bool frame_done(false);
laszip_POINTER writer = nullptr;
laszip_point* laz_point = nullptr;
std::mutex writer_mutex;
std::ofstream csv_writer;

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
	for(uint32_t i = 0; i < data->dot_num; ++i)
	{
		laz_point->intensity = pts[i].reflectivity;
		laz_point->gps_time = static_cast<double>(ts) * 1e-9;
		laz_point->user_data = 0;
		laz_point->classification = pts[i].tag;
		laz_point->point_source_ID = 0;
		coords[0] = 0.001 * pts[i].x;
		coords[1] = 0.001 * pts[i].y;
		coords[2] = 0.001 * pts[i].z;
		laszip_set_coordinates(writer, coords);
		laszip_write_point(writer);
		csv_writer << coords[0] << ',' << coords[1] << ',' << coords[2] << ',' << static_cast<int>(pts[i].reflectivity) << ','
				   << static_cast<double>(ts) * 1e-9 << ',' << 0 << ',' << static_cast<int>(pts[i].tag) << ',' << 0 << '\n';
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
		bool ok = LivoxLidarSdkInit(nullptr);
		if(ok)
			LivoxLidarSdkUninit();
		return ok ? 0 : 1;
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
		std::cerr << "Failed to create laszip writer" << std::endl;
		LivoxLidarSdkUninit();
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
}
