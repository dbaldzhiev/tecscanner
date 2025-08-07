#include "csv_writer.h"
#include "save_laz.h"  // for Point definition

#include <iostream>

bool openCsv(CsvWriter& writer, const std::string& filename)
{
    writer.file.open(filename);
    if(!writer.file)
    {
        std::cerr << "Failed to open CSV writer" << std::endl;
        return false;
    }
    writer.file << "x,y,z,intensity,gps_time,line_id,tag,laser_id\n";
    return true;
}

void writePoint(CsvWriter& writer, const Point& p)
{
    if(!writer.file)
    {
        return;
    }
    writer.file << p.x << ',' << p.y << ',' << p.z << ','
                << static_cast<int>(p.intensity) << ',' << p.gps_time << ','
                << static_cast<int>(p.line_id) << ',' << static_cast<int>(p.tag)
                << ',' << p.laser_id << "\n";
}

void closeCsv(CsvWriter& writer)
{
    if(writer.file)
    {
        writer.file.close();
    }
}

