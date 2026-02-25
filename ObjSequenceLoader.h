#pragma once
#include <string>
#include <vector>
#include <memory>
#include "DxLib.h"

struct ObjSequence {
    // frames of vertex positions (each frame must have same vertex count)
    std::vector<std::vector<VECTOR>> frames;
    std::vector<int> indices; // shared indices
    int FrameCount() const { return (int)frames.size(); }
    size_t VertexCount() const { return frames.empty() ? 0 : frames[0].size(); }
};

namespace ObjSequenceLoader {
    // Given path to one .obj in a numbered sequence (e.g. Assets/char_0000.obj),
    // find other files with same prefix and load sequence. Returns nullptr on failure.
    std::shared_ptr<ObjSequence> LoadSequence(const std::string& examplePath);
}
