#pragma once
#include "DxLib.h"
#include <string>
#include <unordered_map>

// GUI: DxLibの簡易ウィジェット（Label/Button）
namespace GUI {

// Label
inline void Label(int x, int y, const std::string& text, int color = GetColor(255,255,255)) {
    DrawString(x, y, text.c_str(), color);
}

// Helper: stable key for a button based on geometry and label
inline size_t ButtonKey(int x, int y, int w, int h, const std::string& text) {
    // combine values into a size_t
    size_t h1 = std::hash<std::string>()(text);
    size_t key = h1 ^ (static_cast<size_t>(x) << 32) ^ (static_cast<size_t>(y) << 16) ^ (static_cast<size_t>(w) << 8) ^ static_cast<size_t>(h);
    return key;
}

// Button: returns true only once per mouse down (rising edge) for each distinct button
inline bool Button(int x, int y, int w, int h, const std::string& text) {
    static std::unordered_map<size_t,int> s_prevMap;
    int mx, my;
    GetMousePoint(&mx, &my);
    int currentMouse = GetMouseInput();
    bool inside = (mx >= x && mx <= x + w && my >= y && my <= y + h);
    int color = inside ? GetColor(200,200,255) : GetColor(100,100,200);
    DrawBox(x, y, x + w, y + h, color, TRUE);
    DrawString(x + 8, y + 8, text.c_str(), GetColor(255,255,255));
    bool clicked = false;
    bool leftNow = (currentMouse & MOUSE_INPUT_LEFT) != 0;

    size_t key = ButtonKey(x,y,w,h,text);
    int prevMouse = 0;
    auto it = s_prevMap.find(key);
    if (it != s_prevMap.end()) prevMouse = it->second;
    bool leftPrev = (prevMouse & MOUSE_INPUT_LEFT) != 0;
    if (inside && leftNow && !leftPrev) clicked = true; // rising edge
    s_prevMap[key] = currentMouse;
    return clicked;
}

}
