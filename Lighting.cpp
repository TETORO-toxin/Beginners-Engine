#include "Lighting.h"
#include <cmath>

namespace {
    VECTOR mainDir = VGet(-0.5f, -1.0f, -0.25f);
    int mainColor = 0xFFFFFF;
    float mainIntensity = 1.0f;

    VECTOR Normalize(const VECTOR& v) {
        float len = sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
        if (len == 0) return VGet(0,0,0);
        return VGet(v.x/len, v.y/len, v.z/len);
    }
}

namespace Lighting {

void SetMainDirectionalLight(float dirX, float dirY, float dirZ, int color, float intensity) {
    mainDir = Normalize(VGet(dirX, dirY, dirZ));
    mainColor = color;
    mainIntensity = intensity;
}

VECTOR GetMainDirectionalDir() { return mainDir; }
int GetMainDirectionalColor() { return mainColor; }
float GetMainDirectionalIntensity() { return mainIntensity; }

}
