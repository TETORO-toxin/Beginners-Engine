#pragma once
#include "DxLib.h"

namespace Lighting {
    void SetMainDirectionalLight(float dirX, float dirY, float dirZ, int color, float intensity);
    VECTOR GetMainDirectionalDir(); // normalized direction (points where light is pointing)
    int GetMainDirectionalColor();
    float GetMainDirectionalIntensity();
}
