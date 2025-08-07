#include "livox_collector.h"
#include "save_laz.h"
#include "csv_writer.h"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

int main(int argc, char** argv)
{
    if(argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " [--csv] output.laz" << std::endl;
        return 1;
    }

    std::string cfg = "mid360_config.json";
    if(const char* env = std::getenv("LIVOX_SDK_CONFIG"))
    {
        cfg = env;
    }

    bool csv = false;
    std::string output;
    for(int i = 1; i < argc; ++i)
    {
        if(std::strcmp(argv[i], "--check") == 0)
        {
            return LivoxCollector::check(cfg) ? 0 : 1;
        }
        else if(std::strcmp(argv[i], "--csv") == 0)
        {
            csv = true;
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
    double capture_duration = 0.0;
    LivoxCollector collector;
    if(!collector.collect(points, capture_duration, cfg))
    {
        std::cerr << "Failed to collect points" << std::endl;
        return 1;
    }

    CsvWriter csv_writer;
    CsvWriter* csv_ptr = nullptr;
    std::string csv_output;
    if(csv)
    {
        csv_output = output;
        auto dot = csv_output.find_last_of('.');
        if(dot != std::string::npos)
        {
            csv_output.replace(dot, std::string::npos, ".csv");
        }
        else
        {
            csv_output += ".csv";
        }
        if(openCsv(csv_writer, csv_output))
        {
            csv_ptr = &csv_writer;
        }
        else
        {
            std::cerr << "CSV output disabled" << std::endl;
        }
    }

    LazStats stats = saveLaz(output, points, capture_duration, csv_ptr);
    if(csv_ptr)
    {
        closeCsv(csv_writer);
    }
    return stats.point_count > 0 ? 0 : 1;
}
