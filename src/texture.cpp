#include "texture.h"
#include <filesystem>
#include <GL/glew.h>
#include "tiny_obj_loader.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace gl {

    void Texture::LoadMaterials(const std::vector<tinyobj::material_t>& materials, std::string& filename, DataTex& data) {
        for (const auto& mat : materials) {
            if (!mat.ambient_texname.empty()) LoadTexture(filename, mat.ambient_texname, data);
            if (!mat.diffuse_texname.empty()) LoadTexture(filename, mat.diffuse_texname, data);
            if (!mat.specular_texname.empty()) LoadTexture(filename, mat.specular_texname, data);
            if (!mat.specular_highlight_texname.empty()) LoadTexture(filename, mat.specular_highlight_texname, data);
            if (!mat.bump_texname.empty()) LoadTexture(filename, mat.bump_texname, data);
            if (!mat.alpha_texname.empty()) LoadTexture(filename, mat.alpha_texname, data);
            if (!mat.reflection_texname.empty()) LoadTexture(filename, mat.reflection_texname, data);
        }
    }

    void Texture::BindMaterialTextures(const texture_names& mat, GLuint programId, DataTex& data) {
        auto TryBind = [&programId, &data](const std::string& texName, const std::string& uniformName, int unit) {
            if (!texName.empty() && data.textures.contains(texName)) {
                glActiveTexture(GL_TEXTURE0 + unit);
                glBindTexture(GL_TEXTURE_2D, data.textures.at(texName));
                glUniform1i(glGetUniformLocation(programId, uniformName.c_str()), unit);
            }
        };

        TryBind(mat.ambient_texname, "u_ambientTex", 0);
        TryBind(mat.diffuse_texname, "u_diffuseTex", 1);
        TryBind(mat.specular_texname, "u_specularTex", 2);
        TryBind(mat.specular_highlight_texname, "u_specularHighTex", 3);
        TryBind(mat.bump_texname, "u_bumpTex", 4);
        TryBind(mat.reflection_texname, "u_reflectionTex", 5);
        TryBind(mat.alpha_texname, "u_alphaTex", 6);
    }

    void Texture::LoadTexture(std::string& filename, const std::string& texname, DataTex& data) {
        FixPath(filename);
        std::filesystem::path texPath = texname;
        if (texPath.empty() || data.textures.contains(texPath.string())) return;
        data.textures[texname] = LoadTexture(filename, texname);
    }

    GLuint Texture::LoadTexture(std::string& filename, const std::string& texname) {
        FixPath(filename);
        std::filesystem::path texPath = texname;
        std::string baseDir = GetBaseDir(filename);
        if (baseDir.empty()) baseDir = ".";

        if (!std::filesystem::exists(texPath)) {
            texPath = std::filesystem::path(baseDir) / texPath;
            std::string newPath = texPath.string();
            FixPath(newPath);
            texPath = newPath;
            if (!std::filesystem::exists(texPath)) {
                std::cerr << "Texture not found: " << texPath << "\n";
                exit(1);
            }
        }

        int width, height, channels;
        unsigned char* image = stbi_load(texPath.string().c_str(), &width, &height, &channels, STBI_default);
        if (!image) {
            std::cerr << "Failed to load texture: " << texPath << "\n";
            exit(1);
        }

        GLuint textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        GLenum format = GL_RGB;
        if (channels == 1) format = GL_RED;
        else if (channels == 2) format = GL_RG;
        else if (channels == 4) format = GL_RGBA;

        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, image);
        glBindTexture(GL_TEXTURE_2D, 0);
        stbi_image_free(image);

        return textureID;
    }

    GLuint Texture::LoadTextureEmbedded(int bufferSize, void* data) {
        int width, height, channels;
        void* image = stbi_load_from_memory((const stbi_uc*)data, bufferSize, &width, &height, &channels, 0);
        GLuint textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        GLenum format = GL_RGB;
        if (channels == 1) format = GL_RED;
        else if (channels == 2) format = GL_RG;
        else if (channels == 4) format = GL_RGBA;

        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, image);
        glBindTexture(GL_TEXTURE_2D, 0);
        stbi_image_free(image);
        return textureID;
    }

    std::string Texture::GetBaseDir(std::string_view filepath) {
        size_t pos = filepath.find_last_of("/\\");
        return (pos != std::string::npos) ? std::string(filepath.substr(0, pos)) : "";
    }

    void Texture::FixPath(std::string& path) {
#ifdef _WIN32
        std::ranges::replace(path, '/', '\\');
#else
        std::ranges::replace(path, '\\', '/');
#endif
    }
}