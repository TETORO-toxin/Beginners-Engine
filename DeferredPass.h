#pragma once
#include "RenderPass.h"
#include "RenderResource.h"

// Deferred pass skeleton
class DeferredPass : public RenderPass {
public:
    DeferredPass() {}
    ~DeferredPass() override {}

    void Setup() override {}
    void Execute() override {
        // Placeholder: GBuffer creation, geometry pass, lighting pass
        std::cout << "DeferredPass: Execute (placeholder)\n";
    }

    std::string Name() const override { return "DeferredPass"; }

    std::vector<RenderResourceHandle> Writes() const override {
        return { {"GBuffer_RT0"}, {"GBuffer_RT1"}, {"GBuffer_RT2"}, {"GBuffer_RT3"}, {"Depth"} };
    }
};
