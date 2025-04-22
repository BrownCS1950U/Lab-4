#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <unordered_map>
#include <string>
#include "debug.h"
#include "tiny_obj_loader.h"

struct texture_names {
    std::string ambient_texname;             // map_Ka. For ambient or ambient occlusion.
    std::string diffuse_texname;             // map_Kd
    std::string specular_texname;            // map_Ks
    std::string specular_highlight_texname;  // map_Ns
    std::string bump_texname;                // map_bump, map_Bump, bump
    std::string alpha_texname;               // map_d
    std::string reflection_texname;
};

struct DrawObject {
    GLuint vao = 0;
    GLuint vbo = 0; // vertex buffer id
    size_t numTriangles = 0;
    size_t material_id = -1;

    glm::vec3 bmin; // Boundary Min
    glm::vec3 bmax; // Boundary Max

    // Texture related
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    glm::vec3 transmittance;
    glm::vec3 emission;
    float shininess;
    float ior;
    float dissolve;
    int illum;
    int material_size;
    texture_names texNames;
};

namespace gl {

    class DataTex {

    public:

        std::unordered_map<std::string, GLuint> textures;
        std::vector<DrawObject> m_draw_objects;
    };

    class Texture {
    public:
        static void LoadMaterials(const std::vector<tinyobj::material_t>& materials, std::string& filename, DataTex& data);
        static void BindMaterialTextures(const texture_names& mat, GLuint programId, DataTex& data);
        static void LoadTexture(std::string& filename, const std::string& texname, DataTex& data);
        static GLuint LoadTextureEmbedded(int bufferSize, void* data);
        static GLuint LoadTexture(std::string& filename, const std::string& texname);


    private:
        static std::string GetBaseDir(std::string_view filepath);
        static void FixPath(std::string &path);
    };
}