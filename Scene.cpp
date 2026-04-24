#include "Scene.h"
#include "Collider.h"
#include "Serializer.h"
#include "DxLib.h"
#include "SpriteRenderer.h"
#include "CameraComponent.h"
#include "Lighting.h"
#include <algorithm>
#include <cmath>

void Scene::AddRootObject(std::shared_ptr<GameObject> obj) {
    if (!obj) return;
    if (obj->IsPrefab()) return;
    roots_.push_back(obj);
    // Awake immediately for editor-time root
    obj->Awake();
    if (inPlayMode_) {
        // also add to runtime roots if playing
        auto inst = obj->Clone();
        playRoots_.push_back(inst);
        inst->Awake();
    }

    RebuildColliderList();
}

std::vector<std::string> Scene::CollectOverridesFor(const std::shared_ptr<GameObject>& instance) const {
    std::vector<std::string> out;
    if (!instance) return out;
    // If this instance references a prefab asset, attempt to load prefab and compare
    std::string prefabPath = instance->GetPrefabSourcePath();
    if (prefabPath.empty()) return out;
    auto prefab = Serializer::LoadPrefab(prefabPath);
    if (!prefab) return out;
    // Compare transform x/y only for minimal implementation
    if (fabsf(instance->transform().x - prefab->transform().x) > 1e-6f) {
        out.push_back("PROP:" + instance->name() + ":transform.x:" + std::to_string(instance->transform().x));
    }
    if (fabsf(instance->transform().y - prefab->transform().y) > 1e-6f) {
        out.push_back("PROP:" + instance->name() + ":transform.y:" + std::to_string(instance->transform().y));
    }
    return out;
}

void Scene::ApplyOverridesTo(const std::shared_ptr<GameObject>& instance, const std::vector<std::string>& overrides) {
    if (!instance) return;
    for (auto& o : overrides) {
        // expected format PROP:Name:field:val
        if (o.rfind("PROP:",0) != 0) continue;
        size_t p1 = o.find(':',5);
        if (p1 == std::string::npos) continue;
        size_t p2 = o.find(':', p1+1);
        if (p2 == std::string::npos) continue;
        std::string iname = o.substr(5, p1-5);
        std::string field = o.substr(p1+1, p2 - (p1+1));
        std::string val = o.substr(p2+1);
        if (iname != instance->name()) continue;
        if (field == "transform.x") instance->transform().x = std::stof(val);
        else if (field == "transform.y") instance->transform().y = std::stof(val);
    }
}

void Scene::RemoveRootObject(std::shared_ptr<GameObject> obj) {
    // if removed object is selected, clear selection
    if (!selected_.expired() && selected_.lock() == obj) selected_.reset();
    roots_.erase(std::remove(roots_.begin(), roots_.end(), obj), roots_.end());
    if (inPlayMode_) {
        // also remove any runtime clones that match by name (best-effort)
        playRoots_.erase(std::remove_if(playRoots_.begin(), playRoots_.end(), [&](const std::shared_ptr<GameObject>& p){ return p->name() == obj->name(); }), playRoots_.end());
    }
    RebuildColliderList();
}

void Scene::AddPrefab(std::shared_ptr<GameObject> prefab) {
    if (!prefab) return;
    prefab->SetPrefab(true);
    prefab->Awake();
    prefabs_.push_back(prefab);
}

void Scene::Awake() {
    auto& src = inPlayMode_ ? playRoots_ : roots_;
    for (auto& r : src) r->Awake();
    RebuildColliderList();
}

void Scene::Start() {
    auto& src = inPlayMode_ ? playRoots_ : roots_;
    for (auto& r : src) r->Start();
}

void Scene::Update() {
    // update all root objects (use playRoots_ when in play mode)
    auto& src = inPlayMode_ ? playRoots_ : roots_;
    for (auto& r : src) r->Update();
    // collision / AABB handling
    PhysicsStep();
}

void Scene::Render() {
    auto& src = inPlayMode_ ? playRoots_ : roots_;
    for (auto& r : src) {
        if (r->IsPrefab()) continue; // never render prefab templates
        r->Render();
    }
}

