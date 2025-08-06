#pragma once

#include "save_laz.h"

#include <string>
#include <vector>

class LivoxCollector
{
public:
    bool collect(std::vector<Point>& points,
                 double& duration,
                 const std::string& cfg);
    static bool check(const std::string& cfg);
};
