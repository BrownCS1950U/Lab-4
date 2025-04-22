#include "skinnedMesh.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "../texture.h"
#include "../camera.h"
#include "../shaders.h"

#ifdef _WIN32
#include <Windows.h>
#else
#include <sys/time.h>
#endif

using namespace std;

#define POSITION_LOCATION 0
#define TEX_COORD_LOCATION 1
#define NORMAL_LOCATION 2
#define BONE_ID_LOCATION 3
#define BONE_WEIGHT_LOCATION 4

long long GetCurrentTimeMillis()
{
#ifdef _WIN32
    return GetTickCount64();
#else
    timeval t;
    gettimeofday(&t, NULL);

    long long ret = t.tv_sec * 1000 + t.tv_usec / 1000;
    return ret;
#endif
}

inline glm::mat4 AiToGlmMat4(const aiMatrix4x4& mat) {
    return {
        mat.a1, mat.b1, mat.c1, mat.d1,
        mat.a2, mat.b2, mat.c2, mat.d2,
        mat.a3, mat.b3, mat.c3, mat.d3,
        mat.a4, mat.b4, mat.c4, mat.d4
    };
}

inline glm::mat3 AiToGlmMat3(const aiMatrix3x3& mat) {
    return {
        mat.a1, mat.b1, mat.c1,
        mat.a2, mat.b2, mat.c2,
        mat.a3, mat.b3, mat.c3
    };
}

inline glm::vec3 AiToGlmVec3(const aiVector3D& vec) {
    return {vec.x, vec.y, vec.z};
}

inline glm::quat AiToGlmQuat(const aiQuaternion& quat) {
    return {quat.w, quat.x, quat.y, quat.z};
}

SkinnedMesh::~SkinnedMesh() { Clear(); }

void SkinnedMesh::Clear() {
    if (m_Buffers[0] != 0) {
        glDeleteBuffers(std::size(m_Buffers), m_Buffers);
    }

    if (m_VAO != 0) {
        glDeleteVertexArrays(1, &m_VAO);
        m_VAO = 0;
    }
}

bool SkinnedMesh::init() {
    GLuint vs = gl::Shader::init_shaders(GL_VERTEX_SHADER, "../res/shaders/skinned_vertex.glsl");
    GLuint fs = gl::Shader::init_shaders(GL_FRAGMENT_SHADER, "../res/shaders/skinned_fragment.glsl");
    m_shaderProg = gl::Shader::init_program(vs, fs);
    GLint linkStatus;
    glGetProgramiv(m_shaderProg, GL_LINK_STATUS, &linkStatus);
    if (linkStatus != GL_TRUE) {
        GLchar infoLog[512];
        glGetProgramInfoLog(m_shaderProg, 512, NULL, infoLog);
        fprintf(stderr, "Program linking failed: %s\n", infoLog);
        return false;
    }

    glUseProgram(m_shaderProg);
    WVPLoc = gl::Shader::GetUniformLocation("gWVP", m_shaderProg);
    samplerLoc = gl::Shader::GetUniformLocation("gSampler", m_shaderProg);
    samplerSpecularExponentLoc = gl::Shader::GetUniformLocation("gSamplerSpecularExponent", m_shaderProg);
    materialLoc.AmbientColor = gl::Shader::GetUniformLocation("gMaterial.AmbientColor", m_shaderProg);
    materialLoc.DiffuseColor = gl::Shader::GetUniformLocation("gMaterial.DiffuseColor", m_shaderProg);
    materialLoc.SpecularColor = gl::Shader::GetUniformLocation("gMaterial.SpecularColor", m_shaderProg);
    CameraLocalPosLoc = gl::Shader::GetUniformLocation("gCameraLocalPos", m_shaderProg);


    if (WVPLoc == 0xFFFFFFFF ||
        samplerLoc == 0xFFFFFFFF ||
        samplerSpecularExponentLoc == 0xFFFFFFFF ||
        materialLoc.AmbientColor == 0xFFFFFFFF ||
        materialLoc.DiffuseColor == 0xFFFFFFFF ||
        materialLoc.SpecularColor == 0xFFFFFFFF ||
        CameraLocalPosLoc == 0xFFFFFFFF){return false;}

    for (unsigned int i = 0 ; i < std::size(m_boneLocation) ; i++) {
        char Name[200];
        memset(Name, 0, sizeof(Name));
        SNPRINTF(Name, sizeof(Name), "gBones[%d]", i);
        m_boneLocation[i] = gl::Shader::GetUniformLocation(Name, m_shaderProg);
    }
    glUniform1i(samplerLoc, 0);
    glUniform1i(samplerSpecularExponentLoc, 8);

    return true;
}

