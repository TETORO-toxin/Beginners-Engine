#pragma once
#include <string>
#include <memory>
#include "Mesh.h"

namespace ObjLoader {
    std::shared_ptr<Mesh> LoadObj(const std::string& path);
}
