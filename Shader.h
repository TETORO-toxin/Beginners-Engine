#pragma once
#include <string>

// Lightweight shader abstraction ? currently a no-op wrapper around DxLib/placeholder.
class Shader {
public:
    Shader() {}
    ~Shader() {}

    // Load shader from file (vertex/pixel). Returns true on success (or when no real shader support available).
    bool Load(const std::string& vertexPath, const std::string& pixelPath);

    // Bind shader for rendering (no-op if unsupported)
    void Bind();

    // Set a float3 uniform (name-based; no-op fallback)
    void SetFloat3(const std::string& name, float x, float y, float z);

    // Set a float uniform
    void SetFloat(const std::string& name, float v);

    // Set a color/int uniform
    void SetInt(const std::string& name, int v);
};