bool SkinnedMesh::LoadMesh(const string& Filename) {

    Clear();  // Release the previously loaded mesh (if it exists)

    glGenVertexArrays(1, &m_VAO);
    glBindVertexArray(m_VAO);
    glGenBuffers(std::size(m_Buffers), m_Buffers);

    bool Ret = false;
    pScene = Importer.ReadFile(Filename.c_str(), ASSIMP_LOAD_FLAGS);
    glm::mat4 fixZUp = glm::rotate(glm::mat4(1.0f), -glm::half_pi<float>(), glm::vec3(1, 0, 0));

    if (pScene) {
        m_GlobalInverseTransform = glm::inverse(fixZUp * AiToGlmMat4(pScene->mRootNode->mTransformation));
        Ret = InitFromScene(pScene, Filename);
    } else printf("Error parsing '%s': '%s'\n", Filename.c_str(), Importer.GetErrorString());

    glBindVertexArray(0);
    m_startTime = GetCurrentTimeMillis();
    m_currentTime = m_startTime;
    return Ret;
}

GLenum SkinnedMesh::InitFromScene(const aiScene* paiScene, const string& Filename) {

    m_Meshes.resize(paiScene->mNumMeshes);
    m_Materials.resize(paiScene->mNumMaterials);

    unsigned int NumVertices = 0;
    unsigned int NumIndices = 0;

    CountVerticesAndIndices(paiScene, NumVertices, NumIndices);
    m_Indices.reserve(NumIndices);
    InitAllMeshes(paiScene);

    if (!InitMaterials(paiScene, Filename)) {
        return false;
    }

    PopulateBuffers();
    return Debug::checkGLError();
}


void SkinnedMesh::CountVerticesAndIndices(const aiScene* paiScene, unsigned int& NumVertices, unsigned int& NumIndices) {
    for (unsigned int i = 0 ; i < m_Meshes.size() ; i++) {
        m_Meshes[i].MaterialIndex = paiScene->mMeshes[i]->mMaterialIndex;
        m_Meshes[i].NumIndices = paiScene->mMeshes[i]->mNumFaces * 3;
        m_Meshes[i].BaseVertex = NumVertices;
        m_Meshes[i].BaseIndex = NumIndices;

        NumVertices += paiScene->mMeshes[i]->mNumVertices;
        NumIndices  += m_Meshes[i].NumIndices;
    }
}

void SkinnedMesh::InitAllMeshes(const aiScene* paiScene) {
    for (unsigned int i = 0 ; i < m_Meshes.size() ; i++) {
        const aiMesh* paiMesh = paiScene->mMeshes[i];
        InitSingleMesh(i, paiMesh);
    }
}

void SkinnedMesh::InitSingleMesh(uint MeshIndex, const aiMesh* paiMesh) {

    const aiVector3D Zero3D(0.0f, 0.0f, 0.0f);
    SkinnedVertex v;

    for (unsigned int i = 0; i < paiMesh->mNumVertices; i++) {
        v.Position = AiToGlmVec3(paiMesh->mVertices[i]);
        m_Positions.push_back(v.Position);
        v.Normal = (paiMesh->mNormals) ? AiToGlmVec3(paiMesh->mNormals[i]) : glm::vec3(0.0f, 1.0f, 0.0f);
        m_Normals.push_back(-v.Normal);
        const aiVector3D& pTexCoord = paiMesh->HasTextureCoords(0) ? paiMesh->mTextureCoords[0][i] : Zero3D;
        v.TexCoords = glm::vec2(pTexCoord.x, 1.0f - pTexCoord.y);
        m_TexCoords.push_back(v.TexCoords);
        m_SkinnedVertices.push_back(v);
    }

    for (unsigned int i = 0; i < paiMesh->mNumFaces; i++) {
        const aiFace& Face = paiMesh->mFaces[i];
        m_Indices.push_back(Face.mIndices[0]);
        m_Indices.push_back(Face.mIndices[1]);
        m_Indices.push_back(Face.mIndices[2]);
    }

    LoadMeshBones(MeshIndex, paiMesh, m_SkinnedVertices, m_Meshes[MeshIndex].BaseVertex);
}


void SkinnedMesh::LoadMeshBones(uint MeshIndex, const aiMesh* pMesh, vector<SkinnedVertex>& SkinnedVertices, int BaseVertex) {

    if (pMesh->mNumBones > MAX_BONES) {
        printf("The number of bones in the model (%d) is larger than the maximum supported (%d)\n", pMesh->mNumBones, MAX_BONES);
        printf("Make sure to increase the macro MAX_BONES in the C++ header as well as in the shader to the same value\n");
        assert(0);
    }

    for (uint i = 0 ; i < pMesh->mNumBones ; i++) {
        LoadSingleBone(MeshIndex, pMesh->mBones[i], SkinnedVertices, BaseVertex);
    }
}

