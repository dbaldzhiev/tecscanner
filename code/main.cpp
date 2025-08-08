#include "save_laz.h"
#include <memory>
#include <string>
#include <iostream>

int main(int argc, char** argv) {
    if (argc >= 2 && std::string(argv[1]) == "--check") {
        // In this simplified version the presence of the executable implies
        // that the LiDAR software stack is available.
        return 0;
    }
    if (argc < 2) {
        std::cerr << "usage: " << argv[0] << " <output.laz>" << std::endl;
        return 1;
    }
    std::string filename = argv[1];
    mandeye::LivoxPointsBufferPtr buffer = std::make_shared<mandeye::LivoxPointsBuffer>();
    auto stats = mandeye::saveLaz(filename, buffer);
    if (!stats) {
        std::cerr << "Failed to save laz file" << std::endl;
        return 1;
    }
    return 0;
}
