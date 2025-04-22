#pragma once

#include <glm/glm.hpp>

#include <vector>
#include <iostream>
#include <unordered_map>
#include "debug.h"
#include "texture.h"

namespace gl {

class Mesh{

public:

    static DataTex load_obj(const std::string &filename);
    static void draw(GLenum face, GLenum type, GLuint programID, gl::DataTex& data);
    static void check_errors(const std::string& desc);

};
}