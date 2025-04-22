#pragma once

#include <iostream>
#include <string>

#ifdef _WIN32
#define SNPRINTF _snprintf
#else
#define SNPRINTF snprintf
#endif

#define INVALID_UNIFORM_LOCATION 0xffffffff
#define MAX_BONES 200

namespace gl {
class Shader{
public:
    
    static GLuint init_shaders (GLenum type, const char * filename);
    static GLuint init_program (GLuint vertexshader, GLuint fragmentshader);
    static GLint GetUniformLocation(const char* pUniformName, GLuint m_shaderProg);

private:
    static std::string read_text_file(const char * filename);
    static void program_errors (GLuint program);
    static void shader_errors (GLuint shader);

};
}