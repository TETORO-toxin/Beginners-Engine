#pragma once
#include "Component.h"
#include <set>

// Collider: 簡易的なAABBコライダ
struct Collider : public Component {
    float width = 0;
    float height = 0;

    // 衝突管理: 現在衝突しているコライダ集合
    std::set<Collider*> currentCollisions;

    Collider() {}
    Collider(float w, float h) : width(w), height(h) {}

    // AABB判定（ローカル中心を基準にする）
    bool TestAABB(float ax, float ay, float aw, float ah, float bx, float by, float bw, float bh) const {
        return !(ax + aw < bx || bx + bw < ax || ay + ah < by || by + bh < ay);
    }

    std::shared_ptr<Component> Clone() const override {
        return std::make_shared<Collider>(width, height);
    }
};
