#pragma once

#include <string>
#include <vector>

// Forward declare resource handle
struct RenderResourceHandle {
    std::string name;
};

// Base class for render passes
class RenderPass {
public:
    virtual ~RenderPass() {}

    // Called once before Execute for per-frame setup
    virtual void Setup() {}

    // Perform the pass
    virtual void Execute() {}

    // Declare resource dependencies (reads)
    virtual std::vector<RenderResourceHandle> Reads() const { return {}; }

    // Declare resource outputs/writes
    virtual std::vector<RenderResourceHandle> Writes() const { return {}; }

    // Friendly name
    virtual std::string Name() const { return "UnnamedPass"; }
};
