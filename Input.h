#pragma once
#include "DxLib.h"

// Input: シンプルな入力ラッパー
struct Input {
    // キーが押されているか
    static bool GetKey(int key) { return CheckHitKey(key) != 0; }

    // マウス座標取得
    static void GetMouse(int& x, int& y) { GetMousePoint(&x, &y); }
};
