#pragma once

#include <map>
#include <vector>
#include <GL/glew.h>
#include <cassert>
#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>       // Output data structure
#include <assimp/postprocess.h> // Post processing flags
#include <glm/glm.hpp>
// #include "worldTransform.h"

#ifdef _WIN32
#define SNPRINTF _snprintf
#else
#define SNPRINTF snprintf
#endif

#define MAX_BONES 200

#define ASSIMP_LOAD_FLAGS (aiProcess_JoinIdenticalVertices |    \
                           aiProcess_Triangulate |              \
                           aiProcess_GenSmoothNormals |         \
                           aiProcess_LimitBoneWeights |         \
                           aiProcess_SplitLargeMeshes |         \
                           aiProcess_ImproveCacheLocality |     \
                           aiProcess_RemoveRedundantMaterials | \
                           aiProcess_FindDegenerates |          \
                           aiProcess_FindInvalidData |          \
                           aiProcess_GenUVCoords |              \
                           aiProcess_CalcTangentSpace)


class Material {

public:

    std::string m_name;

    glm::vec4 AmbientColor = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
    glm::vec4 DiffuseColor = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
    glm::vec4 SpecularColor = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);

    GLuint pDiffuse = 0; // base color of the material
    GLuint pNormal = 0;
    GLuint pSpecularExponent = 0;

    float m_transparencyFactor = 1.0f;
    float m_alphaTest = 0.0f;

    ~Material() {}
};


class SkinnedMesh {
public:
    SkinnedMesh() {};
    ~SkinnedMesh();

    bool init();
    bool LoadMesh(const std::string& Filename);
    void Render(const glm::mat4& model,
                const glm::mat4& view,
                const glm::mat4& proj,
                bool multiAnimations,
                int startAnim,
                int endAnim,
                float blendFactor);

    uint NumBones() const { return (uint)m_BoneNameToIndexMap.size(); }
    const Material& GetMaterial();
    void GetBoneTransforms(float TimeInSeconds, std::vector<glm::mat4>& Transforms, unsigned int AnimationIndex);
    void GetBoneTransformsBlended(float TimeInSeconds, std::vector<glm::mat4>& BlendedTransforms,
                             unsigned int StartAnimIndex, unsigned int EndAnimIndex, float BlendFactor);

    long long m_startTime = 0;
    long long m_currentTime = 0;
    bool m_runAnimation = true;
    long long m_totalPauseTime = 0;
    long long m_pauseStart = 0;
    int m_animationIndex = 0;
    float m_blendFactor = 0.0f;

private:

#define MAX_NUM_BONES_PER_VERTEX 4
#define INVALID_MATERIAL 0xFFFFFFFF



    void Clear();
    GLenum InitFromScene(const aiScene* pScene, const std::string& Filename);
    void CountVerticesAndIndices(const aiScene* pScene, unsigned int& NumVertices, unsigned int& NumIndices);
    void ReserveSpace(unsigned int NumVertices, unsigned int NumIndices);
    void InitAllMeshes(const aiScene* pScene);
    void InitSingleMesh(uint MeshIndex, const aiMesh* paiMesh);
    bool InitMaterials(const aiScene* pScene, const std::string& Filename);
    void PopulateBuffers();

    void LoadTextures(const std::string& Dir, const aiMaterial* pMaterial, int index);
    void LoadDiffuseTexture(const std::string& Dir, const aiMaterial* pMaterial, int index);
    void LoadSpecularTexture(const std::string& Dir, const aiMaterial* pMaterial, int index);
    void LoadColors(const aiMaterial* pMaterial, int index);

    struct VertexBoneData {
        uint BoneIDs[MAX_NUM_BONES_PER_VERTEX];
        float Weights[MAX_NUM_BONES_PER_VERTEX];

        VertexBoneData(){}

        void AddBoneData(uint BoneID, float Weight) {
            for (uint i = 0 ; i < std::size(BoneIDs) ; i++) {
                if (Weights[i] == 0.0) {
                    BoneIDs[i] = BoneID;
                    Weights[i] = Weight;
                    return;
                }
            }
            assert(0);
        }
    };

