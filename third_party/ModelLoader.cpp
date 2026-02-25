#include "ModelLoader.h"
#include <iostream>

#if __has_include(<assimp/Importer.hpp>)
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

static VECTOR AiVecToVector(const aiVector3D& v) { return VGet(v.x, v.y, v.z); }
static MATRIX AiMatToMatrix(const aiMatrix4x4& m) {
    MATRIX out;
    // DxLib MATRIX is row-major 4x4 with members m[4][4] accessible? Use SetMatrix from DxLib is not available here; populate manually
    // We'll map aiMatrix to MATRIX fields if available as float member.
    // As a minimal approach, set identity and ignore full mapping; users can extend if needed.
    SetIdentity(&out);
    out.m[0][0] = m.a1; out.m[0][1] = m.a2; out.m[0][2] = m.a3; out.m[0][3] = m.a4;
    out.m[1][0] = m.b1; out.m[1][1] = m.b2; out.m[1][2] = m.b3; out.m[1][3] = m.b4;
    out.m[2][0] = m.c1; out.m[2][1] = m.c2; out.m[2][2] = m.c3; out.m[2][3] = m.c4;
    out.m[3][0] = m.d1; out.m[3][1] = m.d2; out.m[3][2] = m.d3; out.m[3][3] = m.d4;
    return out;
}

