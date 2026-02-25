#pragma once
#include <vector>
#include <memory>
#include <string>
#include "GameObject.h"

// Forward declare Collider as struct to match its definition in Collider.h
struct Collider;

// Scene: GameObject collection and basic scene lifecycle
class Scene {
public:
    Scene() {}
    ~Scene() {}

    void AddRootObject(std::shared_ptr<GameObject> obj);
    void RemoveRootObject(std::shared_ptr<GameObject> obj);

    // Register a prefab template (stored separately, not part of scene root list)
    void AddPrefab(std::shared_ptr<GameObject> prefab);
    const std::vector<std::shared_ptr<GameObject>>& GetPrefabs() const { return prefabs_; }

    void Awake();
    void Start();
    void Update();
    void Render();

    // Prefab instantiation (returns clone)
    std::shared_ptr<GameObject> Instantiate(std::shared_ptr<GameObject> prefab);

    // Scene save/load
    bool Save(const std::string& path);
    bool Load(const std::string& path);

    // Root objects access
    const std::vector<std::shared_ptr<GameObject>>& GetRoots() const { return roots_; }

    // Selection API: single selected GameObject (owned externally by scene roots)
    void SetSelectedObject(std::shared_ptr<GameObject> obj) { selected_ = obj; }
    std::shared_ptr<GameObject> GetSelectedObject() const { return selected_.lock(); }
    void ClearSelection() { selected_.reset(); }

    // Find GameObject by name in roots (shallow search)
    std::shared_ptr<GameObject> FindRootByName(const std::string& name) const;

    // Render mode (2D or 3D)
    enum class RenderMode { Mode2D, Mode3D };
    RenderMode renderMode = RenderMode::Mode2D;

    struct Camera2D {
        float x = 0;
        float y = 0;
        float zoom = 1.0f;
    } camera;

    struct Camera3D {
        float x = 0; // target position
        float y = 0;
        float z = 0;
        float yaw = 0; // rotation around Y
        float pitch = 0; // rotation around X
        float distance = 500.0f; // distance from target
        float fov = 60.0f;
    } camera3D;

    struct DirectionalLight {
        float dirX = -0.5f;
        float dirY = -1.0f;
        float dirZ = -0.25f;
        int color = 0xFFFFFF;
        float intensity = 1.0f;
    } mainLight;

    bool showGrid = true;

    int RenderToTarget(int width, int height, const Camera2D& cam);
    int RenderToTarget3D(int width, int height, const Camera3D& cam);
    int RenderToTarget(int width, int height); // fallback that uses current renderMode

private:
    std::vector<std::shared_ptr<GameObject>> roots_;
    std::vector<std::shared_ptr<GameObject>> prefabs_;

    std::weak_ptr<GameObject> selected_;

    std::vector<Collider*> colliders_;

    void RebuildColliderList();
    void PhysicsStep();
};
