#pragma once

#include <fstream>
#include <string>

struct Point; // forward declaration

struct CsvWriter
{
    std::ofstream file;
};

bool openCsv(CsvWriter& writer, const std::string& filename);
void writePoint(CsvWriter& writer, const Point& p);
void closeCsv(CsvWriter& writer);

