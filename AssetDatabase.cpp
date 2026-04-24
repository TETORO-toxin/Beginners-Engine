#include "AssetDatabase.h"
#include <Windows.h>
#include <shlwapi.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <random>
#include <mutex>
#include <thread>
#include <chrono>

#pragma comment(lib, "Shlwapi.lib")

static std::string Utf16ToUtf8(const std::wstring& w) {
    int req = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, NULL, 0, NULL, NULL);
    if (req <= 0) return std::string();
    std::string out; out.resize(req - 1);
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, &out[0], req, NULL, NULL);
    return out;
}
static std::wstring Utf8ToUtf16(const std::string& s) {
    int req = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, NULL, 0);
    if (req <= 0) return std::wstring();
    std::wstring out; out.resize(req - 1);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &out[0], req);
    return out;
}

AssetDatabase& AssetDatabase::Instance() {
    static AssetDatabase inst;
    return inst;
}

bool AssetDatabase::Init() {
    // create folders if missing
    CreateDirectoryW(L"Assets", NULL);
    CreateDirectoryW(L"Library", NULL);
    // scan once
    ScanAssets();
    return true;
}

std::string AssetDatabase::MakeMetaPath(const std::string& assetPath) const {
    // e.g. Assets/foo.obj -> Assets/foo.obj.meta
    return assetPath + ".meta";
}

bool AssetDatabase::LoadMeta(const std::string& metaPath, Meta& out) {
    std::ifstream ifs(metaPath);
    if (!ifs) return false;
    std::string line;
    while (std::getline(ifs, line)) {
        if (line.find("guid=") == 0) out.guid = line.substr(5);
        else if (line.find("lastWrite=") == 0) out.lastWrite = std::stoll(line.substr(10));
        else if (line.find("import=") == 0) out.importSettings = line.substr(7);
        else if (line.find("type=") == 0) out.type = line.substr(5);
        else if (line.find("deps=") == 0) {
            out.deps.clear();
            std::string rest = line.substr(5);
            std::istringstream ss(rest);
            std::string token;
            while (std::getline(ss, token, ';')) {
                if (!token.empty()) out.deps.push_back(token);
            }
        }
    }
    return !out.guid.empty();
}

bool AssetDatabase::SaveMeta(const std::string& metaPath, const Meta& m) {
    std::ofstream ofs(metaPath, std::ios::trunc);
    if (!ofs) return false;
    ofs << "guid=" << m.guid << "\n";
    ofs << "lastWrite=" << m.lastWrite << "\n";
    ofs << "import=" << m.importSettings << "\n";
    ofs << "type=" << m.type << "\n";
    ofs << "deps=";
    for (size_t i = 0; i < m.deps.size(); ++i) {
        if (i) ofs << ";";
        ofs << m.deps[i];
    }
    ofs << "\n";
    return true;
}

std::string AssetDatabase::GenerateGUID() {
    // simple random hex GUID
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis;
    uint64_t a = dis(gen);
    uint64_t b = dis(gen);
    std::ostringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(16) << a << std::setw(16) << b;
    return ss.str();
}

