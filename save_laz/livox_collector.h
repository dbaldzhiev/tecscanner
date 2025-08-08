#pragma once

#include "save_laz.h"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

class LivoxCollector
{
public:
    bool collect(std::vector<Point>& points,
                 std::vector<ImuData>& imus,
                 double& duration,
                 const std::string& cfg);
    static bool check(const std::string& cfg);

    const std::vector<std::pair<std::uint32_t, std::string>>& serials() const
    {
        return serials_;
    }

private:
    std::vector<std::pair<std::uint32_t, std::string>> serials_;
};
