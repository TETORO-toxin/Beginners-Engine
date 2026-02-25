#pragma once
#include "Component.h"

struct LightComponent : public Component {
    enum class Type { Directional, Point };
    LightComponent(Type t = Type::Directional) : type(t) {}
    Type type = Type::Directional;
    float dirX = -0.5f, dirY = -1.0f, dirZ = -0.25f;
    int color = 0xFFFFFF;
    float intensity = 1.0f;

    std::shared_ptr<Component> Clone() const override {
        auto c = std::make_shared<LightComponent>(type);
        c->dirX = dirX; c->dirY = dirY; c->dirZ = dirZ;
        c->color = color; c->intensity = intensity;
        return c;
    }
};
