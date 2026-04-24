#pragma once
#include <vector>
#include <memory>
#include <string>
#include "GameObject.h"
class Scene;

// Forward declare RenderGraph to avoid including heavy headers in Engine.h
class RenderGraph;

// Engine: DxLibを用いてUnityの簡易ランタイム風に振る舞うシングルトンクラス
// - ゲームループ制御
// - GameObjectの管理
// - 各フレームの初期化・描画・更新の呼び出し
class Engine {
public:
    // シングルトン取得
    static Engine& Instance();

    // エンジン初期化
    bool Init();

    // メインループ開始
    void Run();

    // 終了フラグを立てる
    void Quit();

    // GameObjectを登録する
    void AddGameObject(std::shared_ptr<GameObject> obj);

    // Accessor for render graph (may be null if not initialized)
    RenderGraph* GetRenderGraph() { return renderGraph_.get(); }

    // Engine name accessors
    const std::string& GetName() const { return name_; }
    void SetName(const std::string& name) { name_ = name; }

    // Editor play mode control
    bool IsPlaying() const { return isPlaying_; }
    void Play();
    void Stop();
    void TogglePlay() { if (isPlaying_) Stop(); else Play(); }

    // Active scene to control when entering/exiting play mode
    void SetActiveScene(Scene* scene) { activeScene_ = scene; }
    Scene* GetActiveScene() const { return activeScene_; }

private:
    Engine();
    ~Engine();

    bool running_;
    bool isPlaying_ = false;
    std::vector<std::shared_ptr<GameObject>> objects_;

    // Non-owning pointer to the currently edited scene (may be null)
    Scene* activeScene_ = nullptr;

    // Render pipeline core
    std::unique_ptr<RenderGraph> renderGraph_;

    // Human-readable engine name
    std::string name_;
};
