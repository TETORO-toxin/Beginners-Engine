#include "Shader.h"
#include <iostream>

bool Shader::Load(const std::string& vertexPath, const std::string& pixelPath) {
    // Placeholder: if shader files exist, pretend to load. In real engine this would compile and create GPU state.
    // For now we only log and succeed so higher-level code can call Set* methods safely.
    std::cout << "Shader::Load vertex=" << vertexPath << " pixel=" << pixelPath << "\n";
    return true;
}

void Shader::Bind() {
    // no-op
}

void Shader::SetFloat3(const std::string& name, float x, float y, float z) {
    // no-op fallback
}

void Shader::SetFloat(const std::string& name, float v) {
    // no-op fallback
}

void Shader::SetInt(const std::string& name, int v) {
    // no-op fallback
}
