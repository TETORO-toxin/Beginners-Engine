#include "DxLib.h"
#include "Engine.h"
#include "Scene.h"
#include "GameObject.h"
#include "Component.h"
#include "Transform.h"
#include "Time.h"
#include "Input.h"
#include "GUI.h"
#include "GUIEditor.h"
#include "SpriteRenderer.h"
#include "Collider.h"
#include "LabelComponent.h"
#include "EditorUI.h"
#include "UI.h"
#include "MeshRenderer.h"
#include "CameraComponent.h"
#include "LightComponent.h"

#include <memory>
#include <string>

// サンプルのコンポーネント: 簡易的に移動させる
struct MoveComponent : public Component {
    float vx, vy, vz;
    MoveComponent(float _vx = 0, float _vy = 0, float _vz = 0) : vx(_vx), vy(_vy), vz(_vz) {}
    void Update() override {
        if (!owner) return;
        owner->transform().x += vx * Time::deltaTime;
        owner->transform().y += vy * Time::deltaTime;
        owner->transform().z += vz * Time::deltaTime;
    }
    std::shared_ptr<Component> Clone() const override { return std::make_shared<MoveComponent>(vx, vy, vz); }
};

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    // ウィンドウサイズを設定（必要なら要調整）
    const int screenW = 1280;
    const int screenH = 720;
    // ウィンドウモードにしてから DxLib を初期化する（SetGraphMode/ChangeWindowMode は DxLib_Init 前に呼ぶ）
    ChangeWindowMode(TRUE);
    SetGraphMode(screenW, screenH, 32);

    // エンジン初期化（内部でDxLib_Initを呼ぶ）
    if (!Engine::Instance().Init()) {
        return -1;
    }

    // マウスカーソルを表示（念のため）
    SetMouseDispFlag(TRUE);

    // UI layout load
    UI::LoadLayout("layout.txt");

    // シーンを作成
    Scene scene;

    // Create main camera object
    auto mainCam = std::make_shared<GameObject>("Main Camera");
    mainCam->transform().x = 0.0f; mainCam->transform().y = 100.0f; mainCam->transform().z = 0.0f;
    mainCam->AddComponent<CameraComponent>(0.0f, 20.0f, 500.0f, 60.0f);
    scene.AddRootObject(mainCam);

    // Create main directional light
    auto mainLight = std::make_shared<GameObject>("Directional Light");
    mainLight->AddComponent<LightComponent>(LightComponent::Type::Directional);
    scene.AddRootObject(mainLight);

    // プレハブとして使えるGameObjectを作る (register as prefab template)
    auto prefab = std::make_shared<GameObject>("PrefabLabel");
    prefab->transform().x = 100.0f;
    prefab->transform().y = 0.0f;
    prefab->transform().z = 100.0f;
    prefab->transform().scaleX = 2.0f; prefab->transform().scaleY = 2.0f; prefab->transform().scaleZ = 2.0f;
    prefab->AddComponent<LabelComponent>("Prefab");
    prefab->AddComponent<Collider>(32.0f, 16.0f);
    prefab->SetPrefab(true);
    scene.AddPrefab(prefab);

    // プレハブからインスタンス化
    auto go = scene.Instantiate(prefab);
    go->transform().x = 200.0f;
    go->transform().y = 0.0f;
    go->transform().z = 160.0f;
    go->AddComponent<MoveComponent>(30.0f, 0.0f, 0.0f);

    // A 3D cube using MeshRenderer
    auto cube = std::make_shared<GameObject>("Cube");
    cube->transform().x = 400.0f; cube->transform().y = 50.0f; cube->transform().z = 240.0f;
    cube->transform().scaleX = 30.0f; cube->transform().scaleY = 30.0f; cube->transform().scaleZ = 30.0f;
    cube->AddComponent<MeshRenderer>(GetColor(200,100,100));
    scene.AddRootObject(cube);

    // サンプル用のラベルオブジェクト (2D overlay style)
    auto label = std::make_shared<GameObject>("Label");
    label->transform().x = 20.0f; label->transform().y = 300.0f;
    label->AddComponent<LabelComponent>("Hello, Unity6-like Engine");
    scene.AddRootObject(label);

    // インスペクタで操作する対象
    std::shared_ptr<GameObject> inspectTarget = go;
    bool showInspector = true;

    // camera initial
    scene.camera.x = 0;
    scene.camera.y = 0;
    scene.camera.zoom = 1.0f;
    scene.camera3D.x = 0; scene.camera3D.y = 100; scene.camera3D.z = 0;

    // メインループ
    bool running = true;
    int prevTime = GetNowCount();

    // Precompute viewport size to match EditorUI layout
    const int leftW = 220;
    const int rightW = 300;
    const int bottomH = 180;
    const int topH = 28;
    const int viewW = screenW - leftW - rightW - 48;
    const int viewH = screenH - topH - bottomH - 16;

    while (running && ProcessMessage() == 0 && ClearDrawScreen() == 0) {
        int now = GetNowCount();
        int elapsed = now - prevTime;
        prevTime = now;
        Time::Update(elapsed / 1000.0f);

        // Camera controls: WASD to pan (2D) / arrow keys to orbit (3D), mouse wheel to zoom/distance
        int mw = GetMouseWheelRotVol();
        if (mw != 0) {
            if (scene.renderMode == Scene::RenderMode::Mode2D) {
                scene.camera.zoom *= (mw > 0) ? 1.1f : 0.9f;
                if (scene.camera.zoom < 0.1f) scene.camera.zoom = 0.1f;
            } else {
                scene.camera3D.distance *= (mw > 0) ? 0.9f : 1.1f;
                if (scene.camera3D.distance < 10) scene.camera3D.distance = 10;
            }
        }
        float camSpeed = 300.0f * Time::deltaTime;
        if (scene.renderMode == Scene::RenderMode::Mode2D) {
            if (Input::GetKey(KEY_INPUT_W)) scene.camera.y -= camSpeed / scene.camera.zoom;
            if (Input::GetKey(KEY_INPUT_S)) scene.camera.y += camSpeed / scene.camera.zoom;
            if (Input::GetKey(KEY_INPUT_A)) scene.camera.x -= camSpeed / scene.camera.zoom;
            if (Input::GetKey(KEY_INPUT_D)) scene.camera.x += camSpeed / scene.camera.zoom;
        } else {
            if (Input::GetKey(KEY_INPUT_LEFT)) scene.camera3D.yaw -= 60.0f * Time::deltaTime;
            if (Input::GetKey(KEY_INPUT_RIGHT)) scene.camera3D.yaw += 60.0f * Time::deltaTime;
            if (Input::GetKey(KEY_INPUT_UP)) scene.camera3D.pitch -= 30.0f * Time::deltaTime;
            if (Input::GetKey(KEY_INPUT_DOWN)) scene.camera3D.pitch += 30.0f * Time::deltaTime;
        }

        // Toggle render mode with M key
        if (CheckHitKey(KEY_INPUT_M)) {
            scene.renderMode = (scene.renderMode == Scene::RenderMode::Mode2D) ? Scene::RenderMode::Mode3D : Scene::RenderMode::Mode2D;
        }

        // シーン更新
        scene.Update();

        // Render scene into offscreen target using camera
        int viewportGraph = scene.RenderToTarget(viewW, viewH);

        // Editor UI（Godot風レイアウトを模したUI）
        EditorUI::DrawEditor(scene, inspectTarget, viewportGraph, screenW, screenH);

        // インスペクタの通常GUIも描画（追加の操作用）
        if (GUI::Button(10, 40, 160, 32, showInspector ? "Inspector: On" : "Inspector: Off")) {
            showInspector = !showInspector;
        }
        if (showInspector) {
            GUIEditor::DrawInspector(inspectTarget);
        }

        // キーでプレハブ複製
        if (Input::GetKey(KEY_INPUT_SPACE)) {
            auto inst = scene.Instantiate(prefab);
            inst->transform().x = 50.0f + static_cast<float>(rand() % 300);
            inst->transform().y = 50.0f + static_cast<float>(rand() % 200);
        }

        // 画面反映
        ScreenFlip();

        // Delete the offscreen graph to free resources
        if (viewportGraph != 0) {
            DeleteGraph(viewportGraph);
            viewportGraph = 0;
        }

        // 終了チェック
        if (CheckHitKey(KEY_INPUT_ESCAPE)) running = false;
    }

    // Save layout
    UI::SaveLayout("layout.txt");

    DxLib_End();
    return 0;
}