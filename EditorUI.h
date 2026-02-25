#pragma once
#include "DxLib.h"
#include <memory>
#include <vector>
#include <string>
#include "Scene.h"
#include "GameObject.h"
#include "UI.h"
#include "LightComponent.h"
#include "CameraComponent.h"
#include "Serializer.h"
#include "Collider.h"
#include "MeshRenderer.h"
#include "UnityPackageImporter.h"
#include "LabelComponent.h"
#include "third_party/ModelLoader.h"
#include "SkinnedMeshRenderer.h"
#include "EffekseerComponent.h"

namespace EditorUI {

static std::string OpenFileDialogUtf8(const wchar_t* filter = L"All\0*.*\0\0") {
    OPENFILENAMEW ofn;
    wchar_t szFile[MAX_PATH] = {0};
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = GetActiveWindow();
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    if (GetOpenFileNameW(&ofn)) {
        // convert wide-char to UTF-8
        int required = WideCharToMultiByte(CP_UTF8, 0, szFile, -1, nullptr, 0, nullptr, nullptr);
        if (required <= 0) return std::string();
        std::string out;
        out.resize(required - 1);
        WideCharToMultiByte(CP_UTF8, 0, szFile, -1, &out[0], required, nullptr, nullptr);
        return out;
    }
    return std::string();
}

inline void DrawEditor(Scene& scene, std::shared_ptr<GameObject> &selected, int viewportGraph, int screenW, int screenH) {
    int leftW = 220;
    int rightW = 300;
    int bottomH = 180;
    int topH = 28; // toolbar

    DrawBox(0, 0, screenW, screenH, GetColor(48, 50, 54), TRUE);
    DrawBox(0, 0, screenW, topH, GetColor(33, 37, 43), TRUE);
    DrawString(8, 6, "Scene  |  Project  |  Debug  |  Editor", GetColor(200,200,200));

    int hierW = 200;
    int hierX = 8; int hierY = topH + 8;
    UI::BeginPanel("Hierarchy", hierX, hierY, hierW, screenH - topH - bottomH - 16);
    int hy = hierY + 28;
    int hid = 0;
    for (auto& obj : scene.GetRoots()) {
        int tx = hierX + 8;
        int ty = hy + hid * 20;
        DrawFormatString(tx, ty, GetColor(200,200,200), "%s", obj->name().c_str());
        int mx2, my2; GetMousePoint(&mx2, &my2);
        if (mx2 >= tx && mx2 <= tx + 160 && my2 >= ty && my2 <= ty + 16 && (GetMouseInput() & MOUSE_INPUT_LEFT) != 0) {
            selected = obj;
        }
        hid++;
    }

    int prefabStartY = hy + hid * 20 + 8;
    DrawFormatString(hierX + 8, prefabStartY, GetColor(180,180,180), "Prefabs:");
    int pidx = 0;
    for (auto& pf : scene.GetPrefabs()) {
        int px = hierX + 8;
        int py = prefabStartY + 18 + pidx * 22;
        DrawFormatString(px, py, GetColor(200,200,200), "%s", pf->name().c_str());
        if (UI::Button(px + 120, py - 2, 64, 20, "Inst")) {
            auto inst = scene.Instantiate(pf);
            inst->transform().x = 0;
            inst->transform().y = 0;
            inst->transform().z = 0;
        }
        pidx++;
    }

    int saveBtnY = prefabStartY + 18 + pidx * 22 + 8;
    if (UI::Button(hierX + 8, saveBtnY, 180, 22, "Save Selected as Prefab")) {
        if (selected) {
            std::string filename = std::string("prefabs/") + selected->name() + ".prefab";
            Serializer::SavePrefab(filename, selected);
            auto clone = selected->Clone();
            clone->SetPrefab(true);
            scene.AddPrefab(clone);
        }
    }

    // Import Prefab button -> show file dialog and load prefab
    int importBtnY = saveBtnY + 28;
    if (UI::Button(hierX + 8, importBtnY, 180, 22, "Import Prefab...")) {
        std::string path = OpenFileDialogUtf8(L"Prefab Files\0*.prefab\0All Files\0*.*\0\0");
        if (!path.empty()) {
            auto pf = Serializer::LoadPrefab(path);
            if (pf) {
                pf->SetPrefab(true);
                scene.AddPrefab(pf);
            }
        }
    }

    UI::EndPanel();

    int viewX = hierW + 32;
    int viewY = topH + 8;
    int viewW = screenW - hierW - rightW - 56;
    int viewH = screenH - topH - bottomH - 16;

    DrawBox(viewX-2, viewY-2, viewX + viewW + 2, viewY + viewH + 2, GetColor(20,20,20), TRUE);
    DrawBox(viewX, viewY, viewX + viewW, viewY + viewH, GetColor(60,66,72), TRUE);
    DrawFormatString(viewX + 6, viewY + 6, GetColor(220,220,220), "Perspective (%s)", scene.renderMode == Scene::RenderMode::Mode2D ? "2D" : "3D");

    if (viewportGraph != 0) {
        DrawGraph(viewX, viewY, viewportGraph, TRUE);
    }

    // Project panel: bottom-left area
    int projX = 8;
    int projY = screenH - bottomH + 36;
    int projW = 260;
    int projH = bottomH - 48;
    UI::BeginPanel("Project", projX, projY, projW, projH);

    static std::vector<std::string> assets;
    static int assetSelected = -1;
    static int lastClickTime = 0;

    // scan Assets folder using Win32 API
    assets.clear();
    WIN32_FIND_DATAA fd;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    CreateDirectoryA("Assets", NULL);
    std::string search = std::string("Assets\\*");
    hFind = FindFirstFileA(search.c_str(), &fd);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (strcmp(fd.cFileName, ".") != 0 && strcmp(fd.cFileName, "..") != 0) {
                assets.push_back(fd.cFileName);
            }
        } while (FindNextFileA(hFind, &fd) != 0);
        FindClose(hFind);
    }

    int ay = projY + 8;
    int idx = 0;
    for (auto& a : assets) {
        DrawFormatString(projX + 8, ay + idx * 18, GetColor(200,200,200), "%s", a.c_str());
        int mx2, my2; GetMousePoint(&mx2, &my2);
        if (mx2 >= projX + 8 && mx2 <= projX + projW - 8 && my2 >= ay + idx*18 && my2 <= ay + idx*18 + 16 && (GetMouseInput() & MOUSE_INPUT_LEFT) != 0) {
            int now = GetNowCount();
            if (assetSelected == idx && now - lastClickTime < 300) {
                // double click: if .obj then instantiate
                std::string name = assets[idx];
                std::string full = std::string("Assets/") + name;
                if (full.size() >= 4) {
                    std::string ext = full.substr(full.size()-4);
                    if (ext == ".obj" || ext == ".OBJ") {
                        auto go = std::make_shared<GameObject>(name);
                        go->AddComponent<MeshRenderer>(full);
                        scene.AddRootObject(go);
                    } else if (ext == ".fbx" || ext == ".FBX") {
                        auto go = std::make_shared<GameObject>(name);
                        // inspect model to decide skinned or static
                        auto mesh = ModelLoader::LoadModel(full);
                        if (mesh && (!mesh->boneNames.empty() || !mesh->animations.empty())) {
                            go->AddComponent<SkinnedMeshRenderer>(full);
                            go->AddComponent<LabelComponent>(name + " (Skinned)");
                        } else {
                            go->AddComponent<MeshRenderer>(full);
                            go->AddComponent<LabelComponent>(name + " (FBX)");
                        }
                        scene.AddRootObject(go);
                    } else if (ext == ".efk" || ext == ".EFK") {
                        // Effekseer effect file: create GameObject with EffekseerComponent if available
                        auto go = std::make_shared<GameObject>(name);
                        // Prefer to add EffekseerComponent so effect can be played if library present
                        go->AddComponent<EffekseerComponent>(full);
                        scene.AddRootObject(go);
                    }
                }
            }
            assetSelected = idx;
            lastClickTime = now;
        }
        idx++;
    }

    int impY = projY + projH - 28;
    if (UI::Button(projX + 8, impY, 140, 22, "Import Asset...")) {
        std::string path = OpenFileDialogUtf8(L"Asset Files\0*.obj;*.fbx;*.efk\0All Files\0*.*\0\0");
        if (!path.empty()) {
            // copy into Assets folder using Win32 wide-char functions
            int req = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, NULL, 0);
            if (req > 0) {
                std::wstring wpath; wpath.resize(req);
                MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, &wpath[0], req);
                wchar_t fname[MAX_PATH];
                _wsplitpath_s(wpath.c_str(), NULL, 0, NULL, 0, fname, MAX_PATH, NULL, 0);
                std::wstring wdest = std::wstring(L"Assets\\") + fname;
                CopyFileW(wpath.c_str(), wdest.c_str(), FALSE);
                // attempt to load as sequence
                // build UTF-8 dest filename
                int fnReq = WideCharToMultiByte(CP_UTF8, 0, fname, -1, nullptr, 0, nullptr, nullptr);
                std::string fnUtf8;
                if (fnReq > 0) { fnUtf8.resize(fnReq - 1); WideCharToMultiByte(CP_UTF8, 0, fname, -1, &fnUtf8[0], fnReq, nullptr, nullptr); }
                std::string example = std::string("Assets/") + fnUtf8;
                auto seq = ObjSequenceLoader::LoadSequence(example);
                if (seq) {
                    auto go = std::make_shared<GameObject>(fnUtf8);
                    auto smr = std::make_shared<SkinnedMeshRenderer>();
                    smr->SetMorphSequence(seq);
                    go->AddComponent<SkinnedMeshRenderer>(*smr);
                    scene.AddRootObject(go);
                } else {
                    // If the imported file is an Effekseer (.efk) file, just add as asset and create placeholder
                    if (fnUtf8.size() >= 4) {
                        std::string ext = fnUtf8.substr(fnUtf8.size()-4);
                        if (ext == ".efk" || ext == ".EFK") {
                            auto go = std::make_shared<GameObject>(fnUtf8);
                            go->AddComponent<EffekseerComponent>(std::string("Assets/") + fnUtf8);
                            scene.AddRootObject(go);
                        }
                    }
                }
            }
        }
    }

    // Import UnityPackage button
    if (UI::Button(projX + 152, impY, 140, 22, "Import UnityPackage...")) {
        std::string pkg = OpenFileDialogUtf8(L"UnityPackage\0*.unitypackage\0All Files\0*.*\0\0");
        if (!pkg.empty()) {
            std::vector<std::string> imported;
            bool ok = UnityPackageImporter::ImportUnityPackage(pkg, "Assets", imported);
            if (ok) {
                // instantiate imported assets where sensible
                for (auto &fn : imported) {
                    std::string full = std::string("Assets/") + fn;
                    if (fn.size() >= 4) {
                        std::string ext = fn.substr(fn.size()-4);
                        if (ext == ".obj" || ext == ".OBJ") {
                            auto go = std::make_shared<GameObject>(fn);
                            go->AddComponent<MeshRenderer>(full);
                            scene.AddRootObject(go);
                        } else if (ext == ".fbx" || ext == ".FBX") {
                            auto go = std::make_shared<GameObject>(fn);
                            auto mesh = ModelLoader::LoadModel(full);
                            if (mesh && (!mesh->boneNames.empty() || !mesh->animations.empty())) {
                                go->AddComponent<SkinnedMeshRenderer>(full);
                                go->AddComponent<LabelComponent>(fn + " (Skinned)");
                            } else {
                                go->AddComponent<MeshRenderer>(full);
                                go->AddComponent<LabelComponent>(fn + " (FBX)");
                            }
                            scene.AddRootObject(go);
                        } else if (ext == ".efk" || ext == ".EFK") {
                            auto go = std::make_shared<GameObject>(fn);
                            go->AddComponent<EffekseerComponent>(std::string("Assets/") + fn);
                            scene.AddRootObject(go);
                        }
                    }
                }
            }
        }
    }

    UI::EndPanel();

    static bool dragging = false;
    static std::shared_ptr<GameObject> dragObj = nullptr;
    static float dragOffsetX = 0.0f, dragOffsetY = 0.0f;
    static int prevMouse = 0;

    int mx, my; GetMousePoint(&mx, &my);
    int mouseNow = GetMouseInput();
    bool leftNow = (mouseNow & MOUSE_INPUT_LEFT) != 0;
    bool leftPrev = (prevMouse & MOUSE_INPUT_LEFT) != 0;

    bool mouseInViewport = (mx >= viewX && mx <= viewX + viewW && my >= viewY && my <= viewY + viewH);

    if (scene.renderMode == Scene::RenderMode::Mode2D) {
        float worldX = (float)(mx - viewX - viewW/2) / scene.camera.zoom + scene.camera.x;
        float worldY = (float)(my - viewY - viewH/2) / scene.camera.zoom + scene.camera.y;

        if (mouseInViewport && leftNow && !leftPrev && !dragging) {
            for (auto it = scene.GetRoots().rbegin(); it != scene.GetRoots().rend(); ++it) {
                auto obj = *it;
                bool hit = false;
                for (auto& c : obj->GetAllComponents()) {
                    auto colPtr = std::dynamic_pointer_cast<Collider>(c);
                    if (colPtr) {
                        float ox = obj->transform().x;
                        float oy = obj->transform().y;
                        float hw = colPtr->width * 0.5f;
                        float hh = colPtr->height * 0.5f;
                        if (worldX >= ox - hw && worldX <= ox + hw && worldY >= oy - hh && worldY <= oy + hh) { hit = true; break; }
                    }
                }
                if (!hit) {
                    float dx = obj->transform().x - worldX;
                    float dy = obj->transform().y - worldY;
                    if (dx*dx + dy*dy < 16*16) hit = true;
                }
                if (hit) {
                    dragging = true;
                    dragObj = obj;
                    dragOffsetX = obj->transform().x - worldX;
                    dragOffsetY = obj->transform().y - worldY;
                    selected = obj;
                    break;
                }
            }
        }

        if (dragging && dragObj) {
            if (leftNow) {
                dragObj->transform().x = worldX + dragOffsetX;
                dragObj->transform().y = worldY + dragOffsetY;
            } else {
                dragging = false;
                dragObj.reset();
            }
        }
    }

    prevMouse = mouseNow;

    if (selected) {
        float wx = selected->transform().x;
        float wy = selected->transform().y;
        int gx = viewX + (int)((wx - scene.camera.x) * scene.camera.zoom + viewW/2.0f);
        int gy = viewY + (int)((wy - scene.camera.y) * scene.camera.zoom + viewH/2.0f);
        if (gx < viewX) gx = viewX + viewW/2;
        if (gy < viewY) gy = viewY + viewH/2;
        DrawLine(gx, gy, gx + 40, gy, GetColor(255,0,0));
        DrawLine(gx, gy, gx, gy - 40, GetColor(0,255,0));
        DrawBox(gx + 36, gy - 6, gx + 44, gy + 6, GetColor(255,0,0), TRUE);
        DrawBox(gx - 6, gy - 44, gx + 6, gy - 36, GetColor(0,255,0), TRUE);
        DrawCircle(gx, gy, 28, GetColor(180,180,255), FALSE);
    }

    int bottomY = screenH - bottomH;
    DrawBox(0, bottomY, screenW - rightW - 8, screenH, GetColor(33,37,43), TRUE);
    DrawFormatString(8, bottomY + 6, GetColor(180,180,180), "File");
    int codeX = 8;
    int codeY = bottomY + 28;
    DrawBox(codeX, codeY, screenW - rightW - 16, screenH - 8, GetColor(20,22,24), TRUE);
    const char* lines[] = { "shader_type spatial;", "render_mode cull_disabled;", "", "uniform sampler2D tex_alb;", "uniform vec4 albedo : source_color;", "uniform float sway_amt = 32;" };
    for (int i = 0; i < 6; ++i) DrawString(codeX + 8, codeY + 8 + i * 18, lines[i], GetColor(160,200,240));

    int rightX = screenW - rightW + 8;
    int rightY = topH + 8;
    int rightH = screenH - topH - bottomH - 16;
    UI::BeginPanel("Inspector", rightX, rightY, rightW - 16, rightH);

    int tabX = rightX + 8;
    int tabY = rightY + 4;
    static int activeTab = 1; // 0=Scene,1=Inspector,2=Light,3=Camera
    if (UI::Button(tabX, tabY, 60, 20, "Scene")) activeTab = 0; tabX += 64;
    if (UI::Button(tabX, tabY, 80, 20, "Inspector")) activeTab = 1; tabX += 84;
    if (UI::Button(tabX, tabY, 60, 20, "Light")) activeTab = 2; tabX += 64;
    if (UI::Button(tabX, tabY, 80, 20, "Camera")) activeTab = 3; tabX += 84;

    if (activeTab == 0) {
        int sy = rightY + 36;
        DrawFormatString(rightX + 8, sy, GetColor(200,200,200), "Scene Settings");
        sy += 18;
        std::string gridLabel = scene.showGrid ? "Grid: On" : "Grid: Off";
        if (UI::Button(rightX + 8, sy, 120, 22, gridLabel)) {
            scene.showGrid = !scene.showGrid;
        }
        sy += 28;
        DrawFormatString(rightX + 8, sy, GetColor(180,180,180), "Camera (3D)");
        sy += 18;
        float yaw = scene.camera3D.yaw;
        float pitch = scene.camera3D.pitch;
        float dist = scene.camera3D.distance;
        float fov = scene.camera3D.fov;
        if (UI::SliderFloat(rightX + 8, sy, 200, "Yaw", yaw, -180.0f, 180.0f)) scene.camera3D.yaw = yaw;
        sy += 22;
        if (UI::SliderFloat(rightX + 8, sy, 200, "Pitch", pitch, -89.0f, 89.0f)) scene.camera3D.pitch = pitch;
        sy += 22;
        if (UI::SliderFloat(rightX + 8, sy, 200, "Distance", dist, 10.0f, 2000.0f)) scene.camera3D.distance = dist;
        sy += 22;
        if (UI::SliderFloat(rightX + 8, sy, 200, "FOV", fov, 10.0f, 120.0f)) scene.camera3D.fov = fov;
        sy += 28;

        DrawFormatString(rightX + 8, sy, GetColor(180,180,180), "Directional Light");
        sy += 18;
        float lx = scene.mainLight.dirX;
        float ly = scene.mainLight.dirY;
        float lz = scene.mainLight.dirZ;
        if (UI::SliderFloat(rightX + 8, sy, 200, "Dir X", lx, -1.0f, 1.0f)) scene.mainLight.dirX = lx;
        sy += 22;
        if (UI::SliderFloat(rightX + 8, sy, 200, "Dir Y", ly, -1.0f, 1.0f)) scene.mainLight.dirY = ly;
        sy += 22;
        if (UI::SliderFloat(rightX + 8, sy, 200, "Dir Z", lz, -1.0f, 1.0f)) scene.mainLight.dirZ = lz;
        sy += 22;
        float lint = scene.mainLight.intensity;
        if (UI::SliderFloat(rightX + 8, sy, 200, "Intensity", lint, 0.0f, 8.0f)) scene.mainLight.intensity = lint;
        sy += 26;
        int r = (scene.mainLight.color >> 16) & 0xFF;
        int g = (scene.mainLight.color >> 8) & 0xFF;
        int b = (scene.mainLight.color) & 0xFF;
        if (UI::ColorPicker(rightX + 8, sy, 260, 80, r, g, b)) {
            scene.mainLight.color = (r << 16) | (g << 8) | b;
        }
        sy += 88;

    } else if (activeTab == 1) {
        int selY = rightY + 36;
        if (selected) {
            DrawFormatString(rightX + 8, selY, GetColor(200,200,200), "Name: %s", selected->name().c_str());
            selY += 20;
            DrawFormatString(rightX + 8, selY, GetColor(180,180,180), "Position: %.1f, %.1f", selected->transform().x, selected->transform().y);
            selY += 24;
            int ci = 0;
            for (auto& c : selected->GetAllComponents()) {
                DrawFormatString(rightX + 8, selY + ci*22, GetColor(200,200,200), "%d: Component", ci);
                if (UI::Button(rightX + 160, selY + ci*22, 60, 18, c->enabled ? "On" : "Off")) {
                    c->enabled = !c->enabled;
                }
                ci++;
            }
        }
    } else if (activeTab == 2) {
        int ly2 = rightY + 36; int li = 0;
        for (auto& obj : scene.GetRoots()) {
            for (auto& c : obj->GetAllComponents()) {
                auto lc = std::dynamic_pointer_cast<LightComponent>(c);
                if (lc) {
                    DrawFormatString(rightX + 8, ly2 + li*26, GetColor(200,200,200), "%s: Light", obj->name().c_str());
                    if (UI::Button(rightX + 160, ly2 + li*26, 120, 20, "Select")) selected = obj;
                    li++;
                }
            }
        }
    } else if (activeTab == 3) {
        int cy2 = rightY + 36; int ci2 = 0;
        for (auto& obj : scene.GetRoots()) {
            for (auto& c : obj->GetAllComponents()) {
                auto cc = std::dynamic_pointer_cast<CameraComponent>(c);
                if (cc) {
                    DrawFormatString(rightX + 8, cy2 + ci2*26, GetColor(200,200,200), "%s: Camera", obj->name().c_str());
                    if (UI::Button(rightX + 160, cy2 + ci2*26, 120, 20, "Select")) selected = obj;
                    ci2++;
                }
            }
        }
    }

    UI::EndPanel();
}

} // namespace EditorUI
