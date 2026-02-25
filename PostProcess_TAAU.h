#pragma once

// TAAU stub: Temporal anti-aliasing + upscaling
class PostProcess_TAAU {
public:
    PostProcess_TAAU() {}
    ~PostProcess_TAAU() {}

    void Initialize(int width, int height) {}
    int Apply(int inputTexture) { return inputTexture; }
};
