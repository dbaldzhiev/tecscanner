#pragma once

#include "save_laz.h"

#include <cstdint>
#include <string>
#include <unordered_map>
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

    const std::vector<std::pair<std::uint16_t, std::string>>& serials() const
    {
        return serials_;
    }

private:
    std::vector<std::pair<std::uint16_t, std::string>> serials_;
    std::unordered_map<std::uint32_t, std::uint16_t> handle_to_id_;
};