void SkinnedMesh::LoadSingleBone(uint MeshIndex, const aiBone* pBone,
                                 vector<SkinnedVertex>& SkinnedVertices, int BaseVertex) {

    int BoneId = GetBoneId(pBone);
    if (BoneId == m_BoneInfo.size()) {
        BoneInfo bi(AiToGlmMat4(pBone->mOffsetMatrix));
        m_BoneInfo.push_back(bi);
    }

    for (uint i = 0 ; i < pBone->mNumWeights ; i++) {
        const aiVertexWeight& vw = pBone->mWeights[i];
        uint GlobalVertexID = BaseVertex + pBone->mWeights[i].mVertexId;
        SkinnedVertices[GlobalVertexID].Bones.AddBoneData(BoneId, vw.mWeight);
    }
}

int SkinnedMesh::GetBoneId(const aiBone* pBone) {
    int BoneIndex = 0;
    string BoneName(pBone->mName.C_Str());

    if (!m_BoneNameToIndexMap.contains(BoneName)) {
        BoneIndex = (int)m_BoneNameToIndexMap.size();
        m_BoneNameToIndexMap[BoneName] = BoneIndex;
    } else {
        BoneIndex = m_BoneNameToIndexMap[BoneName];
    }

    return BoneIndex;
}

string GetDirFromFilename(const string& Filename) {
    string::size_type SlashIndex;

#ifdef _WIN64
    SlashIndex = Filename.find_last_of("\\");
    if (SlashIndex == -1) SlashIndex = Filename.find_last_of("/");
#else
    SlashIndex = Filename.find_last_of("/");
#endif

    string Dir;
    if (SlashIndex == string::npos) Dir = ".";
    else if (SlashIndex == 0) Dir = "/";
    else Dir = Filename.substr(0, SlashIndex);
    return Dir;
}

bool SkinnedMesh::InitMaterials(const aiScene* paiScene, const string& Filename) {

    string Dir = GetDirFromFilename(Filename);
    bool Ret = true;
    for (int i = 0 ; i < paiScene->mNumMaterials ; i++) {
        const aiMaterial* pMaterial = paiScene->mMaterials[i];
        LoadTextures(Dir, pMaterial, i);
        LoadColors(pMaterial, i);
    }

    return Ret;
}

void SkinnedMesh::LoadTextures(const string& Dir, const aiMaterial* pMaterial, int index) {
    LoadDiffuseTexture(Dir, pMaterial, index);
    LoadSpecularTexture(Dir, pMaterial, index);
}

void SkinnedMesh::LoadDiffuseTexture(const string& Dir, const aiMaterial* pMaterial, int index) {

    m_Materials[index].pDiffuse = 0;
    if (pMaterial->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
        aiString Path;

        if (pMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &Path, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS) {
            const aiTexture* paiTexture = pScene->GetEmbeddedTexture(Path.C_Str());
            string p(Path.data);

#ifdef _WIN32
            if (p.starts_with(".\\")) {
                p = p.substr(2, p.size() - 2);
            }
#else
            if (p.starts_with("./")) {
                p = p.substr(2, p.size() - 2);
            }
#endif

            string FullPath = Dir + "/" + p;
            p = p.substr(p.find_last_of('/') + 1);
            if (paiTexture) {
                int buffer_size = paiTexture->mWidth;
                auto data = paiTexture->pcData;
                m_Materials[index].pDiffuse = gl::Texture::LoadTextureEmbedded(buffer_size, data);
            } else {
                m_Materials[index].pDiffuse = gl::Texture::LoadTexture(FullPath, p);
            }

        }
    }
}


