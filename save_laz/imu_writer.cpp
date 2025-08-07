#include "imu_writer.h"

#include <iostream>

bool openImuCsv(ImuWriter& writer, const std::string& filename)
{
    writer.file.open(filename);
    if(!writer.file)
    {
        std::cerr << "Failed to open IMU CSV writer" << std::endl;
        return false;
    }
    writer.file << "timestamp gyroX gyroY gyroZ accX accY accZ imuId timestampUnix\n";
    return true;
}

void writeImu(ImuWriter& writer, const ImuData& imu)
{
    if(!writer.file)
    {
        return;
    }
    writer.file << imu.timestamp << ' '
                << imu.gyro_x << ' ' << imu.gyro_y << ' ' << imu.gyro_z << ' '
                << imu.acc_x << ' ' << imu.acc_y << ' ' << imu.acc_z << ' '
                << imu.imu_id << ' ' << imu.timestamp_unix << '\n';
}

void closeImuCsv(ImuWriter& writer)
{
    if(writer.file)
    {
        writer.file.close();
    }
}