std::shared_ptr<Mesh> ModelLoader::LoadModel(const std::string& path) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path,
        aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_GenNormals | aiProcess_CalcTangentSpace);
    if (!scene || !scene->HasMeshes()) {
        std::cerr << "ModelLoader: failed to load " << path << "\n";
        return nullptr;
    }

    auto outMesh = std::make_shared<Mesh>();

    // materials
    outMesh->materials.reserve(scene->mNumMaterials);
    for (unsigned int i = 0; i < scene->mNumMaterials; ++i) {
        aiMaterial* m = scene->mMaterials[i];
        Mesh::Material mat;
        aiString name;
        if (m->Get(AI_MATKEY_NAME, name) == AI_SUCCESS) mat.name = name.C_Str();
        aiColor3D col(1.0f,1.0f,1.0f);
        if (m->Get(AI_MATKEY_COLOR_DIFFUSE, col) == AI_SUCCESS) {
            int r = (int)(col.r * 255.0f);
            int g = (int)(col.g * 255.0f);
            int b = (int)(col.b * 255.0f);
            mat.diffuseColor = (r<<16)|(g<<8)|b;
        }
        aiString tex;
        if (m->GetTextureCount(aiTextureType_DIFFUSE) > 0 && m->GetTexture(aiTextureType_DIFFUSE, 0, &tex) == AI_SUCCESS) {
            mat.diffuseTexture = tex.C_Str();
        }
        outMesh->materials.push_back(mat);
    }

    // For simplicity, merge all meshes into single Mesh and create submesh entries
    size_t vertexOffset = 0;
    size_t indexOffset = 0;
    for (unsigned int mi = 0; mi < scene->mNumMeshes; ++mi) {
        const aiMesh* am = scene->mMeshes[mi];
        // record submesh
        Mesh::SubMesh sm;
        sm.indexStart = outMesh->indices.size();
        sm.indexCount = 0;
        sm.materialIndex = (am->mMaterialIndex >= 0) ? am->mMaterialIndex : -1;

        // vertices
        for (unsigned int vi = 0; vi < am->mNumVertices; ++vi) {
            outMesh->vertices.push_back(AiVecToVector(am->mVertices[vi]));
            if (am->HasNormals()) outMesh->normals.push_back(AiVecToVector(am->mNormals[vi])); else outMesh->normals.push_back(VGet(0,0,0));
            if (am->HasTextureCoords(0)) {
                aiVector3D uv = am->mTextureCoords[0][vi];
                outMesh->uvs.push_back(VGet(uv.x, uv.y, 0));
            } else {
                outMesh->uvs.push_back(VGet(0,0,0));
            }
            // initialize bone data
            outMesh->boneIndices.push_back({{-1,-1,-1,-1}});
            outMesh->boneWeights.push_back({{0,0,0,0}});
        }

        // faces / indices
        if (am->HasFaces()) {
            for (unsigned int fi = 0; fi < am->mNumFaces; ++fi) {
                const aiFace& f = am->mFaces[fi];
                if (f.mNumIndices == 3) {
                    outMesh->indices.push_back((int)(vertexOffset + f.mIndices[0]));
                    outMesh->indices.push_back((int)(vertexOffset + f.mIndices[1]));
                    outMesh->indices.push_back((int)(vertexOffset + f.mIndices[2]));
                    sm.indexCount += 3;
                } else if (f.mNumIndices > 3) {
                    for (unsigned int k = 1; k + 1 < f.mNumIndices; ++k) {
                        outMesh->indices.push_back((int)(vertexOffset + f.mIndices[0]));
                        outMesh->indices.push_back((int)(vertexOffset + f.mIndices[k]));
                        outMesh->indices.push_back((int)(vertexOffset + f.mIndices[k+1]));
                        sm.indexCount += 3;
                    }
                }
            }
        }

        // bones
        if (am->HasBones()) {
            for (unsigned int bi = 0; bi < am->mNumBones; ++bi) {
                aiBone* ab = am->mBones[bi];
                // find or add bone name
                std::string bname = ab->mName.C_Str();
                int boneIndex = -1;
                for (size_t bn = 0; bn < outMesh->boneNames.size(); ++bn) {
                    if (outMesh->boneNames[bn] == bname) { boneIndex = (int)bn; break; }
                }
                if (boneIndex == -1) {
                    boneIndex = (int)outMesh->boneNames.size();
                    outMesh->boneNames.push_back(bname);
                    outMesh->boneOffsetMatrices.push_back(AiMatToMatrix(ab->mOffsetMatrix));
                }
                // assign weights
                for (unsigned int wi = 0; wi < ab->mNumWeights; ++wi) {
                    unsigned int vindex = ab->mWeights[wi].mVertexId + (unsigned int)vertexOffset;
                    float w = ab->mWeights[wi].mWeight;
                    auto& bidx = outMesh->boneIndices[vindex];
                    auto& bw = outMesh->boneWeights[vindex];
                    for (int k = 0; k < 4; ++k) {
                        if (bidx[k] == -1) { bidx[k] = boneIndex; bw[k] = w; break; }
                    }
                }
            }
        }

        outMesh->submeshes.push_back(sm);
        vertexOffset += am->mNumVertices;
    }

    // normalize weights per-vertex
    for (size_t vi = 0; vi < outMesh->boneWeights.size(); ++vi) {
        float sum = 0.0f;
        for (int k = 0; k < 4; ++k) sum += outMesh->boneWeights[vi][k];
        if (sum > 0.0f) {
            for (int k = 0; k < 4; ++k) outMesh->boneWeights[vi][k] /= sum;
        }
    }

    // node hierarchy - simple traversal
    std::function<int(const aiNode*, int)> addNode = [&](const aiNode* n, int parent)->int {
        int idx = (int)outMesh->nodes.size();
        Mesh::Node node;
        node.name = n->mName.C_Str();
        node.parent = parent;
        node.transform = AiMatToMatrix(n->mTransformation);
        outMesh->nodes.push_back(node);
        if (parent >= 0) outMesh->nodes[parent].children.push_back(idx);
        for (unsigned int c = 0; c < n->mNumChildren; ++c) addNode(n->mChildren[c], idx);
        return idx;
    };
    addNode(scene->mRootNode, -1);

    // animations
    if (scene->HasAnimations()) {
        for (unsigned int ai = 0; ai < scene->mNumAnimations; ++ai) {
            aiAnimation* a = scene->mAnimations[ai];
            Mesh::Animation anim;
            anim.name = a->mName.C_Str();
            anim.duration = a->mDuration;
            anim.ticksPerSecond = (a->mTicksPerSecond != 0.0) ? a->mTicksPerSecond : 25.0;
            for (unsigned int ch = 0; ch < a->mNumChannels; ++ch) {
                aiNodeAnim* na = a->mChannels[ch];
                Mesh::AnimChannel channel;
                channel.nodeName = na->mNodeName.C_Str();
                for (unsigned int p = 0; p < na->mNumPositionKeys; ++p) {
                    auto& k = na->mPositionKeys[p];
                    channel.positions.push_back({k.mTime, AiVecToVector(k.mValue)});
                }
                for (unsigned int r = 0; r < na->mNumRotationKeys; ++r) {
                    auto& k = na->mRotationKeys[r];
                    channel.rotations.push_back({k.mTime, VGet((float)k.mValue.x,(float)k.mValue.y,(float)k.mValue.z)}); // w ignored
                }
                for (unsigned int s = 0; s < na->mNumScalingKeys; ++s) {
                    auto& k = na->mScalingKeys[s];
                    channel.scales.push_back({k.mTime, AiVecToVector(k.mValue)});
                }
                anim.channels.push_back(channel);
            }
            outMesh->animations.push_back(anim);
        }
    }

    return outMesh;
}

#else

std::shared_ptr<Mesh> ModelLoader::LoadModel(const std::string& path) {
    std::cerr << "ModelLoader: Assimp not available, cannot load: " << path << "\n";
    return nullptr;
}

#endif
