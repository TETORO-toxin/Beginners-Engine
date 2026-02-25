#pragma once
#include <vector>
#include <memory>
#include <string>
#include <algorithm>
#include "Component.h"
#include "Transform.h"

// GameObject: Unity風のオブジェクト。複数のComponentを持つことができる
class GameObject : public std::enable_shared_from_this<GameObject> {
public:
    GameObject(const std::string& name = "GameObject");
    ~GameObject();

    // ライフサイクル
    void Awake();
    void Start();
    void Update();
    void Render();

    // Componentの追加
    template<typename T, typename... Args>
    std::shared_ptr<T> AddComponent(Args&&... args)
    {
        auto comp = std::make_shared<T>(std::forward<Args>(args)...);
        comp->owner = this;
        components_.push_back(comp);
        comp->Awake();
        return comp;
    }

    // Component検索
    template<typename T>
    std::shared_ptr<T> GetComponent()
    {
        for (auto& c : components_) {
            auto ptr = std::dynamic_pointer_cast<T>(c);
            if (ptr) return ptr;
        }
        return nullptr;
    }

    // 全コンポーネント取得（衝突検出等で内部使用）
    const std::vector<std::shared_ptr<Component>>& GetAllComponents() const { return components_; }

    // コンポーネントの個数
    size_t GetComponentCount() const { return components_.size(); }

    // コンポーネント削除（インデックス指定）
    void RemoveComponentAt(size_t index);

    // Transform取得
    Transform& transform() { return transform_; }

    // 親子関係
    void SetParent(std::shared_ptr<GameObject> parent);
    std::shared_ptr<GameObject> parent() const { return parent_.lock(); }

    const std::string& name() const { return name_; }
    void SetName(const std::string& n) { name_ = n; }

    // Prefab複製
    std::shared_ptr<GameObject> Clone() const;

    // 固有ID
    int id() const { return id_; }

    // Prefab flag: mark an object as prefab template (will be ignored from scene updates)
    void SetPrefab(bool v) { prefab_ = v; }
    bool IsPrefab() const { return prefab_; }

private:
    static int nextId_;
    int id_;
    std::string name_;
    Transform transform_;
    std::vector<std::shared_ptr<Component>> components_;
    std::weak_ptr<GameObject> parent_;
    bool started_ = false;
    bool prefab_ = false;
};