void SkinnedMesh::LoadSpecularTexture(const string& Dir, const aiMaterial* pMaterial, int index) {
    m_Materials[index].pSpecularExponent = 0;

    if (pMaterial->GetTextureCount(aiTextureType_SHININESS) > 0) {
        aiString Path;
        if (pMaterial->GetTexture(aiTextureType_SHININESS, 0, &Path, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS) {
            string p(Path.data);
#ifdef _WIN32
            if (p.starts_with(".\\")) p = p.substr(2, p.size() - 2);
#else
            if (p.starts_with("./")) p = p.substr(2, p.size() - 2);
#endif

            string FullPath = Dir + "/" + p;
            p = p.substr(p.find_last_of('/') + 1);
            m_Materials[index].pSpecularExponent = gl::Texture::LoadTexture(FullPath, "pSpecularExponent");
        }
    }
}

void SkinnedMesh::LoadColors(const aiMaterial* pMaterial, int index) {

    aiColor4D AmbientColor(0.0f, 0.0f, 0.0f, 0.0f);
    glm::vec4 AllOnes(1.0f, 1.0f, 1.0f, 1.0);

    int ShadingModel = 0;
    if (pMaterial->Get(AI_MATKEY_SHADING_MODEL, ShadingModel) == AI_SUCCESS) {
        printf("Shading model %d\n", ShadingModel);
    }

    if (pMaterial->Get(AI_MATKEY_COLOR_AMBIENT, AmbientColor) == AI_SUCCESS) {
        m_Materials[index].AmbientColor.r = AmbientColor.r;
        m_Materials[index].AmbientColor.g = AmbientColor.g;
        m_Materials[index].AmbientColor.b = AmbientColor.b;
    } else {
        m_Materials[index].AmbientColor = AllOnes;
    }

    aiColor3D DiffuseColor(0.0f, 0.0f, 0.0f);
    if (pMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, DiffuseColor) == AI_SUCCESS) {
        m_Materials[index].DiffuseColor.r = DiffuseColor.r;
        m_Materials[index].DiffuseColor.g = DiffuseColor.g;
        m_Materials[index].DiffuseColor.b = DiffuseColor.b;
    }

    aiColor3D SpecularColor(0.0f, 0.0f, 0.0f);
    if (pMaterial->Get(AI_MATKEY_COLOR_SPECULAR, SpecularColor) == AI_SUCCESS) {
        m_Materials[index].SpecularColor.r = SpecularColor.r;
        m_Materials[index].SpecularColor.g = SpecularColor.g;
        m_Materials[index].SpecularColor.b = SpecularColor.b;
    }
}


void SkinnedMesh::PopulateBuffers() {
    glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[POS_VB]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Buffers[INDEX_BUFFER]);

    glBufferData(GL_ARRAY_BUFFER, sizeof(m_SkinnedVertices[0]) * m_SkinnedVertices.size(), &m_SkinnedVertices[0], GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(m_Indices[0]) * m_Indices.size(), &m_Indices[0], GL_STATIC_DRAW);

    size_t NumFloats = 0;
    glEnableVertexAttribArray(POSITION_LOCATION);
    glVertexAttribPointer(POSITION_LOCATION, 3, GL_FLOAT, GL_FALSE, sizeof(SkinnedVertex),
                          (const void*)(NumFloats * sizeof(float)));
    NumFloats += 3;

    glEnableVertexAttribArray(TEX_COORD_LOCATION);
    glVertexAttribPointer(TEX_COORD_LOCATION, 2, GL_FLOAT, GL_FALSE, sizeof(SkinnedVertex),
                          (const void*)(NumFloats * sizeof(float)));
    NumFloats += 2;

    glEnableVertexAttribArray(NORMAL_LOCATION);
    glVertexAttribPointer(NORMAL_LOCATION, 3, GL_FLOAT, GL_FALSE, sizeof(SkinnedVertex),
                          (const void*)(NumFloats * sizeof(float)));
    NumFloats += 3;

    glEnableVertexAttribArray(BONE_ID_LOCATION);
    glVertexAttribIPointer(BONE_ID_LOCATION, MAX_NUM_BONES_PER_VERTEX, GL_INT, sizeof(SkinnedVertex),
                           (const void*)(NumFloats * sizeof(float)));
    NumFloats += MAX_NUM_BONES_PER_VERTEX;

    glEnableVertexAttribArray(BONE_WEIGHT_LOCATION);
    glVertexAttribPointer(BONE_WEIGHT_LOCATION, MAX_NUM_BONES_PER_VERTEX, GL_FLOAT, GL_FALSE, sizeof(SkinnedVertex),
                          (const void*)(NumFloats * sizeof(float)));
}