std::shared_ptr<GameObject> Scene::Instantiate(std::shared_ptr<GameObject> prefab) {
    if (!prefab) return nullptr;
    auto inst = prefab->Clone();
    AddRootObject(inst);
    // if in play mode, also create runtime clone used in playRoots_
    if (inPlayMode_) {
        auto runInst = prefab->Clone();
        playRoots_.push_back(runInst);
        runInst->Awake();
        runInst->Start();
    }
    return inst;
}

void Scene::RebuildColliderList() {
    colliders_.clear();
    auto& src = inPlayMode_ ? playRoots_ : roots_;
    for (auto& r : src) {
        if (r->IsPrefab()) continue;
        for (auto& c : r->GetAllComponents()) {
            auto col = std::dynamic_pointer_cast<Collider>(c);
            if (col) colliders_.push_back(col.get());
        }
    }
}

void Scene::PhysicsStep() {
    // naive AABB collision detection
    for (size_t i = 0; i < colliders_.size(); ++i) {
        Collider* a = colliders_[i];
        GameObject* ao = a->owner;
        float ax = ao->transform().x;
        float ay = ao->transform().y;
        float aw = a->width;
        float ah = a->height;
        for (size_t j = i + 1; j < colliders_.size(); ++j) {
            Collider* b = colliders_[j];
            GameObject* bo = b->owner;
            float bx = bo->transform().x;
            float by = bo->transform().y;
            float bw = b->width;
            float bh = b->height;
            bool hit = a->TestAABB(ax, ay, aw, ah, bx, by, bw, bh);
            bool wasHit = a->currentCollisions.count(b) > 0;
            if (hit && !wasHit) {
                a->currentCollisions.insert(b);
                b->currentCollisions.insert(a);
                for (auto& comp : ao->GetAllComponents()) comp->OnCollisionEnter(b);
                for (auto& comp : bo->GetAllComponents()) comp->OnCollisionEnter(a);
            } else if (hit && wasHit) {
                for (auto& comp : ao->GetAllComponents()) comp->OnCollisionStay(b);
                for (auto& comp : bo->GetAllComponents()) comp->OnCollisionStay(a);
            } else if (!hit && wasHit) {
                a->currentCollisions.erase(b);
                b->currentCollisions.erase(a);
                for (auto& comp : ao->GetAllComponents()) comp->OnCollisionExit(b);
                for (auto& comp : bo->GetAllComponents()) comp->OnCollisionExit(a);
            }
        }
    }
}

bool Scene::Save(const std::string& path) {
    return Serializer::SaveScene(path, roots_);
}

bool Scene::Load(const std::string& path) {
    roots_.clear();
    bool ok = Serializer::LoadScene(path, roots_);
    RebuildColliderList();
    return ok;
}

void Scene::EnterPlayMode() {
    if (inPlayMode_) return;
    playRoots_.clear();
    // deep-clone root objects for runtime
    for (auto& r : roots_) {
        if (r->IsPrefab()) continue;
        auto clone = r->Clone();
        playRoots_.push_back(clone);
    }
    inPlayMode_ = true;
    // initialize runtime clones
    for (auto& r : playRoots_) r->Awake();
    for (auto& r : playRoots_) r->Start();
    RebuildColliderList();
}

void Scene::ExitPlayMode() {
    if (!inPlayMode_) return;
    // discard runtime clones
    playRoots_.clear();
    inPlayMode_ = false;
    // restore editor-time state: wake editor roots so inspector shows expected values
    for (auto& r : roots_) r->Awake();
    RebuildColliderList();
}

int Scene::RenderToTarget(int width, int height, const Camera2D& cam) {
    int screen = MakeScreen(width, height, TRUE);
    if (screen == -1) return 0;
    int prev = GetDrawScreen();
    SetDrawScreen(screen);
    ClearDrawScreen();
    auto& src = inPlayMode_ ? playRoots_ : roots_;
    for (auto& r : src) {
        if (r->IsPrefab()) continue; // never render prefab templates
        float ox = r->transform().x;
        float oy = r->transform().y;
        r->transform().x = (ox - cam.x) * cam.zoom + width / 2.0f;
        r->transform().y = (oy - cam.y) * cam.zoom + height / 2.0f;
        r->Render();
        r->transform().x = ox;
        r->transform().y = oy;
    }

    SetDrawScreen(prev);
    return screen;
}

