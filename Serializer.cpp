#include "Serializer.h"
#include "GameObject.h"
#include "Component.h"
#include "LabelComponent.h"
#include "SpriteRenderer.h"
#include "Collider.h"
#include <fstream>
#include <iostream>

// 注意: 簡易実装。サポートするコンポーネントのみ保存/復元する。
// フォーマットは非常に単純: オブジェクトごとに名前, x,y, 各コンポーネントの型とデータ

bool Serializer::SaveScene(const std::string& path, const std::vector<std::shared_ptr<GameObject>>& roots) {
    std::ofstream ofs(path);
    if (!ofs) return false;
    for (auto& r : roots) {
        ofs << "OBJ\n";
        ofs << "NAME:" << r->name() << "\n";
        ofs << "POS:" << r->transform().x << "," << r->transform().y << "\n";
        for (auto& c : r->GetAllComponents()) {
            if (auto sc = std::dynamic_pointer_cast<SpriteRenderer>(c)) {
                ofs << "SPRITE:" << sc->path_ << "\n";
            } else if (auto lb = std::dynamic_pointer_cast<LabelComponent>(c)) {
                ofs << "LABEL:" << lb->text << "\n";
            } else if (auto col = std::dynamic_pointer_cast<Collider>(c)) {
                ofs << "COL:" << col->width << "," << col->height << "\n";
            }
        }
        ofs << "ENDOBJ\n";
    }
    return true;
}

bool Serializer::LoadScene(const std::string& path, std::vector<std::shared_ptr<GameObject>>& outRoots) {
    std::ifstream ifs(path);
    if (!ifs) return false;
    std::string line;
    std::shared_ptr<GameObject> current;
    while (std::getline(ifs, line)) {
        if (line == "OBJ") {
            current = std::make_shared<GameObject>("Loaded");
            outRoots.push_back(current);
        } else if (line.rfind("NAME:", 0) == 0) {
            if (!outRoots.empty()) {
                current = outRoots.back();
                std::string name = line.substr(5);
                current->SetName(name);
            }
        } else if (line.rfind("POS:", 0) == 0) {
            auto s = line.substr(4);
            float x = 0, y = 0;
            if (sscanf_s(s.c_str(), "%f,%f", &x, &y) == 2) {
                current->transform().x = x;
                current->transform().y = y;
            }
        } else if (line.rfind("SPRITE:", 0) == 0) {
            auto p = line.substr(7);
            current->AddComponent<SpriteRenderer>(p);
        } else if (line.rfind("LABEL:", 0) == 0) {
            auto t = line.substr(6);
            current->AddComponent<LabelComponent>(t);
        } else if (line.rfind("COL:", 0) == 0) {
            float w=0,h=0;
            if (sscanf_s(line.c_str()+4, "%f,%f", &w, &h) == 2) {
                current->AddComponent<Collider>(w,h);
            }
        } else if (line == "ENDOBJ") {
            if (current) current->Awake();
        }
    }
    return true;
}

bool Serializer::SavePrefab(const std::string& path, std::shared_ptr<GameObject> prefab) {
    if (!prefab) return false;
    std::ofstream ofs(path);
    if (!ofs) return false;
    ofs << "PREFAB\n";
    ofs << "NAME:" << prefab->name() << "\n";
    ofs << "POS:" << prefab->transform().x << "," << prefab->transform().y << "\n";
    for (auto& c : prefab->GetAllComponents()) {
        if (auto sc = std::dynamic_pointer_cast<SpriteRenderer>(c)) {
            ofs << "SPRITE:" << sc->path_ << "\n";
        } else if (auto lb = std::dynamic_pointer_cast<LabelComponent>(c)) {
            ofs << "LABEL:" << lb->text << "\n";
        } else if (auto col = std::dynamic_pointer_cast<Collider>(c)) {
            ofs << "COL:" << col->width << "," << col->height << "\n";
        }
    }
    ofs << "ENDPREFAB\n";
    return true;
}

std::shared_ptr<GameObject> Serializer::LoadPrefab(const std::string& path) {
    std::ifstream ifs(path);
    if (!ifs) return nullptr;
    std::string line;
    std::shared_ptr<GameObject> current = nullptr;
    while (std::getline(ifs, line)) {
        if (line == "PREFAB") {
            current = std::make_shared<GameObject>("Prefab");
        } else if (line.rfind("NAME:", 0) == 0) {
            if (current) current->SetName(line.substr(5));
        } else if (line.rfind("POS:", 0) == 0) {
            if (current) {
                float x=0,y=0;
                if (sscanf_s(line.c_str()+4, "%f,%f", &x, &y) == 2) {
                    current->transform().x = x;
                    current->transform().y = y;
                }
            }
        } else if (line.rfind("SPRITE:", 0) == 0) {
            if (current) current->AddComponent<SpriteRenderer>(line.substr(7));
        } else if (line.rfind("LABEL:", 0) == 0) {
            if (current) current->AddComponent<LabelComponent>(line.substr(6));
        } else if (line.rfind("COL:", 0) == 0) {
            if (current) {
                float w=0,h=0;
                if (sscanf_s(line.c_str()+4, "%f,%f", &w, &h) == 2) {
                    current->AddComponent<Collider>(w,h);
                }
            }
        } else if (line == "ENDPREFAB") {
            if (current) current->Awake();
        }
    }
    if (current) current->SetPrefab(true);
    return current;
}