void SkinnedMesh::Render(const glm::mat4& model,
                         const glm::mat4& view,
                         const glm::mat4& proj,
                         bool multiAnimations,
                         int startAnim,
                         int endAnim,
                         float blendFactor) {

    m_currentTime = GetCurrentTimeMillis();
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(m_shaderProg);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPolygonOffset(1.0, 1.0);
    glm::mat4 WVP = proj * view * model;
    glUniformMatrix4fv(WVPLoc, 1, GL_FALSE, glm::value_ptr(WVP));
    auto camLocPos = gl::Camera::get_position();
    glUniform3f(CameraLocalPosLoc, camLocPos.x, camLocPos.y, camLocPos.z);

    glUniform3fv(glGetUniformLocation(m_shaderProg, "dir"), 1, glm::value_ptr(gl::Camera::getLook()));

    float AnimationTimeSec = (float)((double)m_currentTime - (double)m_startTime) / 1000.0f;
    float TotalPauseTimeSec = (float)((double)m_totalPauseTime / 1000.0f);
    AnimationTimeSec -= TotalPauseTimeSec;
    static float BlendFactor = 0.0f;
    static float BlendDirection = 0.0001f;

    vector<glm::mat4> Transforms;
    if(multiAnimations) {
        GetBoneTransformsBlended(AnimationTimeSec, Transforms, startAnim, endAnim, blendFactor);
    } else {
        GetBoneTransforms(AnimationTimeSec, Transforms, 0);
    }
    for (uint i = 0 ; i < Transforms.size() ; i++) {
        if (i >= MAX_BONES) return;
        glUniformMatrix4fv(m_boneLocation[i], 1, GL_FALSE, glm::value_ptr(Transforms[i]));
    }

    BlendFactor += BlendDirection;
    constexpr float EDGE_THRESHOLD_LOW = 0.1f;
    constexpr float EDGE_THRESHOLD_HIGH = 0.9f;
    constexpr float EDGE_STEP = 0.0002f;
    constexpr float DEFAULT_STEP = 0.001f;

    const float direction_sign = (BlendDirection > 0.0f) ? 1.0f : -1.0f;
    const bool is_near_edge = BlendFactor <= EDGE_THRESHOLD_LOW || BlendFactor >= EDGE_THRESHOLD_HIGH;
    const float magnitude = is_near_edge ? EDGE_STEP : DEFAULT_STEP;
    BlendDirection = direction_sign * magnitude;

    if (BlendFactor > 1.0f || BlendFactor < 0.0f) BlendDirection *= -1.0f;
    BlendFactor = std::clamp(BlendFactor, 0.0f, 1.0f);

    glBindVertexArray(m_VAO);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPolygonOffset(1.0, 1.0);
    for (auto & m_Meshe : m_Meshes) {
        unsigned int MaterialIndex = m_Meshe.MaterialIndex;

        assert(MaterialIndex < m_Materials.size());


        if (m_Materials[MaterialIndex].pSpecularExponent) {
            glActiveTexture(GL_TEXTURE0 + 8);
            glBindTexture(GL_TEXTURE_2D, m_Materials[MaterialIndex].pSpecularExponent);

        }

        if (m_Materials[MaterialIndex].pDiffuse) {
            glActiveTexture(GL_TEXTURE0 + 0);
            glBindTexture(GL_TEXTURE_2D, m_Materials[MaterialIndex].pDiffuse);

        }
        auto mat = m_Materials[MaterialIndex];
        glUniform3f(materialLoc.AmbientColor, mat.AmbientColor.r, mat.AmbientColor.g, mat.AmbientColor.b);
        glUniform3f(materialLoc.DiffuseColor, mat.DiffuseColor.r, mat.DiffuseColor.g, mat.DiffuseColor.b);
        glUniform3f(materialLoc.SpecularColor, mat.SpecularColor.r, mat.SpecularColor.g, mat.SpecularColor.b);

        glDrawElementsBaseVertex(GL_TRIANGLES, m_Meshe.NumIndices, GL_UNSIGNED_INT,
                                 (void*)(sizeof(unsigned int) * m_Meshe.BaseIndex),
                                 m_Meshe.BaseVertex);
    }
    glBindVertexArray(0);
}


const Material& SkinnedMesh::GetMaterial() {
    for (unsigned int i = 0 ; i < m_Materials.size() ; i++) {
        if (m_Materials[i].AmbientColor != glm::vec4(0.0f)) return m_Materials[i];
    }
    return m_Materials[0];
}


uint SkinnedMesh::FindPosition(float AnimationTimeTicks, const aiNodeAnim* pNodeAnim) {

    for (uint i = 0 ; i < pNodeAnim->mNumPositionKeys - 1 ; i++) {
        float t = (float)pNodeAnim->mPositionKeys[i + 1].mTime;
        if (AnimationTimeTicks < t) {
            return i;
        }
    }
    return 0;
}


