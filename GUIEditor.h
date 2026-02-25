#pragma once
#include "DxLib.h"
#include <memory>
#include <string>
#include "GameObject.h"
#include "Component.h"
#include "GUI.h"
#include "LabelComponent.h"
#include "SpriteRenderer.h"
#include "Collider.h"

// GUIEditor: 簡易エディタ風インスペクタ（ドラッグで位置の編集、コンポーネント一覧の表示）
namespace GUIEditor {

inline void DrawInspector(std::shared_ptr<GameObject> obj) {
    if (!obj) return;
    GUI::Label(400, 10, (std::string("Inspector: ") + obj->name()).c_str());
    float x = obj->transform().x;
    float y = obj->transform().y;
    DrawFormatString(400, 40, GetColor(255,255,255), "Position: %.1f, %.1f", x, y);

    // ドラッグで移動: Inspector上のボタンを押しながらマウスを動かす
    static bool dragging = false;
    int mx, my;
    GetMousePoint(&mx, &my);
    if (GUI::Button(400, 70, 120, 28, "Drag Position")) {
        dragging = true;
    }
    if (dragging) {
        obj->transform().x = (float)mx;
        obj->transform().y = (float)my;
        // クリックを離したらドラッグ終了
        if ((GetMouseInput() & MOUSE_INPUT_LEFT) == 0) dragging = false;
    }

    // コンポーネント一覧表示と簡易編集
    int y0 = 110;
    int idx = 0;
    for (auto& c : obj->GetAllComponents()) {
        std::string typeName = "Component";
        if (dynamic_cast<LabelComponent*>(c.get())) typeName = "Label";
        if (dynamic_cast<SpriteRenderer*>(c.get())) typeName = "Sprite";
        if (dynamic_cast<Collider*>(c.get())) typeName = "Collider";
        DrawFormatString(400, y0 + idx * 24, GetColor(200,200,200), "%d: %s", idx, typeName.c_str());
        // 有効/無効トグル
        if (GUI::Button(520, y0 + idx * 24, 80, 20, c->enabled ? "Enabled" : "Disabled")) {
            c->enabled = !c->enabled;
            if (c->enabled) c->OnEnable(); else c->OnDisable();
        }
        idx++;
    }
}

}
