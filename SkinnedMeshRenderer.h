#pragma once
#include "Component.h"
#include "Mesh.h"
#include "DxLib.h"
#include <memory>
#include <string>
#include <vector>
#include "ObjSequenceLoader.h"

// SkinnedMeshRenderer: performs CPU skinning and simple animation playback
struct SkinnedMeshRenderer : public Component {
    enum class Mode { None, Skinning, Morph };

    SkinnedMeshRenderer() : meshPath_(), mesh_(nullptr), currentAnim(-1), time(0.0), mode(Mode::None) {}
    SkinnedMeshRenderer(const std::string& path) : meshPath_(path), mesh_(nullptr), currentAnim(-1), time(0.0), mode(Mode::None) {}

    void Awake() override {
        if (!meshPath_.empty()) {
            // try ModelLoader first
            mesh_ = ModelLoader::LoadModel(meshPath_);
            if (!mesh_) {
                // maybe it's an OBJ sequence: not loaded here
            }
            if (mesh_ && (!mesh_->boneNames.empty() || !mesh_->animations.empty())) {
                mode = Mode::Skinning;
            }
        }
    }

    void SetMorphSequence(std::shared_ptr<ObjSequence> seq) { morphSeq_ = seq; if (seq) mode = Mode::Morph; }

    void Update() override {
        if (mode == Mode::Morph && morphSeq_) {
            if (morphSeq_->FrameCount() == 0) return;
            double fps = 30.0;
            double t = time * fps;
            int f0 = (int)floor(t);
            int f1 = f0 + 1;
            double alpha = t - f0;
            int fc = morphSeq_->FrameCount();
            f0 = ((f0 % fc) + fc) % fc;
            f1 = ((f1 % fc) + fc) % fc;
            morphVertices_.resize(morphSeq_->VertexCount());
            for (size_t i = 0; i < morphSeq_->VertexCount(); ++i) {
                VECTOR a = morphSeq_->frames[f0][i];
                VECTOR b = morphSeq_->frames[f1][i];
                morphVertices_[i].x = (float)(a.x * (1.0 - alpha) + b.x * alpha);
                morphVertices_[i].y = (float)(a.y * (1.0 - alpha) + b.y * alpha);
                morphVertices_[i].z = (float)(a.z * (1.0 - alpha) + b.z * alpha);
            }
            time += Time::deltaTime;
        }
        // skinning mode left as-is (placeholder)
    }

    void Render() override {
        if (!owner) return;
        const auto* vertsPtr = (mode == Mode::Morph && !morphVertices_.empty()) ? &morphVertices_ : nullptr;
        if (mode == Mode::Skinning) {
            // not implemented fully here
            vertsPtr = &mesh_->vertices;
        }
        if (!vertsPtr) return;
        const auto& verts = *vertsPtr;
        float tx = owner->transform().x;
        float ty = owner->transform().y;
        float tz = owner->transform().z;
        float sx = owner->transform().scaleX;
        float sy = owner->transform().scaleY;
        float sz = owner->transform().scaleZ;
        for (size_t i = 0; i + 2 < morphSeq_->indices.size(); i += 3) {
            VECTOR v0 = verts[morphSeq_->indices[i + 0]];
            VECTOR v1 = verts[morphSeq_->indices[i + 1]];
            VECTOR v2 = verts[morphSeq_->indices[i + 2]];
            v0.x = v0.x * sx + tx; v0.y = v0.y * sy + ty; v0.z = v0.z * sz + tz;
            v1.x = v1.x * sx + tx; v1.y = v1.y * sy + ty; v1.z = v1.z * sz + tz;
            v2.x = v2.x * sx + tx; v2.y = v2.y * sy + ty; v2.z = v2.z * sz + tz;
            DrawLine3D(v0, v1, GetColor(200,200,200));
            DrawLine3D(v1, v2, GetColor(200,200,200));
            DrawLine3D(v2, v0, GetColor(200,200,200));
        }
    }

    void PlayMorph(float startTime = 0.0f) { time = startTime; }

    std::string meshPath_;
    std::shared_ptr<Mesh> mesh_;
    int currentAnim;
    double time;
    Mode mode;

    std::shared_ptr<ObjSequence> morphSeq_;
    std::vector<VECTOR> morphVertices_;
};
