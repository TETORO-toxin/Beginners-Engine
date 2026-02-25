#pragma once
#include <string>
#include <memory>
#include "../Mesh.h"

namespace ModelLoader {
    // Load model from file (supports FBX/other via Assimp if available).
    // Returns nullptr on failure or if Assimp is not available.
    std::shared_ptr<Mesh> LoadModel(const std::string& path);
}
