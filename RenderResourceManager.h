#pragma once
#include "RenderResource.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <iostream>

// Simple pooled resource manager for transient RT/DS
class RenderResourceManager {
public:
    RenderResourceManager() {}
    ~RenderResourceManager() {
        // Ensure all resources are released
        for (auto &p : pool_) {
            for (auto &h : p.second) {
                std::cout << "Cleanup pooled handle " << h.id << "\n";
            }
        }
        for (auto &u : live_) {
            std::cout << "Release live resource " << u.first << " handle " << u.second.id << "\n";
        }
    }

    // Allocate a transient RT matching desc, reusing pooled ones when possible
    RTHandle AllocateRT(const std::string &name, const RTDesc &desc) {
        std::string key = Key(desc);
        auto pit = pool_.find(key);
        if (pit != pool_.end() && !pit->second.empty()) {
            RTHandle h = pit->second.back();
            pit->second.pop_back();
            live_[name] = h;
            std::cout << "Reuse RT " << name << " -> handle " << h.id << "\n";
            return h;
        }
        RTHandle h{ nextId_++ };
        live_[name] = h;
        std::cout << "Alloc RT " << name << " -> handle " << h.id << "\n";
        return h;
    }

    // Release a resource; if transient, return to pool
    void ReleaseRT(const std::string &name, const RTDesc &desc) {
        auto it = live_.find(name);
        if (it == live_.end()) return;
        RTHandle h = it->second;
        live_.erase(it);
        if (desc.transient) {
            pool_[Key(desc)].push_back(h);
            std::cout << "Pool RT " << name << " handle " << h.id << "\n";
        } else {
            std::cout << "Free RT " << name << " handle " << h.id << "\n";
        }
    }

private:
    std::string Key(const RTDesc &d) const {
        return std::to_string(d.width) + "x" + std::to_string(d.height) + "_" + d.format;
    }

    std::unordered_map<std::string, std::vector<RTHandle>> pool_;
    std::unordered_map<std::string, RTHandle> live_;
    int nextId_ = 1;
};