int Scene::RenderToTarget3D(int width, int height, const Camera3D& cam) {
    Camera3D camUsed = cam;
    for (auto& obj : roots_) {
        if (obj->IsPrefab()) continue; // ignore prefab templates
        for (auto& c : obj->GetAllComponents()) {
            auto camComp = std::dynamic_pointer_cast<CameraComponent>(c);
            if (camComp) {
                camComp->yaw = cam.yaw;
                camComp->pitch = cam.pitch;
                camComp->distance = cam.distance;
                camComp->fov = cam.fov;

                camUsed.yaw = camComp->yaw;
                camUsed.pitch = camComp->pitch;
                camUsed.distance = camComp->distance;
                camUsed.fov = camComp->fov;
                camUsed.x = obj->transform().x;
                camUsed.y = obj->transform().y;
                camUsed.z = obj->transform().z;
                goto camera_determined;
            }
        }
    }
camera_determined:;

    int screen = MakeScreen(width, height, TRUE);
    if (screen == -1) return 0;
    int prev = GetDrawScreen();
    SetDrawScreen(screen);
    ClearDrawScreen();

    float radYaw = camUsed.yaw * 3.14159265f / 180.0f;
    float radPitch = camUsed.pitch * 3.14159265f / 180.0f;
    float cx = camUsed.x + cosf(radYaw) * cosf(radPitch) * camUsed.distance;
    float cy = camUsed.y + sinf(radPitch) * camUsed.distance;
    float cz = camUsed.z + sinf(radYaw) * cosf(radPitch) * camUsed.distance;
    SetCameraPositionAndTarget_UpVecY(VGet(cx, cy, cz), VGet(camUsed.x, camUsed.y, camUsed.z));

    // Update global lighting info from scene mainLight
    Lighting::SetMainDirectionalLight(mainLight.dirX, mainLight.dirY, mainLight.dirZ, mainLight.color, mainLight.intensity);

    for (auto& r : roots_) {
        if (r->IsPrefab()) continue; // never render prefab templates
        r->Render();
    }

    if (showGrid) {
        int gridSize = 1000;
        int step = 50;
        int half = gridSize / 2;
        for (int x = -half; x <= half; x += step) {
            DrawLine3D(VGet((float)x, 0.0f, (float)-half), VGet((float)x, 0.0f, (float)half), GetColor(120,120,120));
        }
        for (int z = -half; z <= half; z += step) {
            DrawLine3D(VGet((float)-half, 0.0f, (float)z), VGet((float)half, 0.0f, (float)z), GetColor(120,120,120));
        }
    }

    DrawCircle(width - 60, 40, 10, GetColor(255,245,200), TRUE);

    for (auto& r : roots_) {
        if (r->IsPrefab()) continue;
        for (auto& c : r->GetAllComponents()) {
            auto sr = std::dynamic_pointer_cast<SpriteRenderer>(c);
            if (sr && sr->handle_ != -1) {
                float ox = r->transform().x;
                float oz = r->transform().y; // using 2D y as z for simple mapping
                int sx = (int)(width/2 + (ox - camUsed.x));
                int sz = (int)(height/2 + (oz - camUsed.z));
                DrawBox(sx - 20, sz + 2, sx + 20, sz + 12, GetColor(0,0,0), TRUE);
            }
        }
    }

    SetDrawScreen(prev);
    return screen;
}

int Scene::RenderToTarget(int width, int height) {
    if (renderMode == RenderMode::Mode2D) return RenderToTarget(width, height, camera);
    return RenderToTarget3D(width, height, camera3D);
}

std::shared_ptr<GameObject> Scene::FindRootByName(const std::string& name) const {
    for (auto& r : roots_) {
        if (r->name() == name) return r;
    }
    return nullptr;
}
