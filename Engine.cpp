#include "Engine.h"
#include "DxLib.h"
#include "Time.h"
#include "RenderGraph.h"
#include "DeferredPass.h"
#include "ForwardPlusPass.h"
#include <Windows.h>

Engine& Engine::Instance() {
    static Engine instance;
    return instance;
}

Engine::Engine() : running_(false), name_("Beginners Engine") {}
Engine::~Engine() {}

bool Engine::Init() {
    if (DxLib_Init() == -1) return false;
    SetDrawScreen(DX_SCREEN_BACK);
    SetMouseDispFlag(TRUE);
    while (::ShowCursor(TRUE) < 0) {}

    SetMainWindowText(name_.c_str());

    renderGraph_ = std::make_unique<RenderGraph>();
    renderGraph_->AddPass(std::make_shared<DeferredPass>());
    renderGraph_->AddPass(std::make_shared<ForwardPlusPass>());

    running_ = true;
    return true;
}

void Engine::Play() {
    if (isPlaying_) return;
    isPlaying_ = true;
    for (auto& obj : objects_) obj->Start();
}

void Engine::Stop() {
    if (!isPlaying_) return;
    isPlaying_ = false;
    for (auto& obj : objects_) obj->Awake();
}

void Engine::Run() {
    while (running_ && ProcessMessage() == 0 && ClearDrawScreen() == 0) {
        static int prevTime = GetNowCount();
        int now = GetNowCount();
        int elapsed = now - prevTime;
        prevTime = now;
        Time::Update(elapsed / 1000.0f);

        // Toggle play mode with Space key (rising edge)
        static bool prevSpaceDown = false;
        bool curSpaceDown = (CheckHitKey(KEY_INPUT_SPACE) != 0);
        if (curSpaceDown && !prevSpaceDown) TogglePlay();
        prevSpaceDown = curSpaceDown;

        // Update
        for (auto& obj : objects_) {
            obj->Update();
        }

        if (renderGraph_) {
            renderGraph_->Execute();
        } else {
            for (auto& obj : objects_) {
                obj->Render();
            }
        }

        ScreenFlip();

        if (CheckHitKey(KEY_INPUT_ESCAPE)) {
            Quit();
        }
    }
    DxLib_End();
}

void Engine::Quit() { running_ = false; }

void Engine::AddGameObject(std::shared_ptr<GameObject> obj) {
    objects_.push_back(obj);
}
