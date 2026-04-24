#pragma once
#include <string>
#include <vector>
#include <memory>

class GameObject;

namespace Serializer {
    bool SaveScene(const std::string& path, const std::vector<std::shared_ptr<GameObject>>& roots);
    bool LoadScene(const std::string& path, std::vector<std::shared_ptr<GameObject>>& outRoots);

    // Prefab save/load for editor: store a single GameObject template
    bool SavePrefab(const std::string& path, std::shared_ptr<GameObject> prefab);
    std::shared_ptr<GameObject> LoadPrefab(const std::string& path);

    // Save/load simple override diffs (keyed by prefab filename). Format is minimal and textual.
    bool SavePrefabOverrides(const std::string& path, const std::vector<std::string>& overrides);
    bool LoadPrefabOverrides(const std::string& path, std::vector<std::string>& outOverrides);
}
