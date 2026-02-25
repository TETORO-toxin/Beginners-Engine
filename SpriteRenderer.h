#pragma once
#include "Component.h"
#include "DxLib.h"
#include <string>

// SpriteRenderer: 画像を描画するコンポーネント（簡易）
struct SpriteRenderer : public Component {
    SpriteRenderer() : handle_(-1) {}
    SpriteRenderer(const std::string& path) : handle_(-1), path_(path) {}
    ~SpriteRenderer() { if (handle_ != -1) DeleteGraph(handle_); }

    void Awake() override {
        if (!path_.empty()) {
            handle_ = LoadGraph(path_.c_str());
        }
    }

    void Render() override {
        if (handle_ == -1 || !owner) return;
        int x = static_cast<int>(owner->transform().x);
        int y = static_cast<int>(owner->transform().y);
        DrawGraph(x, y, handle_, TRUE);
    }

    std::shared_ptr<Component> Clone() const override {
        return std::make_shared<SpriteRenderer>(path_);
    }

    std::string path_;
    int handle_;
};