    struct SkinnedVertex {
        glm::vec3 Position{};
        glm::vec2 TexCoords{};
        glm::vec3 Normal{};
        VertexBoneData Bones{};
    };
    struct LocalTransform {
        aiVector3D Scaling;
        aiQuaternion Rotation;
        aiVector3D Translation;
    };

    void LoadMeshBones(uint MeshIndex, const aiMesh* pMesh, std::vector<SkinnedVertex>& SkinnedVertices, int BaseVertex);
    void LoadSingleBone(uint MeshIndex, const aiBone* pBone, std::vector<SkinnedVertex>& SkinnedVertices, int BaseVertex);
    int GetBoneId(const aiBone* pBone);
    void CalcInterpolatedScaling(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
    void CalcInterpolatedRotation(aiQuaternion& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
    void CalcInterpolatedPosition(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
    static uint FindScaling(float AnimationTime, const aiNodeAnim* pNodeAnim);
    static uint FindRotation(float AnimationTime, const aiNodeAnim* pNodeAnim);
    uint FindPosition(float AnimationTime, const aiNodeAnim* pNodeAnim);
    const aiNodeAnim* FindNodeAnim(const aiAnimation& pAnimation, const std::string& NodeName);
    void CalcLocalTransform(LocalTransform& Transform, float AnimationTimeTicks, const aiNodeAnim* pNodeAnim);
    void ReadNodeHierarchy(float AnimationTimeTicks, const aiNode* pNode,
                           const glm::mat4& ParentTransform, const aiAnimation& Animation);
    void ReadNodeHierarchyBlended(float StartAnimationTimeTicks, float EndAnimationTimeTicks,
                                               const aiNode* pNode, const glm::mat4& ParentTransform,
                                               const aiAnimation& StartAnimation, const aiAnimation& EndAnimation,
                                               float BlendFactor);

    float CalcAnimationTimeTicks(float TimeInSeconds, unsigned int AnimationIndex);

    enum BUFFER_TYPE {
        INDEX_BUFFER = 0,
        POS_VB       = 1,
        TEXCOORD_VB  = 2,
        NORMAL_VB    = 3,
        BONE_VB      = 4,
        NUM_BUFFERS  = 5
    };

    GLuint m_VAO = 0;
    GLuint m_Buffers[NUM_BUFFERS] = { 0 };

    struct BasicMeshEntry {
        BasicMeshEntry() {
            NumIndices = 0;
            BaseVertex = 0;
            BaseIndex = 0;
            MaterialIndex = INVALID_MATERIAL;
        }

        unsigned int NumIndices;
        unsigned int BaseVertex;
        unsigned int BaseIndex;
        unsigned int MaterialIndex;
    };

    Assimp::Importer Importer;
    const aiScene* pScene = NULL;
    std::vector<BasicMeshEntry> m_Meshes;
    std::vector<Material> m_Materials;

    // Temporary space for vertex stuff before we load them into the GPU
    std::vector<glm::vec3> m_Positions;
    std::vector<glm::vec3> m_Normals;
    std::vector<glm::vec2> m_TexCoords;
    std::vector<unsigned int> m_Indices;
    std::vector<VertexBoneData> m_Bones;
    std::vector<SkinnedVertex> m_SkinnedVertices;

    std::map<std::string, uint> m_BoneNameToIndexMap;

    struct BoneInfo {
        glm::mat4 OffsetMatrix;
        glm::mat4 FinalTransformation;

        BoneInfo(const glm::mat4& Offset) {
            OffsetMatrix = Offset;
            FinalTransformation = glm::mat4(0.0f);
        }
    };

    std::vector<BoneInfo> m_BoneInfo;
    glm::mat4 m_GlobalInverseTransform;
    glm::mat4 FinalTrans;
    glm::mat4 world;

    GLuint WVPLoc;
    GLuint samplerLoc;
    GLuint samplerSpecularExponentLoc;
    GLuint CameraLocalPosLoc;

    GLuint m_boneLocation[MAX_BONES];
    GLuint m_shaderProg = 0;

    struct {
        GLuint AmbientColor;
        GLuint DiffuseColor;
        GLuint SpecularColor;
    } materialLoc;
};