int AssetDatabase::ScanAssets() {
    std::lock_guard<std::mutex> lk(mutex_);
    metas_.clear(); guidToPath_.clear(); deps_.clear();

    // First, enumerate assets (non-.meta files) and collect lastWrite times
    WIN32_FIND_DATAW fd;
    std::wstring searchAll = L"Assets\\*";
    HANDLE hFindAll = FindFirstFileW(searchAll.c_str(), &fd);
    if (hFindAll == INVALID_HANDLE_VALUE) return 0;

    struct FileEntry { std::wstring name; uint64_t lastWrite; };
    std::vector<FileEntry> files;

    do {
        if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0) continue;
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        std::wstring wfn(fd.cFileName);
        // skip .meta files here
        std::wstring lower = wfn;
        for (auto &c : lower) c = towlower(c);
        if (lower.size() >= 5 && lower.substr(lower.size()-5) == L".meta") continue;

        // get last write time
        WIN32_FILE_ATTRIBUTE_DATA fad;
        uint64_t lw = 0;
        if (GetFileAttributesExW((L"Assets\\" + wfn).c_str(), GetFileExInfoStandard, &fad)) {
            ULARGE_INTEGER li; li.HighPart = fad.ftLastWriteTime.dwHighDateTime; li.LowPart = fad.ftLastWriteTime.dwLowDateTime;
            lw = (uint64_t)li.QuadPart;
        }
        files.push_back({wfn, lw});
    } while (FindNextFileW(hFindAll, &fd));
    FindClose(hFindAll);

    // Next, enumerate .meta files and build orphan map keyed by lastWrite
    std::unordered_map<uint64_t, std::wstring> orphanMetaByTime;
    std::unordered_map<std::wstring, Meta> metaLoadedByAssetName; // key: asset file name (wide)

    std::wstring searchMeta = L"Assets\\*.meta";
    HANDLE hFindMeta = FindFirstFileW(searchMeta.c_str(), &fd);
    if (hFindMeta != INVALID_HANDLE_VALUE) {
        do {
            if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0) continue;
            std::wstring metaName(fd.cFileName); // e.g. L"foo.obj.meta"
            // derive asset name by removing trailing .meta
            std::wstring assetName = metaName.substr(0, metaName.size() - 5);
            std::string assetRel = Utf16ToUtf8(assetName);
            assetRel = std::string("Assets/") + assetRel;
            Meta m;
            if (LoadMeta(MakeMetaPath(assetRel), m)) {
                // check if actual file exists
                std::wstring assetPathW = L"Assets\\" + assetName;
                DWORD attr = GetFileAttributesW(assetPathW.c_str());
                if (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
                    // file exists: assign directly
                    metaLoadedByAssetName[assetName] = m;
                } else {
                    // orphan meta: index by recorded lastWrite
                    orphanMetaByTime[(uint64_t)m.lastWrite] = metaName;
                }
            }
        } while (FindNextFileW(hFindMeta, &fd));
        FindClose(hFindMeta);
    }

    int count = 0;
    // Now process files: attach existing meta or try to match orphan
    for (auto &f : files) {
        std::string fnUtf8 = Utf16ToUtf8(f.name);
        std::string rel = std::string("Assets/") + fnUtf8;
        std::string metaPath = MakeMetaPath(rel);

        Meta m;
        bool metaFound = false;

        // if meta file exists at expected path, load it
        std::wstring expectedMetaW = Utf8ToUtf16(metaPath);
        DWORD matt = GetFileAttributesW(expectedMetaW.c_str());
        if (matt != INVALID_FILE_ATTRIBUTES) {
            if (LoadMeta(metaPath, m)) {
                metaFound = true;
            }
        } else {
            // try to find an orphan meta by matching lastWrite
            auto it = orphanMetaByTime.find(f.lastWrite);
            if (it != orphanMetaByTime.end()) {
                // move/rename the meta file to new name
                std::wstring oldMetaName = it->second; // e.g. L"oldname.meta"
                std::wstring oldMetaPathW = L"Assets\\" + oldMetaName;
                std::wstring newMetaPathW = L"Assets\\" + f.name + L".meta";
                // attempt to move
                MoveFileW(oldMetaPathW.c_str(), newMetaPathW.c_str());
                // load moved meta
                if (LoadMeta(metaPath, m)) {
                    metaFound = true;
                }
                // remove from orphan map to avoid reuse
                orphanMetaByTime.erase(it);
            } else {
                // check if a meta was preloaded by exact asset name (case where asset existed when meta scanned)
                auto it2 = metaLoadedByAssetName.find(f.name);
                if (it2 != metaLoadedByAssetName.end()) {
                    m = it2->second;
                    metaFound = true;
                }
            }
        }

        if (!metaFound) {
            // create new meta
            m.guid = GenerateGUID();
            m.lastWrite = (long long)f.lastWrite;
            SaveMeta(metaPath, m);
        }

        metas_[rel] = m;
        guidToPath_[m.guid] = rel;
        if (!m.deps.empty()) deps_[m.guid] = m.deps;
        ++count;
    }

    return count;
}

std::vector<std::string> AssetDatabase::GetAllAssetPaths() const {
    std::lock_guard<std::mutex> lk(mutex_);
    std::vector<std::string> out;
    out.reserve(metas_.size());
    for (auto &p : metas_) out.push_back(p.first);
    return out;
}

