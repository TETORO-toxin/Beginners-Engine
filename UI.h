#pragma once
#include "DxLib.h"
#include <string>
#include <unordered_map>
#include <fstream>
#include "GUI.h"

// UI: 軽量イベント駆動ウィジェット群（DxLib描画の上に構築）
// - ボタン、パネル（ドラッグ可能）、スライダ、テキストフィールド、数値フィールド、カラーピッカーを提供

namespace UI {

struct PanelState { int x, y, w, h; bool dragging; int dragOffsetX, dragOffsetY; };

// グローバル状態
static std::unordered_map<std::string, PanelState> g_panelStates;
// TextField の一時バッファを保持してフォーカス時に参照する
static std::unordered_map<void*, std::string> g_textBuffers;
static void* g_activeTextField = nullptr;

inline void Init() {}

// レイアウト保存/読み込み
inline bool SaveLayout(const std::string& path) {
    std::ofstream ofs(path);
    if (!ofs) return false;
    for (auto& kv : g_panelStates) {
        ofs << kv.first << " " << kv.second.x << " " << kv.second.y << " " << kv.second.w << " " << kv.second.h << "\n";
    }
    return true;
}
inline bool LoadLayout(const std::string& path) {
    std::ifstream ifs(path);
    if (!ifs) return false;
    std::string title;
    while (ifs >> title) {
        PanelState s; ifs >> s.x >> s.y >> s.w >> s.h;
        s.dragging = false; s.dragOffsetX = s.dragOffsetY = 0;
        g_panelStates[title] = s;
    }
    return true;
}

// パネル（タイトルで状態を保持）。ヘッダをドラッグして移動可能
inline void BeginPanel(const std::string& title, int& outX, int& outY, int w, int h) {
    auto it = g_panelStates.find(title);
    if (it == g_panelStates.end()) {
        PanelState s; s.x = outX; s.y = outY; s.w = w; s.h = h; s.dragging = false; s.dragOffsetX = 0; s.dragOffsetY = 0;
        g_panelStates[title] = s;
        it = g_panelStates.find(title);
    }
    PanelState& st = it->second;
    outX = st.x; outY = st.y;
    // Draw panel background and header
    DrawBox(st.x, st.y, st.x + st.w, st.y + st.h, GetColor(40,40,44), TRUE);
    DrawBox(st.x, st.y, st.x + st.w, st.y + 22, GetColor(33,37,43), TRUE);
    DrawString(st.x + 8, st.y + 4, title.c_str(), GetColor(200,200,200));

    int mx, my; GetMousePoint(&mx, &my);
    bool mouseDown = (GetMouseInput() & MOUSE_INPUT_LEFT) != 0;
    // header area
    if (mx >= st.x && mx <= st.x + st.w && my >= st.y && my <= st.y + 22) {
        if (mouseDown && !st.dragging) {
            st.dragging = true;
            st.dragOffsetX = mx - st.x;
            st.dragOffsetY = my - st.y;
        }
    }
    if (!mouseDown) st.dragging = false;
    if (st.dragging) {
        st.x = mx - st.dragOffsetX;
        st.y = my - st.dragOffsetY;
    }
}

inline void EndPanel() {
    // nothing for now
}

inline bool Button(int x, int y, int w, int h, const std::string& label) {
    // Use GUI::Button implementation directly
    return GUI::Button(x, y, w, h, label);
}

inline bool SliderFloat(int x, int y, int w, const std::string& label, float& value, float minv, float maxv) {
    int mx, my; GetMousePoint(&mx, &my);
    DrawBox(x, y, x + w, y + 16, GetColor(60,60,60), TRUE);
    // fill
    float t = 0.0f;
    if (maxv > minv) t = (value - minv) / (maxv - minv);
    int fillW = (int)(t * w);
    DrawBox(x, y, x + fillW, y + 16, GetColor(120,160,200), TRUE);
    DrawString(x + 4, y - 2, label.c_str(), GetColor(220,220,220));
    bool changed = false;
    bool inside = (mx >= x && mx <= x + w && my >= y && my <= y + 16);
    if (inside && (GetMouseInput() & MOUSE_INPUT_LEFT) != 0) {
        float nt = float(mx - x) / float(w);
        if (nt < 0) nt = 0; if (nt > 1) nt = 1;
        value = minv + nt * (maxv - minv);
        changed = true;
    }
    return changed;
}

inline bool TextField(int x, int y, int w, std::string& value) {
    int mx, my; GetMousePoint(&mx, &my);
    // Get buffer for this field
    void* key = &value;
    if (g_textBuffers.find(key) == g_textBuffers.end()) g_textBuffers[key] = value;
    std::string& buf = g_textBuffers[key];

    DrawBox(x, y, x + w, y + 20, GetColor(30,30,30), TRUE);
    DrawString(x + 4, y + 2, buf.c_str(), GetColor(220,220,220));
    bool clicked = (mx >= x && mx <= x + w && my >= y && my <= y + 20 && (GetMouseInput() & MOUSE_INPUT_LEFT) != 0);
    if (clicked) g_activeTextField = key;
    // 入力処理はフォーカスされたフィールドのみで行う
    if (g_activeTextField == key) {
        // letters A-Z
        for (int i = 0; i < 26; ++i) {
            int k = KEY_INPUT_A + i;
            if (CheckHitKey(k)) buf.push_back('a' + i);
        }
        // numbers
        for (int i = 0; i < 10; ++i) {
            int k = KEY_INPUT_0 + i;
            if (CheckHitKey(k)) buf.push_back('0' + i);
        }
        if (CheckHitKey(KEY_INPUT_SPACE)) buf.push_back(' ');
        if (CheckHitKey(KEY_INPUT_BACK)) { if (!buf.empty()) buf.pop_back(); }
        // Enter to commit
        if (CheckHitKey(KEY_INPUT_RETURN)) { value = buf; g_activeTextField = nullptr; }
        // click outside to unfocus
        if ((GetMouseInput() & MOUSE_INPUT_LEFT) != 0 && !(mx >= x && mx <= x + w && my >= y && my <= y + 20)) {
            // commit on unfocus
            value = buf;
            g_activeTextField = nullptr;
        }
    }
    return clicked;
}

inline bool NumberField(int x, int y, int w, float& value) {
    // display as text, but accept numeric keys when focused
    void* key = &value;
    if (g_textBuffers.find(key) == g_textBuffers.end()) {
        char buf0[64]; sprintf_s(buf0, sizeof(buf0), "%.3f", value);
        g_textBuffers[key] = buf0;
    }
    std::string& cur = g_textBuffers[key];
    bool clicked = TextField(x, y, w, cur);
    // Check key input for numbers
    bool changed = false;
    for (int i = 0; i < 10; ++i) { int k = KEY_INPUT_0 + i; if (CheckHitKey(k)) { cur.push_back('0' + i); changed = true; } }
    if (CheckHitKey(KEY_INPUT_PERIOD)) { cur.push_back('.'); changed = true; }
    if (CheckHitKey(KEY_INPUT_MINUS)) { cur.push_back('-'); changed = true; }
    if (CheckHitKey(KEY_INPUT_BACK)) { if (!cur.empty()) { cur.pop_back(); changed = true; } }
    if (CheckHitKey(KEY_INPUT_RETURN)) { value = (float)atof(cur.c_str()); g_activeTextField = nullptr; changed = true; }
    if (changed) {
        // immediate update optional
    }
    // draw numeric text
    DrawString(x + 4, y + 2, cur.c_str(), GetColor(220,220,220));
    return clicked || changed;
}

inline bool ColorPicker(int x, int y, int w, int h, int& r, int& g, int& b) {
    // Draw color preview
    DrawBox(x, y, x + 48, y + 48, GetColor(r, g, b), TRUE);
    bool changed = false;
    // RGB sliders
    float fr = (float)r, fg = (float)g, fb = (float)b;
    if (SliderFloat(x + 56, y, w - 56, "R", fr, 0, 255)) { r = (int)fr; changed = true; }
    if (SliderFloat(x + 56, y + 20, w - 56, "G", fg, 0, 255)) { g = (int)fg; changed = true; }
    if (SliderFloat(x + 56, y + 40, w - 56, "B", fb, 0, 255)) { b = (int)fb; changed = true; }
    return changed;
}

inline bool Vector2Field(int x, int y, const std::string& label, float& vx, float& vy) {
    DrawString(x, y, label.c_str(), GetColor(220,220,220));
    bool a = NumberField(x, y + 18, 100, vx);
    bool b = NumberField(x + 108, y + 18, 100, vy);
    return a || b;
}

}
