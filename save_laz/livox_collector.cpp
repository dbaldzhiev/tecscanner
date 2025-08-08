#include "livox_collector.h"

#include <atomic>
#include <chrono>
#include <cstring>
#include <livox_lidar_api.h>
#include <livox_lidar_def.h>
#include <thread>

bool LivoxCollector::collect(std::vector<Point>& points, std::vector<ImuData>& imus, double& duration, const std::string& cfg)
{
	serials_.clear();
	if(!LivoxLidarSdkInit(cfg.c_str()))
	{
		return false;
	}

	std::atomic_bool running(true);
	std::atomic_bool frame_done(false);

	struct CallbackCtx
	{
		std::vector<Point>* pts;
		std::vector<ImuData>* imus;
		std::atomic_bool* frame_done;
		std::atomic_bool* running;
	} ctx{&points, &imus, &frame_done, &running};

	auto point_cb = [](uint32_t, const uint8_t, LivoxLidarEthernetPacket* data, void* user) {
		if(!data || data->data_type != kLivoxLidarCartesianCoordinateHighData)
		{
			return;
		}
		auto* c = static_cast<CallbackCtx*>(user);
		auto* pts = reinterpret_cast<LivoxLidarCartesianHighRawPoint*>(data->data);
		uint64_t ts = 0;
		std::memcpy(&ts, data->timestamp, sizeof(ts));
		double gps_time = static_cast<double>(ts) * 1e-9;
		for(uint32_t i = 0; i < data->dot_num; ++i)
		{
			Point p;
			p.x = 0.001 * pts[i].x;
			p.y = 0.001 * pts[i].y;
			p.z = 0.001 * pts[i].z;
			p.intensity = pts[i].reflectivity;
			p.tag = pts[i].tag;
			p.line_id = 0;
			p.laser_id = 0;
			p.gps_time = gps_time * 1e3; // milliseconds
			c->pts->push_back(p);
		}
		*(c->frame_done) = true;
		*(c->running) = false;
	};

	auto imu_cb = [](uint32_t handle, const uint8_t, LivoxLidarEthernetPacket* data, void* user) {
		if(!data || data->data_type != kLivoxLidarImuData)
		{
			return;
		}
		auto* c = static_cast<CallbackCtx*>(user);
		auto* imu_raw = reinterpret_cast<LivoxLidarImuRawPoint*>(data->data);
		uint64_t ts = 0;
		std::memcpy(&ts, data->timestamp, sizeof(ts));
		ImuData imu;
		imu.timestamp = ts;
		imu.gyro_x = imu_raw->gyro_x;
		imu.gyro_y = imu_raw->gyro_y;
		imu.gyro_z = imu_raw->gyro_z;
		imu.acc_x = imu_raw->acc_x;
		imu.acc_y = imu_raw->acc_y;
		imu.acc_z = imu_raw->acc_z;
		imu.imu_id = static_cast<std::uint16_t>(handle);
		imu.timestamp_unix = static_cast<std::uint64_t>(
			std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
		c->imus->push_back(imu);
	};

	auto info_cb = [](const uint32_t handle, const LivoxLidarInfo* info, void* user) {
		if(info)
		{
			auto* vec = static_cast<std::vector<std::pair<std::uint32_t, std::string>>*>(user);
			vec->emplace_back(handle, info->sn);
			SetLivoxLidarWorkMode(handle, kLivoxLidarNormal, nullptr, nullptr);
			EnableLivoxLidarImuData(handle, nullptr, nullptr);
		}
	};

	SetLivoxLidarPointCloudCallBack(point_cb, &ctx);
	SetLivoxLidarImuDataCallback(imu_cb, &ctx);
	SetLivoxLidarInfoChangeCallback(info_cb, &serials_);
	LivoxLidarSdkStart();

	auto start = std::chrono::steady_clock::now();
	while(running && !frame_done)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		if(std::chrono::steady_clock::now() - start > std::chrono::seconds(5))
		{
			break;
		}
	}
	duration = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();

	LivoxLidarSdkUninit();
	return frame_done.load();
}

bool LivoxCollector::check(const std::string& cfg)
{
	if(!LivoxLidarSdkInit(cfg.c_str()))
	{
		return false;
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
	return lidar_found.load();
}
