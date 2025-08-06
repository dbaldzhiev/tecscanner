#include "livox_collector.h"
#include "save_laz.h"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

int main(int argc, char** argv)
{
    if(argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " output.laz" << std::endl;
        return 1;
    }

    std::string cfg = "mid360_config.json";
    if(const char* env = std::getenv("LIVOX_SDK_CONFIG"))
    {
        cfg = env;
    }

    if(std::strcmp(argv[1], "--check") == 0)
    {
        return LivoxCollector::check(cfg) ? 0 : 1;
    }

    std::string output = argv[1];
    std::vector<Point> points;
    double capture_duration = 0.0;
    LivoxCollector collector;
    if(!collector.collect(points, capture_duration, cfg))
    {
        std::cerr << "Failed to collect points" << std::endl;
        return 1;
    }

    LazStats stats = saveLaz(output, points, capture_duration);
    return stats.point_count > 0 ? 0 : 1;
}
