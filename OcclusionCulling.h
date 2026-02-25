#pragma once

// OcclusionCulling stub: Hi-Z generation and test
class OcclusionCulling {
public:
    OcclusionCulling() {}
    ~OcclusionCulling() {}

    void Initialize() {}
    void BuildHiZ() {}
    bool IsVisible(int objectId) { return true; }
};
