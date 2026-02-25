#pragma once
#include <string>
#include <vector>

namespace UnityPackageImporter {
    // Import a .unitypackage file. Extracts archive (uses embedded miniz to gunzip + parse tar) into a temp folder,
    // scans for entries' "pathname" and "asset" files and copies FBX/OBJ assets into assetsDir.
    // Returns true on success. outImported receives relative filenames copied into assetsDir.
    bool ImportUnityPackage(const std::string& packagePath, const std::string& assetsDir, std::vector<std::string>& outImported);
}
