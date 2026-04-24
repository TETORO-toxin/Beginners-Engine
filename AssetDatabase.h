#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <thread>
#include <atomic>
#include <Windows.h>

// Simple Asset Database: watches/captures files under "Assets/" and writes/reads .meta files
// This is a minimal, synchronous implementation intended as a starting point for the engine.
class AssetDatabase {
public:
    struct Meta {
        std::string guid;
        long long lastWrite = 0; // simple timestamp
        std::string importSettings; // textual settings
        std::string type; // e.g. "model", "effect"
        std::vector<std::string> deps; // dependency GUIDs or paths
    };

    static AssetDatabase& Instance();

    // Initialize DB (creates folders if necessary)
    bool Init();

    // Scan Assets folder and update meta records. Returns number of assets discovered.
    int ScanAssets();

    // Return list of relative asset paths (e.g. "Assets/char.obj")
    std::vector<std::string> GetAllAssetPaths() const;

    // Start/stop watching Assets folder (simple polling watcher)
    void StartWatching();
    void StopWatching();

    // Import an external file into Assets (copies file). Returns relative filename on success, empty on failure.
    std::string ImportAsset(const std::string& srcPath);

    // Get meta by asset relative path (e.g. "char.obj"). Returns nullptr if not found.
    const Meta* GetMeta(const std::string& relativePath) const;

    // Get GUID by relative path (empty if not found)
    std::string GetGUID(const std::string& relativePath) const;

    // Dependency graph: guid -> vector<dependent guids>
    const std::unordered_map<std::string, std::vector<std::string>>& GetDependencyGraph() const { return deps_; }

private:
    AssetDatabase() = default;
    ~AssetDatabase() = default;

    std::string MakeMetaPath(const std::string& assetPath) const;
    bool LoadMeta(const std::string& metaPath, Meta& out);
    bool SaveMeta(const std::string& metaPath, const Meta& m);
    std::string GenerateGUID();

    std::unordered_map<std::string, Meta> metas_; // key = relative asset path
    std::unordered_map<std::string, std::string> guidToPath_;
    std::unordered_map<std::string, std::vector<std::string>> deps_;
    // watcher
    mutable std::mutex mutex_;
    std::atomic<bool> watching_{false};
    std::thread watchThread_;
    HANDLE dirHandle_ = INVALID_HANDLE_VALUE;
};
