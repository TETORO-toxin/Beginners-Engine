#pragma once

#include <memory>

// Forward宣言
class GameObject;
struct Collider;

// Component: GameObjectにアタッチされる基底クラス
struct Component {
    Component() {}
    virtual ~Component() {}

    // 所有するGameObjectへのポインタ(生ポインタ、GameObjectはshared_ptr管理)
    GameObject* owner = nullptr;

    // ライフサイクル（Unity風）
    // Awake: オブジェクト生成直後に一度呼ばれる
    virtual void Awake() {}
    // Start: 最初のUpdate前に一度呼ばれる
    virtual void Start() {}
    // OnEnable: コンポーネントが有効化されたとき
    virtual void OnEnable() {}
    // OnDisable: コンポーネントが無効化されたとき
    virtual void OnDisable() {}

    // 各フレームに呼ばれる
    virtual void Update() {}
    virtual void Render() {}

    // 衝突イベント（Collider同士の衝突時に呼ばれる）
    virtual void OnCollisionEnter(Collider* other) {}
    virtual void OnCollisionStay(Collider* other) {}
    virtual void OnCollisionExit(Collider* other) {}

    // このコンポーネントが有効かどうか（無効ならUpdate/Renderは呼ばれない）
    bool enabled = true;

    // Prefab複製のためのClone
    virtual std::shared_ptr<Component> Clone() const { return nullptr; }
};
