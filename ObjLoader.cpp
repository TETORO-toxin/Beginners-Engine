#include "ObjLoader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

std::shared_ptr<Mesh> ObjLoader::LoadObj(const std::string& path) {
    std::ifstream ifs(path);
    if (!ifs) return nullptr;
    auto mesh = std::make_shared<Mesh>();
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
            // supports triangles or quads; faces may contain slashes
            std::vector<int> faceIdx;
            std::string part;
            while (iss >> part) {
                int vi = 0;
                if (part.find('/') != std::string::npos) {
                    // replace slashes then parse first number
                    std::replace(part.begin(), part.end(), '/', ' ');
                    std::istringstream ss(part);
                    ss >> vi;
                } else {
                    vi = std::stoi(part);
                }
                if (vi < 0) vi = (int)tempVerts.size() + 1 + vi; // negative index
                faceIdx.push_back(vi - 1);
            }
            if (faceIdx.size() == 3) {
                mesh->indices.push_back(faceIdx[0]);
                mesh->indices.push_back(faceIdx[1]);
                mesh->indices.push_back(faceIdx[2]);
            } else if (faceIdx.size() == 4) {
                // triangulate quad
                mesh->indices.push_back(faceIdx[0]); mesh->indices.push_back(faceIdx[1]); mesh->indices.push_back(faceIdx[2]);
                mesh->indices.push_back(faceIdx[0]); mesh->indices.push_back(faceIdx[2]); mesh->indices.push_back(faceIdx[3]);
            } else if (faceIdx.size() > 4) {
                // fan triangulation
                for (size_t i = 1; i + 1 < faceIdx.size(); ++i) {
                    mesh->indices.push_back(faceIdx[0]);
                    mesh->indices.push_back(faceIdx[i]);
                    mesh->indices.push_back(faceIdx[i+1]);
                }
            }
        }
    }
    mesh->vertices = std::move(tempVerts);
    return mesh;
}
