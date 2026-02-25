#pragma once

// Time: フレーム時間管理の簡易クラス
struct Time {
    static float deltaTime; // 前フレームからの経過時間(秒)
    static float time;      // 起動からの累計時間(秒)

    static void Update(float dt);
};