void SkinnedMesh::CalcInterpolatedPosition(aiVector3D& Out, float AnimationTimeTicks, const aiNodeAnim* pNodeAnim) {

    if (pNodeAnim->mNumPositionKeys == 1) {
        Out = pNodeAnim->mPositionKeys[0].mValue;
        return;
    }

    uint PositionIndex = FindPosition(AnimationTimeTicks, pNodeAnim);
    uint NextPositionIndex = PositionIndex + 1;
    assert(NextPositionIndex < pNodeAnim->mNumPositionKeys);
    float t1 = (float)pNodeAnim->mPositionKeys[PositionIndex].mTime;
    if (t1 > AnimationTimeTicks) {
        Out = pNodeAnim->mPositionKeys[PositionIndex].mValue;
    } else {
        float t2 = (float)pNodeAnim->mPositionKeys[NextPositionIndex].mTime;
        float DeltaTime = t2 - t1;
        float Factor = (AnimationTimeTicks - t1) / DeltaTime;
        assert(Factor >= 0.0f && Factor <= 1.0f);
        const aiVector3D& Start = pNodeAnim->mPositionKeys[PositionIndex].mValue;
        const aiVector3D& End = pNodeAnim->mPositionKeys[NextPositionIndex].mValue;
        aiVector3D Delta = End - Start;
        Out = Start + Factor * Delta;
    }
}


uint SkinnedMesh::FindRotation(float AnimationTimeTicks, const aiNodeAnim* pNodeAnim) {

    assert(pNodeAnim->mNumRotationKeys > 0);
    for (uint i = 0 ; i < pNodeAnim->mNumRotationKeys - 1 ; i++) {
        float t = (float)pNodeAnim->mRotationKeys[i + 1].mTime;
        if (AnimationTimeTicks < t) {
            return i;
        }
    }
    return 0;
}


void SkinnedMesh::CalcInterpolatedRotation(aiQuaternion& Out, float AnimationTimeTicks, const aiNodeAnim* pNodeAnim) {

    if (pNodeAnim->mNumRotationKeys == 1) {
        Out = pNodeAnim->mRotationKeys[0].mValue;
        return;
    }

    uint RotationIndex = FindRotation(AnimationTimeTicks, pNodeAnim);
    uint NextRotationIndex = RotationIndex + 1;
    assert(NextRotationIndex < pNodeAnim->mNumRotationKeys);
    float t1 = (float)pNodeAnim->mRotationKeys[RotationIndex].mTime;
    if (t1 > AnimationTimeTicks) {
        Out = pNodeAnim->mRotationKeys[RotationIndex].mValue;
    } else {
        float t2 = (float)pNodeAnim->mRotationKeys[NextRotationIndex].mTime;
        float DeltaTime = t2 - t1;
        float Factor = (AnimationTimeTicks - t1) / DeltaTime;
        assert(Factor >= 0.0f && Factor <= 1.0f);
        const aiQuaternion& StartRotationQ = pNodeAnim->mRotationKeys[RotationIndex].mValue;
        const aiQuaternion& EndRotationQ   = pNodeAnim->mRotationKeys[NextRotationIndex].mValue;
        aiQuaternion::Interpolate(Out, StartRotationQ, EndRotationQ, Factor);
    }
    Out.Normalize();
}

uint SkinnedMesh::FindScaling(float AnimationTimeTicks, const aiNodeAnim* pNodeAnim) {

    assert(pNodeAnim->mNumScalingKeys > 0);
    for (uint i = 0 ; i < pNodeAnim->mNumScalingKeys - 1 ; i++) {
        float t = (float)pNodeAnim->mScalingKeys[i + 1].mTime;
        if (AnimationTimeTicks < t) {
            return i;
        }
    }
    return 0;
}

void SkinnedMesh::CalcInterpolatedScaling(aiVector3D& Out, float AnimationTimeTicks, const aiNodeAnim* pNodeAnim) {

    if (pNodeAnim->mNumScalingKeys == 1) {
        Out = pNodeAnim->mScalingKeys[0].mValue;
        return;
    }

    uint ScalingIndex = FindScaling(AnimationTimeTicks, pNodeAnim);
    uint NextScalingIndex = ScalingIndex + 1;
    assert(NextScalingIndex < pNodeAnim->mNumScalingKeys);
    auto t1 = (float)pNodeAnim->mScalingKeys[ScalingIndex].mTime;
    if (t1 > AnimationTimeTicks) {
        Out = pNodeAnim->mScalingKeys[ScalingIndex].mValue;
    } else {
        auto t2 = (float)pNodeAnim->mScalingKeys[NextScalingIndex].mTime;
        float DeltaTime = t2 - t1;
        float Factor = (AnimationTimeTicks - t1) / DeltaTime;
        assert(Factor >= 0.0f && Factor <= 1.0f);
        const aiVector3D& Start = pNodeAnim->mScalingKeys[ScalingIndex].mValue;
        const aiVector3D& End   = pNodeAnim->mScalingKeys[NextScalingIndex].mValue;
        aiVector3D Delta = End - Start;
        Out = Start + Factor * Delta;
    }
}


