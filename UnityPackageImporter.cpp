#include "UnityPackageImporter.h"
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <windows.h>
#include <shellapi.h>
#include "third_party/miniz/miniz.h"

static std::string ReadFileToStringUtf8(const std::wstring& path) {
    std::string out;
    HANDLE h = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return out;
    LARGE_INTEGER size;
    if (!GetFileSizeEx(h, &size) || size.QuadPart == 0) { CloseHandle(h); return out; }
    DWORD toRead = (DWORD)size.QuadPart;
    out.resize(toRead);
    DWORD read = 0;
    if (!ReadFile(h, &out[0], toRead, &read, NULL)) { CloseHandle(h); out.clear(); return out; }
    CloseHandle(h);
    return out; // assume file bytes are UTF-8 or ASCII
}

static std::wstring Utf8ToW(const std::string& s) {
    int req = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, NULL, 0);
    if (req <= 0) return L"";
    std::wstring w; w.resize(req - 1);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &w[0], req);
    return w;
}

static std::string WToUtf8(const std::wstring& w) {
    int req = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, NULL, 0, NULL, NULL);
    if (req <= 0) return std::string();
    std::string s; s.resize(req - 1);
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, &s[0], req, NULL, NULL);
    return s;
}

// Helper: run system tar to extract package into temp dir. Returns true on success.
static bool TryTarExtract(const std::wstring& packagePath, const std::wstring& destDir) {
    // Build command: tar -xf "<pkg>" -C "<destDir>"
    std::wstring cmd = L"tar -xf \"" + packagePath + L"\" -C \"" + destDir + L"\"";
    STARTUPINFOW si; PROCESS_INFORMATION pi; ZeroMemory(&si, sizeof(si)); si.cb = sizeof(si); ZeroMemory(&pi, sizeof(pi));
    // CreateProcess requires writable buffer
    std::vector<wchar_t> cmdBuf(cmd.begin(), cmd.end()); cmdBuf.push_back(0);
    BOOL ok = CreateProcessW(NULL, cmdBuf.data(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
    if (!ok) return false;
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exitCode = 0; GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess); CloseHandle(pi.hThread);
    return exitCode == 0;
}

// parse gzip header and call tinfl_uncompress on the deflate payload
static bool DecompressGzip(const std::string& in, std::vector<char>& out) {
    if (in.size() < 18) return false;
    const unsigned char* data = (const unsigned char*)in.data();
    if (data[0] != 0x1f || data[1] != 0x8b || data[2] != 0x08) return false; // not gzip
    unsigned int pos = 10;
    unsigned char flg = data[3];
    if (flg & 0x04) {
        if (pos + 2 > in.size()) return false;
        unsigned int xlen = data[pos] | (data[pos+1] << 8);
        pos += 2 + xlen;
    }
    if (flg & 0x08) { // original file name
        while (pos < in.size() && data[pos] != 0) ++pos;
        ++pos;
    }
    if (flg & 0x10) { // comment
        while (pos < in.size() && data[pos] != 0) ++pos;
        ++pos;
    }
    if (flg & 0x02) { pos += 2; }
    if (pos >= in.size()) return false;
    if (in.size() < 8) return false;
    // ISIZE is last 4 bytes (little endian)
    uint32_t isize = (uint8_t)in[in.size()-4] | ((uint8_t)in[in.size()-3] << 8) | ((uint8_t)in[in.size()-2] << 16) | ((uint8_t)in[in.size()-1] << 24);
    const void* compData = in.data() + pos;
    size_t compSize = in.size() - pos - 8; // minus CRC32+ISIZE
    if (compSize <= 0) return false;

    // allocate output buffer using ISIZE if non-zero, else heuristic
    size_t outSize = (isize != 0) ? isize : compSize * 4 + 1024;
    out.resize(outSize);
    size_t outSizeT = outSize;
    int res = tinfl_uncompress(out.data(), &outSizeT, compData, compSize);
    if (res < 0) {
        // failure
        return false;
    }
    out.resize(outSizeT);
    return true;
}

// write bytes to a path, creating parent directories
static bool WriteFileEnsureDirs(const std::wstring& path, const char* data, size_t size) {
    // create parent dirs
    std::wstring p = path;
    size_t pos = p.find_last_of(L"\\/");
    if (pos != std::wstring::npos) {
        std::wstring dir = p.substr(0, pos);
        // create directories iteratively
        std::wstring cur;
        for (size_t i = 0; i < dir.size(); ++i) {
            cur.push_back(dir[i]);
            if (dir[i] == L'\\' || dir[i] == L'/') {
                CreateDirectoryW(cur.c_str(), NULL);
            }
        }
        CreateDirectoryW(dir.c_str(), NULL);
    }

    HANDLE h = CreateFileW(path.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return false;
    DWORD written = 0;
    bool ok = !!WriteFile(h, data, (DWORD)size, &written, NULL) && written == size;
    CloseHandle(h);
    return ok;
}

bool UnityPackageImporter::ImportUnityPackage(const std::string& packagePath, const std::string& assetsDir, std::vector<std::string>& outImported) {
    outImported.clear();

    // ensure assetsDir exists
    std::wstring assetsDirW = Utf8ToW(assetsDir);
    CreateDirectoryW(assetsDirW.c_str(), NULL);

    // create temp folder
    wchar_t tmpPath[MAX_PATH];
    GetTempPathW(MAX_PATH, tmpPath);
    wchar_t tempFile[MAX_PATH];
    GetTempFileNameW(tmpPath, L"upk", 0, tempFile);
    // delete file and make dir
    DeleteFileW(tempFile);
    CreateDirectoryW(tempFile, NULL);

    bool extracted = false;

    // try system tar extraction first (more reliable when available)
    std::wstring pkgW = Utf8ToW(packagePath);
    if (TryTarExtract(pkgW, std::wstring(tempFile))) {
        extracted = true;
    } else {
        // try tool-independent extraction: read file and gunzip+parse tar
        std::string fileData = ReadFileToStringUtf8(Utf8ToW(packagePath));
        if (!fileData.empty()) {
            std::vector<char> tarData;
            if (DecompressGzip(fileData, tarData)) {
                // parse tar (ustar or simple tar)
                size_t off = 0;
                while (off + 512 <= tarData.size()) {
                    const char* header = tarData.data() + off;
                    bool empty = true;
                    for (int i = 0; i < 100; ++i) { if (header[i]) { empty = false; break; } }
                    if (empty) break;
                    // name
                    char namebuf[256] = {0};
                    memcpy(namebuf, header, 100);
                    // size octal at offset 124, length 12
                    char sizebuf[32] = {0};
                    memcpy(sizebuf, header + 124, 12);
                    size_t fsize = 0;
                    for (int i = 0; i < 12; ++i) {
                        if (sizebuf[i] >= '0' && sizebuf[i] <= '7') {
                            fsize = (fsize << 3) + (sizebuf[i] - '0');
                        }
                    }
                    char type = header[156];
                    std::string name(namebuf);
                    size_t dataOff = off + 512;
                    if (fsize > 0 && dataOff + fsize <= tarData.size()) {
                        // write to temp folder preserving path
                        std::wstring dest = std::wstring(tempFile) + L"\\" + Utf8ToW(name);
                        WriteFileEnsureDirs(dest, tarData.data() + dataOff, fsize);
                    } else {
                        // directory or empty file
                        if (name.size() && (type == '5' || name.back() == '/')) {
                            std::wstring dir = std::wstring(tempFile) + L"\\" + Utf8ToW(name);
                            CreateDirectoryW(dir.c_str(), NULL);
                        }
                    }
                    size_t total = ((fsize + 511) / 512) * 512;
                    off = dataOff + total;
                }
                extracted = true;
            }
        }
    }

    if (!extracted) {
        // fallback to tar command (as before)
        std::wstring cmd = L"tar -xf \"" + pkgW + L"\" -C \"" + std::wstring(tempFile) + L"\"";

        STARTUPINFOW si; PROCESS_INFORMATION pi; ZeroMemory(&si, sizeof(si)); si.cb = sizeof(si); ZeroMemory(&pi, sizeof(pi));
        // CreateProcess requires writable buffer
        std::vector<wchar_t> cmdBuf(cmd.begin(), cmd.end()); cmdBuf.push_back(0);
        if (!CreateProcessW(NULL, cmdBuf.data(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
            // cleanup
            std::wstring from = std::wstring(tempFile) + L"\0";
            SHFILEOPSTRUCTW fo = {0};
            fo.wFunc = FO_DELETE;
            fo.pFrom = from.c_str();
            fo.fFlags = FOF_NO_UI | FOF_SILENT | FOF_NOCONFIRMATION;
            SHFileOperationW(&fo);
            return false;
        }
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess); CloseHandle(pi.hThread);
    }

    // iterate subfolders in temp dir using FindFirstFileW
    std::wstring search = std::wstring(tempFile) + L"\\*";
    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileW(search.c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE) {
        // cleanup
        SHFILEOPSTRUCTW fo = {0};
        fo.wFunc = FO_DELETE; fo.pFrom = tempFile; fo.fFlags = FOF_NO_UI | FOF_SILENT;
        SHFileOperationW(&fo);
        return false;
    }
    do {
        if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0) continue;
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            std::wstring folder = std::wstring(tempFile) + L"\\" + fd.cFileName;
            std::wstring pathnameFile = folder + L"\\pathname";
            std::wstring assetFile = folder + L"\\asset";
            DWORD pa = GetFileAttributesW(pathnameFile.c_str());
            DWORD aa = GetFileAttributesW(assetFile.c_str());
            if (pa != INVALID_FILE_ATTRIBUTES && aa != INVALID_FILE_ATTRIBUTES) {
                std::string originalPath = ReadFileToStringUtf8(pathnameFile);
                if (originalPath.size() >= 4) {
                    std::string lower;
                    lower.resize(originalPath.size());
                    for (size_t i = 0; i < originalPath.size(); ++i) lower[i] = (char)tolower((unsigned char)originalPath[i]);
                    std::string ext = lower.substr(lower.size()-4);
                    if (ext == ".fbx" || ext == ".obj") {
                        // get filename from originalPath (UTF-8) and convert to wide
                        size_t pos = originalPath.find_last_of("/\\");
                        std::string fname = (pos == std::string::npos) ? originalPath : originalPath.substr(pos+1);
                        std::wstring fnameW = Utf8ToW(fname);
                        std::wstring dest = assetsDirW + L"\\" + fnameW;
                        // copy assetFile to dest
                        if (CopyFileW(assetFile.c_str(), dest.c_str(), FALSE)) {
                            outImported.push_back(WToUtf8(fnameW));
                        }
                    }
                }
            }
        }
    } while (FindNextFileW(hFind, &fd));
    FindClose(hFind);

    // cleanup temp dir
    std::wstring from = std::wstring(tempFile) + L"\0"; // double-null terminated
    SHFILEOPSTRUCTW fo = {0};
    fo.wFunc = FO_DELETE;
    fo.pFrom = from.c_str();
    fo.fFlags = FOF_NO_UI | FOF_SILENT | FOF_NOCONFIRMATION;
    SHFileOperationW(&fo);

    return !outImported.empty();
}
