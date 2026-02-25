#pragma once
#include "Component.h"
#include "DxLib.h"
#include <string>

// LabelComponent: テキストを描画する汎用コンポーネント
struct LabelComponent : public Component {
    std::string text;
    int color;
    LabelComponent() : text(""), color(GetColor(255,255,255)) {}
    LabelComponent(const std::string& t, int c = GetColor(255,255,255)) : text(t), color(c) {}
    void Render() override {
        if (!owner) return;
        int x = static_cast<int>(owner->transform().x);
        int y = static_cast<int>(owner->transform().y);
        DrawString(x, y, text.c_str(), color);
    }
    std::shared_ptr<Component> Clone() const override { return std::make_shared<LabelComponent>(text, color); }
};
