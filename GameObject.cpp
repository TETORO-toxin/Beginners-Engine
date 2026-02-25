#include "GameObject.h"
#include "Component.h"

int GameObject::nextId_ = 1;

GameObject::GameObject(const std::string& name) : name_(name), id_(nextId_++) {}
GameObject::~GameObject() {}

void GameObject::Awake() {
    for (auto& c : components_) {
        c->Awake();
    }
}

void GameObject::Start() {
    if (started_) return;
    for (auto& c : components_) {
        c->Start();
    }
    started_ = true;
}

void GameObject::Update() {
    if (prefab_) return; // prefabs don't run updates
    if (!started_) Start();
    for (auto& c : components_) {
        if (c->enabled) c->Update();
    }
}

void GameObject::Render() {
    if (prefab_) return; // prefabs don't render in scene
    for (auto& c : components_) {
        if (c->enabled) c->Render();
    }
}

void GameObject::SetParent(std::shared_ptr<GameObject> parent) {
    // 既存の親から切り離す
    if (auto p = parent_.lock()) {
        // children管理はTransform側なのでここではTransformを操作
        auto& children = p->transform().children;
        children.erase(std::remove(children.begin(), children.end(), &transform_), children.end());
    }

    parent_ = parent;
    if (parent) {
        transform_.parent = &parent->transform();
        parent->transform().children.push_back(&transform_);
    } else {
        transform_.parent = nullptr;
    }
}

std::shared_ptr<GameObject> GameObject::Clone() const {
    auto clone = std::make_shared<GameObject>(name_ + "_Clone");
    clone->transform_ = transform_;
    // clone 3D z and scale
    clone->transform_.z = transform_.z;
    clone->transform_.scaleZ = transform_.scaleZ;
    // preserve prefab flag on clone as false by default (instances are not prefabs)
    clone->prefab_ = false;
    // 親/子関係はクローンでは引き継がれない（必要なら外部で設定）
    clone->transform_.parent = nullptr;
    clone->transform_.children.clear();
    // コンポーネントのCloneを呼ぶ
    for (auto& c : components_) {
        auto copy = c->Clone();
        if (copy) {
            copy->owner = clone.get();
            clone->components_.push_back(copy);
        }
    }
    return clone;
}

void GameObject::RemoveComponentAt(size_t index) {
    if (index >= components_.size()) return;
    components_.erase(components_.begin() + index);
}
