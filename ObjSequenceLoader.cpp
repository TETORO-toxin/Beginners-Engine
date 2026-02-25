#include "ObjSequenceLoader.h"
#include <Windows.h>
#include <regex>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

static bool LoadObjBasic(const std::string& path, std::vector<VECTOR>& outVerts, std::vector<int>& outIndices) {
    std::ifstream ifs(path);
    if (!ifs) return false;
    std::vector<VECTOR> tempVerts;
    std::string line;
    while (std::getline(ifs, line)) {
        if (line.size() < 2) continue;
        if (line[0] == '#') continue;
        std::istringstream iss(line);
        std::string tag;
        iss >> tag;
        if (tag == "v") {
            float x,y,z; iss >> x >> y >> z;
            tempVerts.push_back(VGet(x,y,z));
        } else if (tag == "f") {
            std::vector<int> faceIdx;
            std::string part;
            while (iss >> part) {
                int vi = 0;
                if (part.find('/') != std::string::npos) {
                    std::replace(part.begin(), part.end(), '/', ' ');
                    std::istringstream ss(part);
                    ss >> vi;
                } else {
                    vi = std::stoi(part);
                }
                if (vi < 0) vi = (int)tempVerts.size() + 1 + vi;
                faceIdx.push_back(vi - 1);
            }
            if (faceIdx.size() == 3) {
                outIndices.push_back(faceIdx[0]); outIndices.push_back(faceIdx[1]); outIndices.push_back(faceIdx[2]);
            } else if (faceIdx.size() == 4) {
                outIndices.push_back(faceIdx[0]); outIndices.push_back(faceIdx[1]); outIndices.push_back(faceIdx[2]);
                outIndices.push_back(faceIdx[0]); outIndices.push_back(faceIdx[2]); outIndices.push_back(faceIdx[3]);
            } else if (faceIdx.size() > 4) {
                for (size_t i = 1; i + 1 < faceIdx.size(); ++i) {
                    outIndices.push_back(faceIdx[0]); outIndices.push_back(faceIdx[i]); outIndices.push_back(faceIdx[i+1]);
                }
            }
        }
    }
    outVerts = std::move(tempVerts);
    return true;
}

std::shared_ptr<ObjSequence> ObjSequenceLoader::LoadSequence(const std::string& examplePath) {
    // split directory and filename
    std::string dir;
    std::string filename;
    size_t pos = examplePath.find_last_of("\\/");
    if (pos == std::string::npos) {
        dir = ".";
        filename = examplePath;
    } else {
        dir = examplePath.substr(0, pos);
        filename = examplePath.substr(pos + 1);
    }

    // regex to capture prefix, digits, suffix
    std::smatch m;
    std::regex r("(.*?)(\\d+)(\\.obj)$", std::regex::icase);
    if (!std::regex_match(filename, m, r)) return nullptr;
    std::string prefix = m[1].str();
    std::string digits = m[2].str();
    std::string suffix = m[3].str();

    // enumerate files in dir using Win32 API
    std::string search = dir + "\\*";
    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA(search.c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE) return nullptr;

    std::vector<std::pair<int, std::string>> found;
    do {
        if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) continue;
        std::string fn = fd.cFileName;
        std::smatch mm;
        if (std::regex_match(fn, mm, r)) {
            if (mm[1].str() == prefix && mm[3].str() == suffix) {
                int idx = std::stoi(mm[2].str());
                std::string full = dir + "\\" + fn;
                found.push_back({idx, full});
            }
        }
    } while (FindNextFileA(hFind, &fd) != 0);
    FindClose(hFind);

    if (found.empty()) return nullptr;
    std::sort(found.begin(), found.end(), [](auto &a, auto &b){ return a.first < b.first; });

    auto seq = std::make_shared<ObjSequence>();
    seq->frames.reserve(found.size());
    std::vector<int> indices;
    bool indicesSet = false;
    for (auto &f : found) {
        std::vector<VECTOR> verts;
        std::vector<int> idxs;
        if (!LoadObjBasic(f.second, verts, idxs)) return nullptr;
        if (!indicesSet) { indices = idxs; indicesSet = true; }
        else {
            if (idxs != indices) {
                std::cerr << "ObjSequenceLoader: index mismatch in " << f.second << "\n";
                return nullptr;
            }
        }
        seq->frames.push_back(std::move(verts));
    }
    seq->indices = std::move(indices);
    return seq;
}
