#include "livox_collector.h"
#include "save_laz.h"
#include "imu_writer.h"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>
#include <sstream>
#include <iomanip>
#include <fstream>

int main(int argc, char** argv)
{
    if(argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " [--check] output.laz" << std::endl;
        return 1;
    }

    std::string cfg = "mid360_config.json";
    if(const char* env = std::getenv("LIVOX_SDK_CONFIG"))
    {
        cfg = env;
    }

    std::string output;
    for(int i = 1; i < argc; ++i)
    {
        if(std::strcmp(argv[i], "--check") == 0)
        {
            return LivoxCollector::check(cfg) ? 0 : 1;
        }
        else
        {
            output = argv[i];
        }
    }

    if(output.empty())
    {
        std::cerr << "Output filename required" << std::endl;
        return 1;
    }

    std::vector<Point> points;
    std::vector<ImuData> imus;
    double capture_duration = 0.0;
    LivoxCollector collector;
    if(!collector.collect(points, imus, capture_duration, cfg))
    {
        std::cerr << "Failed to collect points" << std::endl;
        return 1;
    }

    ImuWriter imu_writer;
    {
        std::size_t slash = output.find_last_of("/\\");
        std::size_t dot = output.find_last_of('.');
        std::size_t start = (slash == std::string::npos) ? 0 : slash + 1;
        std::string base = output.substr(start, (dot == std::string::npos) ? std::string::npos : dot - start);
        std::size_t pos = base.find_last_not_of("0123456789");
        unsigned idx = 0;
        if(pos != std::string::npos && pos + 1 < base.size())
        {
            idx = static_cast<unsigned>(std::stoul(base.substr(pos + 1)));
        }
        std::string dir = (slash == std::string::npos) ? std::string() : output.substr(0, slash + 1);
        std::ostringstream oss;
        oss << dir << "imu" << std::setw(4) << std::setfill('0') << idx << ".csv";
        if(openImuCsv(imu_writer, oss.str()))
        {
            for(const auto& imu : imus)
            {
                writeImu(imu_writer, imu);
            }
            closeImuCsv(imu_writer);
        }
    }

    LazStats stats = saveLaz(output, points, capture_duration);

    {
        std::size_t slash = output.find_last_of("/\\");
        std::size_t dot = output.find_last_of('.');
        std::size_t start = (slash == std::string::npos) ? 0 : slash + 1;
        std::string base =
            output.substr(start, (dot == std::string::npos) ? std::string::npos
                                                              : dot - start);
        std::size_t pos = base.find_last_not_of("0123456789");
        unsigned idx = 0;
        if(pos != std::string::npos && pos + 1 < base.size())
        {
            idx = static_cast<unsigned>(std::stoul(base.substr(pos + 1)));
        }
        std::string dir = (slash == std::string::npos) ? std::string()
                                                        : output.substr(0, slash + 1);
        std::ostringstream oss;
        oss << dir << "status" << std::setw(4) << std::setfill('0') << idx
            << ".json";
        std::ofstream sf(oss.str());
        if(sf)
        {
            sf << stats.produceStatus().dump(2);
        }
    }

    return stats.point_count > 0 ? 0 : 1;
}
