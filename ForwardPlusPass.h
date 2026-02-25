#pragma once
#include "RenderPass.h"
#include "RenderResource.h"

// Forward+ pass skeleton
class ForwardPlusPass : public RenderPass {
public:
    ForwardPlusPass() {}
    ~ForwardPlusPass() override {}

    void Setup() override {}
    void Execute() override {
        // Placeholder: light culling (tiles/clusters) and forward rendering
        std::cout << "ForwardPlusPass: Execute (placeholder)\n";
    }

    std::string Name() const override { return "ForwardPlusPass"; }

    std::vector<RenderResourceHandle> Reads() const override {
        return { {"GBuffer_RT0"}, {"GBuffer_RT1"}, {"GBuffer_RT2"}, {"GBuffer_RT3"}, {"Depth"} };
    }

    std::vector<RenderResourceHandle> Writes() const override {
        return { {"LightListBuffer"} };
    }
};
