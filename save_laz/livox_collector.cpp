#include "livox_collector.h"

#include <atomic>
#include <chrono>
#include <cstring>
#include <livox_lidar_api.h>
#include <livox_lidar_def.h>
#include <thread>

bool LivoxCollector::collect(std::vector<Point>& points,
                             double& duration,
                             const std::string& cfg)
{
    if(!LivoxLidarSdkInit(cfg.c_str()))
    {
        return false;
    }

    std::atomic_bool running(true);
    std::atomic_bool frame_done(false);

    struct CallbackCtx
    {
        std::vector<Point>* pts;
        std::atomic_bool* frame_done;
        std::atomic_bool* running;
    } ctx{&points, &frame_done, &running};

    auto point_cb = [](uint32_t, const uint8_t, LivoxLidarEthernetPacket* data, void* user)
    {
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
            p.gps_time = gps_time;
            c->pts->push_back(p);
        }
        *(c->frame_done) = true;
        *(c->running) = false;
    };

    auto info_cb = [](const uint32_t handle, const LivoxLidarInfo* info, void*)
    {
        if(info)
        {
            SetLivoxLidarWorkMode(handle, kLivoxLidarNormal, nullptr, nullptr);
        }
    };

    SetLivoxLidarPointCloudCallBack(point_cb, &ctx);
    SetLivoxLidarInfoChangeCallback(info_cb, nullptr);
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
    auto cb = [](const uint32_t, const LivoxLidarInfo* info, void* client_data)
    {
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
