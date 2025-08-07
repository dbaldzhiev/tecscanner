#pragma once

#include "save_laz.h"

#include <fstream>
#include <string>

struct ImuWriter
{
    std::ofstream file;
};

bool openImuCsv(ImuWriter& writer, const std::string& filename);
void writeImu(ImuWriter& writer, const ImuData& imu);
void closeImuCsv(ImuWriter& writer);

