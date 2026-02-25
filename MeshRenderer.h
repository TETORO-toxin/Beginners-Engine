#pragma once
#include "Component.h"
#include "DxLib.h"
#include "Mesh.h"
#include <memory>
#include <string>
#include <algorithm>
#include <cmath>
#include "ObjLoader.h"
#include "third_party/ModelLoader.h"
#include "Lighting.h"
#include "Shader.h"

// MeshRenderer: render a loaded mesh (wireframe) or fallback cube
struct MeshRenderer : public Component {
    MeshRenderer() : color_(GetColor(200,200,200)), meshPath_(), mesh_(nullptr) { InitShader(); }
    MeshRenderer(int color) : color_(color), meshPath_(), mesh_(nullptr) { InitShader(); }
    MeshRenderer(const std::string& meshPath) : color_(GetColor(200,200,200)), meshPath_(meshPath), mesh_(nullptr) { InitShader(); }

    void InitShader() {
        if (!shader_) shader_ = std::make_shared<Shader>();
        // attempt to load simple shader (no-op succeeds)
        shader_->Load("shaders/simple_lit.vert", "shaders/simple_lit.hlsl");
    }

    void Awake() override {
        if (!meshPath_.empty()) {
            // choose loader based on extension
            if (meshPath_.size() >= 4) {
                std::string ext = meshPath_.substr(meshPath_.size() - 4);
                for (auto &c : ext) c = (char)tolower((unsigned char)c);
                if (ext == ".obj") {
                    mesh_ = ObjLoader::LoadObj(meshPath_);
                } else {
                    mesh_ = ModelLoader::LoadModel(meshPath_);
                }
            } else {
                mesh_ = ObjLoader::LoadObj(meshPath_);
            }
        }
    }

