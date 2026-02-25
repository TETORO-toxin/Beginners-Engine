#pragma once
#include <vector>
#include <memory>
#include <unordered_map>
#include <string>
#include <iostream>
#include "RenderPass.h"
#include "RenderResourceManager.h"

// Minimal RenderGraph skeleton for pipeline planning
class RenderPass;

struct RenderResourceDesc {
    enum class Type { RenderTarget, DepthStencil, Buffer } type = Type::RenderTarget;
    int width = 0;
    int height = 0;
    std::string format; // descriptive
    bool transient = true; // can be reused across passes
};

struct RenderResource {
    RenderResourceDesc desc;
    int runtimeHandle = 0; // placeholder for API texture/RT handle
};

class RenderGraph {
public:
    RenderGraph() : resManager_(std::make_unique<RenderResourceManager>()) {}
    ~RenderGraph() {}

    void AddPass(std::shared_ptr<RenderPass> pass) {
        passes_.push_back(pass);
    }

    // Register a named transient resource
    void RegisterResource(const std::string& name, const RenderResourceDesc& desc) {
        resources_[name].desc = desc;
    }

    // Simple execution: enumerate passes, allocate resources on first write, free if transient after last use
    void Execute() {
        // compute last use for each resource
        std::unordered_map<std::string, int> lastUse;
        for (size_t i = 0; i < passes_.size(); ++i) {
            auto& p = passes_[i];
            for (auto r : p->Reads()) {
                int prev = -1;
                auto it = lastUse.find(r.name);
                if (it != lastUse.end()) prev = it->second;
                int cur = static_cast<int>(i);
                if (cur > prev) lastUse[r.name] = cur;
            }
            for (auto r : p->Writes()) {
                int prev = -1;
                auto it = lastUse.find(r.name);
                if (it != lastUse.end()) prev = it->second;
                int cur = static_cast<int>(i);
                if (cur > prev) lastUse[r.name] = cur;
            }
        }

        for (size_t i = 0; i < passes_.size(); ++i) {
            auto& p = passes_[i];
            if (!p) continue;

            std::cout << "Execute pass: " << p->Name() << "\n";

            // allocate resources that this pass writes and are not yet allocated
            for (auto r : p->Writes()) {
                auto it = resources_.find(r.name);
                if (it == resources_.end()) {
                    std::cerr << "Warning: resource not registered: " << r.name << "\n";
                    continue;
                }
                if (it->second.runtimeHandle == 0) {
                    // allocate via manager
                    RenderResourceDesc desc;
                    desc.width = it->second.desc.width;
                    desc.height = it->second.desc.height;
                    desc.format = it->second.desc.format;
                    desc.transient = it->second.desc.transient;
                    // translate to RTDesc
                    RTDesc rtd{ desc.width, desc.height, desc.format, desc.transient };
                    RTHandle h = resManager_->AllocateRT(it->first, rtd);
                    it->second.runtimeHandle = h.id;
                }
            }

            // Setup and execute
            p->Setup();
            p->Execute();

            // release resources whose last use is this pass and are transient
            for (auto r : p->Reads()) {
                auto it = resources_.find(r.name);
                if (it != resources_.end() && it->second.desc.transient) {
                    auto luIt = lastUse.find(r.name);
                    if (luIt != lastUse.end() && luIt->second == static_cast<int>(i)) {
                        RTDesc rtd{ it->second.desc.width, it->second.desc.height, it->second.desc.format, it->second.desc.transient };
                        resManager_->ReleaseRT(it->first, rtd);
                        it->second.runtimeHandle = 0;
                    }
                }
            }
            for (auto r : p->Writes()) {
                auto it = resources_.find(r.name);
                if (it != resources_.end() && it->second.desc.transient) {
                    auto luIt = lastUse.find(r.name);
                    if (luIt != lastUse.end() && luIt->second == static_cast<int>(i)) {
                        RTDesc rtd{ it->second.desc.width, it->second.desc.height, it->second.desc.format, it->second.desc.transient };
                        resManager_->ReleaseRT(it->first, rtd);
                        it->second.runtimeHandle = 0;
                    }
                }
            }
        }
    }

private:
    std::vector<std::shared_ptr<RenderPass>> passes_;
    std::unordered_map<std::string, RenderResource> resources_;
    std::unique_ptr<RenderResourceManager> resManager_;
};