void SkinnedMesh::ReadNodeHierarchy(float AnimationTimeTicks, const aiNode* pNode,
                                    const glm::mat4& ParentTransform, const aiAnimation& Animation) {

    string NodeName(pNode->mName.data);
    glm::mat4 NodeTransformation(AiToGlmMat4(pNode->mTransformation));
    const aiNodeAnim* pNodeAnim = FindNodeAnim(Animation, NodeName);

    if (pNodeAnim) {
        LocalTransform Transform;
        CalcLocalTransform(Transform, AnimationTimeTicks, pNodeAnim);
        glm::mat4 ScalingM = glm::scale(glm::mat4(1.0f), AiToGlmVec3(Transform.Scaling));
        glm::mat4 RotationM = glm::mat4(AiToGlmMat3(Transform.Rotation.GetMatrix()));
        glm::mat4 TranslationM = glm::translate(glm::mat4(1.0f), AiToGlmVec3(Transform.Translation));

        NodeTransformation = TranslationM * RotationM * ScalingM;
    }

    glm::mat4 GlobalTransformation = ParentTransform * NodeTransformation;

    if (m_BoneNameToIndexMap.contains(NodeName)) {
        uint BoneIndex = m_BoneNameToIndexMap[NodeName];
        m_BoneInfo[BoneIndex].FinalTransformation = m_GlobalInverseTransform *
                GlobalTransformation * m_BoneInfo[BoneIndex].OffsetMatrix;
    }

    for (uint i = 0 ; i < pNode->mNumChildren ; i++) {
        string ChildName(pNode->mChildren[i]->mName.data);
        ReadNodeHierarchy(AnimationTimeTicks, pNode->mChildren[i], GlobalTransformation, Animation);
    }
}

void SkinnedMesh::ReadNodeHierarchyBlended(float StartAnimationTimeTicks, float EndAnimationTimeTicks,
                                           const aiNode* pNode, const glm::mat4& ParentTransform,
                                           const aiAnimation& StartAnimation, const aiAnimation& EndAnimation,
                                           float BlendFactor) {

    string NodeName(pNode->mName.data);
    glm::mat4 NodeTransformation(AiToGlmMat4(pNode->mTransformation));
    const aiNodeAnim* pStartNodeAnim = FindNodeAnim(StartAnimation, NodeName);
    LocalTransform StartTransform;

    if (pStartNodeAnim) {
        CalcLocalTransform(StartTransform, StartAnimationTimeTicks, pStartNodeAnim);
    }

    LocalTransform EndTransform;
    const aiNodeAnim* pEndNodeAnim = FindNodeAnim(EndAnimation, NodeName);

    if ((pStartNodeAnim && !pEndNodeAnim) || (!pStartNodeAnim && pEndNodeAnim)) {
        printf("On the node %s there is an animation node for only one of the start/end animations.\n", NodeName.c_str());
        printf("This case is not supported\n");
        exit(0);
    }

    if (pEndNodeAnim) {
        CalcLocalTransform(EndTransform, EndAnimationTimeTicks, pEndNodeAnim);
    }

    if (pStartNodeAnim && pEndNodeAnim) {
        // Interpolate scaling
        const aiVector3D& Scale0 = StartTransform.Scaling;
        const aiVector3D& Scale1 = EndTransform.Scaling;
        aiVector3D BlendedScaling = (1.0f - BlendFactor) * Scale0 + Scale1 * BlendFactor;
        glm::mat4 ScalingM = glm::scale(glm::mat4(1.0f), AiToGlmVec3(BlendedScaling));

        // Interpolate rotation
        const aiQuaternion& Rot0 = StartTransform.Rotation;
        const aiQuaternion& Rot1 = EndTransform.Rotation;
        aiQuaternion BlendedRot;
        aiQuaternion::Interpolate(BlendedRot, Rot0, Rot1, BlendFactor);
        glm::mat4 RotationM = glm::mat4(AiToGlmMat3(BlendedRot.GetMatrix()));

        // Interpolate translation
        const aiVector3D& Pos0 = StartTransform.Translation;
        const aiVector3D& Pos1 = EndTransform.Translation;
        aiVector3D BlendedTranslation = (1.0f - BlendFactor) * Pos0 + Pos1 * BlendFactor;
        glm::mat4 TranslationM = glm::translate(glm::mat4(1.0f), AiToGlmVec3(BlendedTranslation));
        NodeTransformation = TranslationM * RotationM * ScalingM;
    }

    glm::mat4 GlobalTransformation = ParentTransform * NodeTransformation;

    if (m_BoneNameToIndexMap.contains(NodeName)) {
        uint BoneIndex = m_BoneNameToIndexMap[NodeName];
        m_BoneInfo[BoneIndex].FinalTransformation = m_GlobalInverseTransform *
                                                    GlobalTransformation *
                                                    m_BoneInfo[BoneIndex].OffsetMatrix;
    }

    for (uint i = 0 ; i < pNode->mNumChildren ; i++) {
        string ChildName(pNode->mChildren[i]->mName.data);
        ReadNodeHierarchyBlended(StartAnimationTimeTicks, EndAnimationTimeTicks,
                                 pNode->mChildren[i], GlobalTransformation, StartAnimation,
                                 EndAnimation, BlendFactor);
    }
}