    void Render() override {
        if (!owner) return;
        // compute simple light factor
        VECTOR ldir = Lighting::GetMainDirectionalDir();
        float lint = Lighting::GetMainDirectionalIntensity();
        int lcol = Lighting::GetMainDirectionalColor();

        // bind shader and set uniforms (no-op on current platform)
        if (shader_) {
            shader_->Bind();
            shader_->SetFloat3("LightDir", -ldir.x, -ldir.y, -ldir.z);
            // unpack color
            int lr = (lcol >> 16) & 0xFF;
            int lg = (lcol >> 8) & 0xFF;
            int lb = lcol & 0xFF;
            shader_->SetFloat3("LightColor", lr / 255.0f, lg / 255.0f, lb / 255.0f);
            shader_->SetFloat("LightIntensity", lint);
        }

        auto clamp255 = [](int v) {
            if (v < 0) return 0;
            if (v > 255) return 255;
            return v;
        };

        auto ApplyLightingToColor = [&](int baseColor, const VECTOR& normal) {
            int br = (baseColor >> 16) & 0xFF;
            int bg = (baseColor >> 8) & 0xFF;
            int bb = baseColor & 0xFF;
            int lr = (lcol >> 16) & 0xFF;
            int lg = (lcol >> 8) & 0xFF;
            int lb = lcol & 0xFF;
            float ndotl = 0.0f;
            float nl = sqrtf(normal.x*normal.x + normal.y*normal.y + normal.z*normal.z);
            if (nl > 0.0001f) {
                VECTOR nn = VGet(normal.x / nl, normal.y / nl, normal.z / nl);
                ndotl = nn.x * (-ldir.x) + nn.y * (-ldir.y) + nn.z * (-ldir.z);
                if (ndotl < 0.0f) ndotl = 0.0f;
            }
            float factor = lint * ndotl;
            float rVal = br * (1.0f - factor) + (br * (lr / 255.0f)) * factor;
            float gVal = bg * (1.0f - factor) + (bg * (lg / 255.0f)) * factor;
            float bVal = bb * (1.0f - factor) + (bb * (lb / 255.0f)) * factor;
            int rr = clamp255((int)rVal);
            int gg = clamp255((int)gVal);
            int bb2 = clamp255((int)bVal);
            return GetColor(rr, gg, bb2);
        };

        if (mesh_) {
            // simple transform: scale then translate per-vertex
            float tx = owner->transform().x;
            float ty = owner->transform().y;
            float tz = owner->transform().z;
            float sx = owner->transform().scaleX;
            float sy = owner->transform().scaleY;
            float sz = owner->transform().scaleZ;
            // draw triangles
            for (size_t i = 0; i + 2 < mesh_->indices.size(); i += 3) {
                VECTOR v0 = mesh_->vertices[mesh_->indices[i + 0]];
                VECTOR v1 = mesh_->vertices[mesh_->indices[i + 1]];
                VECTOR v2 = mesh_->vertices[mesh_->indices[i + 2]];
                v0.x = v0.x * sx + tx; v0.y = v0.y * sy + ty; v0.z = v0.z * sz + tz;
                v1.x = v1.x * sx + tx; v1.y = v1.y * sy + ty; v1.z = v1.z * sz + tz;
                v2.x = v2.x * sx + tx; v2.y = v2.y * sy + ty; v2.z = v2.z * sz + tz;
                // compute normal if available
                VECTOR normal = VGet(0,1,0);
                if (mesh_->normals.size() == mesh_->vertices.size()) {
                    normal = mesh_->normals[mesh_->indices[i + 0]];
                } else {
                    VECTOR e1 = VSub(v1, v0);
                    VECTOR e2 = VSub(v2, v0);
                    normal = VCross(e1, e2);
                }
                int litColor = ApplyLightingToColor(color_, normal);
                DrawLine3D(v0, v1, litColor);
                DrawLine3D(v1, v2, litColor);
                DrawLine3D(v2, v0, litColor);
            }
            return;
        }
        // fallback: draw cube wireframe
        float wx, wy, wz;
        owner->transform().GetWorldPosition3D(wx, wy, wz);
        float sx = owner->transform().scaleX * 0.5f;
        float sy = owner->transform().scaleY * 0.5f;
        float sz = owner->transform().scaleZ * 0.5f;
        VECTOR v[8];
        v[0] = VGet(wx - sx, wy - sy, wz - sz);
        v[1] = VGet(wx + sx, wy - sy, wz - sz);
        v[2] = VGet(wx + sx, wy + sy, wz - sz);
        v[3] = VGet(wx - sx, wy + sy, wz - sz);
        v[4] = VGet(wx - sx, wy - sy, wz + sz);
        v[5] = VGet(wx + sx, wy - sy, wz + sz);
        v[6] = VGet(wx + sx, wy + sy, wz + sz);
        v[7] = VGet(wx - sx, wy + sy, wz + sz);
        // compute cube face normals and draw with lighting
        VECTOR normals[6] = { VGet(0,0,-1), VGet(0,0,1), VGet(0,-1,0), VGet(0,1,0), VGet(-1,0,0), VGet(1,0,0) };
        // draw edges using average face normal for each edge simplified
        for (int i = 0; i < 12; ++i) {
            int a, b; int fn;
            switch(i) {
                case 0: a=0;b=1;fn=0; break; case 1: a=1;b=2;fn=0; break; case 2: a=2;b=3;fn=0; break; case 3: a=3;b=0;fn=0; break;
                case 4: a=4;b=5;fn=1; break; case 5: a=5;b=6;fn=1; break; case 6: a=6;b=7;fn=1; break; case 7: a=7;b=4;fn=1; break;
                case 8: a=0;b=4;fn=2; break; case 9: a=1;b=5;fn=2; break; case 10: a=2;b=6;fn=2; break; case 11: a=3;b=7;fn=2; break;
            }
            int litColor = ApplyLightingToColor(color_, normals[fn]);
            DrawLine3D(v[a], v[b], litColor);
        }
    }

    std::shared_ptr<Component> Clone() const override {
        if (!meshPath_.empty()) return std::make_shared<MeshRenderer>(meshPath_);
        return std::make_shared<MeshRenderer>(color_);
    }

    int color_;
    std::string meshPath_;
    std::shared_ptr<Mesh> mesh_;
    std::shared_ptr<Shader> shader_;
};
