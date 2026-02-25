#pragma once
#include <string>

struct RTFormat {
    std::string name;
};

struct RTDesc {
    int width = 0;
    int height = 0;
    std::string format; // e.g. "RGBA8_sRGB" or "RGBA16F"
    bool srgb = false;
    bool transient = true;
};

struct DSDesc {
    int width = 0;
    int height = 0;
    std::string format; // e.g. "D24S8"
    bool transient = true;
};

// Handle wrapper for API resource
struct RTHandle {
    int id = 0; // placeholder
};

struct DSHandle {
    int id = 0; // placeholder
};

// Lightweight lifetime wrapper
struct RenderResourceInstance {
    std::string name;
    RTDesc rtDesc;
    RTHandle handle;
};
