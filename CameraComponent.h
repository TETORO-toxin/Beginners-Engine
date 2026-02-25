#pragma once
#include "Component.h"

// Simple CameraComponent for 3D view
struct CameraComponent : public Component {
    CameraComponent() {}
    CameraComponent(float _yaw, float _pitch, float _distance, float _fov)
        : yaw(_yaw), pitch(_pitch), distance(_distance), fov(_fov) {}

    float yaw = 0.0f;
    float pitch = 20.0f;
    float distance = 500.0f;
    float fov = 60.0f;

    std::shared_ptr<Component> Clone() const override {
        return std::make_shared<CameraComponent>(yaw, pitch, distance, fov);
    }
};
