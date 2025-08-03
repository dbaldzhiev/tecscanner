#include <livox_lidar_api.h>
#include <laszip/laszip_api.h>
#include <csignal>
#include <atomic>
#include <mutex>
#include <thread>
#include <vector>
#include <cstring>
#include <iostream>

// Simple utility that streams Livox MID360 data into a LAZ file.
// The program runs until it receives SIGINT (Ctrl+C) and mirrors the
// recording format used by the mandeye_controller project.

namespace {
std::atomic_bool running(true);
laszip_POINTER writer = nullptr;
laszip_point* laz_point = nullptr;
std::mutex writer_mutex;

void signalHandler(int) {
    running = false;
}

void PointCloudCallback(uint32_t, const uint8_t dev_type,
                        LivoxLidarEthernetPacket* data, void*) {
    if (!data || data->data_type != kLivoxLidarCartesianCoordinateHighData) {
        return;
    }
    LivoxLidarCartesianHighRawPoint* pts =
        reinterpret_cast<LivoxLidarCartesianHighRawPoint*>(data->data);
    laszip_F64 coords[3];
    std::lock_guard<std::mutex> lk(writer_mutex);
    for (uint32_t i = 0; i < data->dot_num; ++i) {
        laz_point->intensity = pts[i].reflectivity;
        laz_point->gps_time = static_cast<double>(data->timestamp) * 1e-9;
        laz_point->user_data = pts[i].line_id;
        laz_point->classification = pts[i].tag;
        laz_point->point_source_id = pts[i].laser_id;
        coords[0] = 0.001 * pts[i].x;
        coords[1] = 0.001 * pts[i].y;
        coords[2] = 0.001 * pts[i].z;
        laszip_set_coordinates(writer, coords);
        laszip_write_point(writer);
    }
}
} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " output.laz" << std::endl;
        return 1;
    }
    if (std::strcmp(argv[1], "--check") == 0) {
        bool ok = LivoxLidarSdkInit(nullptr);
        if (ok) LivoxLidarSdkUninit();
        return ok ? 0 : 1;
    }
    std::string output = argv[1];
    std::string cfg = "mid360_config.json";
    if (const char* env = std::getenv("LIVOX_SDK_CONFIG")) {
        cfg = env;
    }
    if (!LivoxLidarSdkInit(cfg.c_str())) {
        std::cerr << "Livox Init Failed" << std::endl;
        return 1;
    }
    signal(SIGINT, signalHandler);
    SetLivoxLidarPointCloudCallBack(PointCloudCallback, nullptr);

    if (laszip_create(&writer)) {
        std::cerr << "Failed to create laszip writer" << std::endl;
        return 1;
    }
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
    if (laszip_open_writer(writer, output.c_str(), 1)) {
        std::cerr << "Failed to open LAZ writer" << std::endl;
        return 1;
    }
    laszip_get_point_pointer(writer, &laz_point);

    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    laszip_close_writer(writer);
    laszip_destroy(writer);
    LivoxLidarSdkUninit();
    return 0;
}