void SkinnedMesh::CalcLocalTransform(LocalTransform& Transform, float AnimationTimeTicks, const aiNodeAnim* pNodeAnim) {
    CalcInterpolatedScaling(Transform.Scaling, AnimationTimeTicks, pNodeAnim);
    CalcInterpolatedRotation(Transform.Rotation, AnimationTimeTicks, pNodeAnim);
    CalcInterpolatedPosition(Transform.Translation, AnimationTimeTicks, pNodeAnim);
}

void SkinnedMesh::GetBoneTransforms(float TimeInSeconds, vector<glm::mat4>& Transforms, unsigned int AnimationIndex) {

    if (AnimationIndex >= pScene->mNumAnimations) {
        printf("Invalid animation index %d, max is %d\n", AnimationIndex, pScene->mNumAnimations);
        assert(0);
    }

    auto Identity = glm::mat4(1.0f);

    float AnimationTimeTicks = CalcAnimationTimeTicks(TimeInSeconds, AnimationIndex);
    const aiAnimation& Animation = *pScene->mAnimations[AnimationIndex];
    ReadNodeHierarchy(AnimationTimeTicks, pScene->mRootNode, Identity, Animation);
    Transforms.resize(m_BoneInfo.size());

    for (uint i = 0 ; i < m_BoneInfo.size() ; i++) {
        Transforms[i] = m_BoneInfo[i].FinalTransformation;
    }
}

void SkinnedMesh::GetBoneTransformsBlended(float TimeInSeconds, vector<glm::mat4>& BlendedTransforms,
                                           unsigned int StartAnimIndex, unsigned int EndAnimIndex, float BlendFactor) {

    if (StartAnimIndex >= pScene->mNumAnimations) {
        printf("Invalid start animation index %d, max is %d\n", StartAnimIndex, pScene->mNumAnimations);
        assert(0);
    }

    if (EndAnimIndex >= pScene->mNumAnimations) {
        printf("Invalid end animation index %d, max is %d\n", EndAnimIndex, pScene->mNumAnimations);
        assert(0);
    }

    if ((BlendFactor < 0.0f) || (BlendFactor > 1.0f)) {
        printf("Invalid blend factor %f\n", BlendFactor);
        assert(0);
    }

    float StartAnimationTimeTicks = CalcAnimationTimeTicks(TimeInSeconds, StartAnimIndex);
    float EndAnimationTimeTicks = CalcAnimationTimeTicks(TimeInSeconds, EndAnimIndex);

    const aiAnimation& StartAnimation = *pScene->mAnimations[StartAnimIndex];
    const aiAnimation& EndAnimation = *pScene->mAnimations[EndAnimIndex];

    glm::mat4 Identity = glm::mat4(1.0f);
    ReadNodeHierarchyBlended(StartAnimationTimeTicks, EndAnimationTimeTicks,
                             pScene->mRootNode, Identity, StartAnimation, EndAnimation, BlendFactor);

    BlendedTransforms.resize(m_BoneInfo.size());
    for (uint i = 0 ; i < m_BoneInfo.size() ; i++) {
        BlendedTransforms[i] = m_BoneInfo[i].FinalTransformation;
    }
}

float SkinnedMesh::CalcAnimationTimeTicks(float TimeInSeconds, unsigned int AnimationIndex) {
    float TicksPerSecond = (float)(pScene->mAnimations[AnimationIndex]->mTicksPerSecond != 0 ?
            pScene->mAnimations[AnimationIndex]->mTicksPerSecond : 25.0f);
    float TimeInTicks = TimeInSeconds * TicksPerSecond;
    float Duration = 0.0f;
    float fraction = modf((float)pScene->mAnimations[AnimationIndex]->mDuration, &Duration);
    float AnimationTimeTicks = fmod(TimeInTicks, Duration);
    return AnimationTimeTicks;
}

const aiNodeAnim* SkinnedMesh::FindNodeAnim(const aiAnimation& pAnimation, const string& NodeName) {
    for (uint i = 0 ; i < pAnimation.mNumChannels ; i++) {

        const aiNodeAnim* pNodeAnim = pAnimation.mChannels[i];
        if (string(pNodeAnim->mNodeName.data) == NodeName) {
            return pNodeAnim;
        }
    }
    return nullptr;
}