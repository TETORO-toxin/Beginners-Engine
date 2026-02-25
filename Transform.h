#pragma once
#include <vector>
#include <memory>

// Transform: 位置・回転・スケールを保持する簡易構造体
struct Transform {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f; // 3D Z coordinate
    float rotation = 0.0f; // 2D rotation (degrees)
    float rotationX = 0.0f; // 3D rotations (degrees)
    float rotationY = 0.0f;
    float rotationZ = 0.0f;
    float scaleX = 1.0f;
    float scaleY = 1.0f;
    float scaleZ = 1.0f;

    // 親子関係のための参照（非 owning）
    Transform* parent = nullptr;
    std::vector<Transform*> children;

    // ローカル座標からワールド座標を計算する簡易メソッド (2D)
    void GetWorldPosition(float& outX, float& outY) const {
        if (parent) {
            float px, py;
            parent->GetWorldPosition(px, py);
            outX = px + x;
            outY = py + y;
        } else {
            outX = x;
            outY = y;
        }
    }

    // ローカル座標からワールド座標を計算する簡易メソッド (3D)
    void GetWorldPosition3D(float& outX, float& outY, float& outZ) const {
        if (parent) {
            float px, py, pz;
            parent->GetWorldPosition3D(px, py, pz);
            outX = px + x;
            outY = py + y;
            outZ = pz + z;
        } else {
            outX = x;
            outY = y;
            outZ = z;
        }
    }
};