// Simple polling watcher: check every second for new/removed files and rescan
void AssetDatabase::StartWatching() {
    if (watching_) return;
    watching_ = true;
    // open directory handle for asynchronous notifications
    dirHandle_ = CreateFileW(L"Assets",
                             FILE_LIST_DIRECTORY,
                             FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                             NULL,
                             OPEN_EXISTING,
                             FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
                             NULL);
    if (dirHandle_ == INVALID_HANDLE_VALUE) {
        // fallback to simple polling thread
        watchThread_ = std::thread([this]() {
            while (watching_) {
                ScanAssets();
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
        });
        return;
    }

    watchThread_ = std::thread([this]() {
        const int bufSize = 16 * 1024;
        std::vector<char> buffer(bufSize);
        OVERLAPPED ol = {};
        HANDLE hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        ol.hEvent = hEvent;

        while (watching_) {
            DWORD bytesReturned = 0;
            BOOL ok = ReadDirectoryChangesW(dirHandle_, buffer.data(), (DWORD)buffer.size(), TRUE,
                                            FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME |
                                            FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_SIZE,
                                            &bytesReturned, &ol, NULL);
            if (!ok) {
                // error; break and fallback
                break;
            }
            // wait for event or stop
            DWORD w = WaitForSingleObject(hEvent, 2000);
            if (w == WAIT_OBJECT_0) {
                // process notifications
                char* ptr = buffer.data();
                while (true) {
                    FILE_NOTIFY_INFORMATION* fni = (FILE_NOTIFY_INFORMATION*)ptr;
                    int len = fni->FileNameLength / sizeof(WCHAR);
                    std::wstring name(fni->FileName, fni->FileName + len);
                    // if not meta events, trigger a rescan
                    std::wstring lower = name;
                    for (auto &c : lower) c = towlower(c);
                    if (lower.find(L".meta") == std::wstring::npos) {
                        ScanAssets();
                        break;
                    }
                    if (fni->NextEntryOffset == 0) break;
                    ptr += fni->NextEntryOffset;
                }
            }
        }
        CloseHandle(hEvent);
    });
}

void AssetDatabase::StopWatching() {
    if (!watching_) return;
    watching_ = false;
    // cancel pending I/O
    if (dirHandle_ != INVALID_HANDLE_VALUE) {
        CancelIo(dirHandle_);
    }
    if (watchThread_.joinable()) watchThread_.join();
    if (dirHandle_ != INVALID_HANDLE_VALUE) {
        CloseHandle(dirHandle_);
        dirHandle_ = INVALID_HANDLE_VALUE;
    }
}

std::string AssetDatabase::ImportAsset(const std::string& srcPath) {
    // copy file into Assets with same filename (no dedupe)
    // convert srcPath to wide
    std::wstring wsrc = Utf8ToUtf16(srcPath);
    wchar_t fname[MAX_PATH];
    _wsplitpath_s(wsrc.c_str(), NULL,0,NULL,0,fname,MAX_PATH,NULL,0);
    std::wstring dest = L"Assets\\" + std::wstring(fname);
    if (!CopyFileW(wsrc.c_str(), dest.c_str(), FALSE)) return std::string();
    // create meta
    std::string fn = Utf16ToUtf8(std::wstring(fname));
    std::string rel = std::string("Assets/") + fn;
    Meta m;
    m.guid = GenerateGUID();
    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (GetFileAttributesExW(dest.c_str(), GetFileExInfoStandard, &fad)) {
        ULARGE_INTEGER li; li.HighPart = fad.ftLastWriteTime.dwHighDateTime; li.LowPart = fad.ftLastWriteTime.dwLowDateTime;
        m.lastWrite = (long long)li.QuadPart;
    }
    SaveMeta(MakeMetaPath(rel), m);
    metas_[rel] = m;
    guidToPath_[m.guid] = rel;
    return rel;
}

const AssetDatabase::Meta* AssetDatabase::GetMeta(const std::string& relativePath) const {
    auto it = metas_.find(relativePath);
    if (it == metas_.end()) return nullptr;
    return &it->second;
}

std::string AssetDatabase::GetGUID(const std::string& relativePath) const {
    auto m = GetMeta(relativePath);
    if (!m) return std::string();
    return m->guid;
}

