#pragma once
#include <vector>
#include <string>
#include <array>
#include "DxLib.h"

struct Mesh {
    // geometry
    std::vector<VECTOR> vertices;
    std::vector<VECTOR> normals;
    std::vector<VECTOR> uvs; // store UV as VECTOR(x=u,y=v,z=0)
    std::vector<int> indices; // triangles

    // submeshes and materials
    struct SubMesh { size_t indexStart; size_t indexCount; int materialIndex; };
    std::vector<SubMesh> submeshes;

    struct Material {
        std::string name;
        int diffuseColor = 0xFFFFFF;
        std::string diffuseTexture; // path
    };
    std::vector<Material> materials;

    // skinning
    std::vector<std::string> boneNames; // index -> bone name
    std::vector<MATRIX> boneOffsetMatrices; // inverse bind transform per bone
    // per-vertex bone indices and weights (support up to 4 bones per vertex)
    std::vector<std::array<int,4>> boneIndices;
    std::vector<std::array<float,4>> boneWeights;

    // node hierarchy (simple flat list)
    struct Node {
        std::string name;
        int parent = -1;
        MATRIX transform; // local transform
        std::vector<int> children;
    };
    std::vector<Node> nodes;

    // animations (simple representation)
    struct AnimKeyVec { double time; VECTOR value; };
    struct AnimKeyQuat { double time; VECTOR value; /* store quaternion x,y,z,w in vector*/ };
    struct AnimChannel {
        std::string nodeName;
        std::vector<AnimKeyVec> positions;
        std::vector<AnimKeyQuat> rotations;
        std::vector<AnimKeyVec> scales;
    };
    struct Animation {
        std::string name;
        double duration = 0.0;
        double ticksPerSecond = 25.0;
        std::vector<AnimChannel> channels;
    };
    std::vector<Animation> animations;

    void DrawWireframe(const MATRIX& world) const {
        // transform and draw edges (triangle list)
        for (size_t i = 0; i + 2 < indices.size(); i += 3) {
            VECTOR v0 = VTransform(vertices[indices[i + 0]], world);
            VECTOR v1 = VTransform(vertices[indices[i + 1]], world);
            VECTOR v2 = VTransform(vertices[indices[i + 2]], world);
            DrawLine3D(v0, v1, GetColor(200,200,200));
            DrawLine3D(v1, v2, GetColor(200,200,200));
            DrawLine3D(v2, v0, GetColor(200,200,200));
        }
    }
};
