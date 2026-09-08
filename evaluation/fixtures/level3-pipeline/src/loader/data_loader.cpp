#include "data_loader.h"
#include <iostream>

namespace loader {

std::string DataLoader::load() {
    std::cout << "loader: loading\n";
    return "raw-data";
}

} // namespace loader
