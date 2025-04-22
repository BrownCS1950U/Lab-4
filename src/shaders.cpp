
#include <fstream>
#include <stdexcept>
#include <GL/glew.h>
#include "shaders.h"

namespace gl {
std::string Shader::read_text_file(const char * filename) {
    std::string line;
    std::string content;
    std::ifstream fileStream(filename, std::ios::in);

    if(!fileStream.is_open()) {
        throw std::runtime_error("Could not read file " + std::string(filename) + ".");
    }

    while (std::getline(fileStream, line)){
        content.append(line + "\n");
    }

    fileStream.close();
    return content;
}

void Shader::program_errors (const GLuint program){
    GLint length;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);

    std::string log(length, '\0');
    glGetProgramInfoLog(program, length, &length, &log[0]);
    glDeleteProgram(program);
    throw std::runtime_error("Compile Error, Log Below: \n" + log + "\n");
}

void Shader::shader_errors (const GLuint shader){
    GLint length;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);

    std::string log(length, '\0');
    glGetShaderInfoLog(shader, length, nullptr, &log[0]);
    glDeleteShader(shader);
    throw std::runtime_error("Compile Error, Log Below: \n" + log + "\n");
}

GLuint Shader::init_shaders (GLenum type, const char *filename){
    GLuint shader = glCreateShader(type);
    GLint compiled;

    std::string str = read_text_file(filename);
    const char * cstr = str.c_str();

    glShaderSource (shader, 1, &cstr, nullptr);
    glCompileShader (shader);
    glGetShaderiv (shader, GL_COMPILE_STATUS, &compiled);

    if (!compiled){
        shader_errors(shader);
        throw std::runtime_error("Shader did not compile!");
    }
    return shader;
}

GLuint Shader::init_program (GLuint vertexshader, GLuint fragmentshader){
    GLint linked;
    GLuint program = glCreateProgram();

    glAttachShader(program, vertexshader);
    glAttachShader(program, fragmentshader);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &linked);

    if (linked) glUseProgram(program);
    else {
        program_errors(program);
        throw std::runtime_error("Shader program did not link correctly!");
    }
    return program;
}

    GLint Shader::GetUniformLocation(const char* pUniformName, GLuint m_shaderProg) {
    GLuint Location = glGetUniformLocation(m_shaderProg, pUniformName);

    if (Location == INVALID_UNIFORM_LOCATION) {
        fprintf(stderr, "Warning! Unable to get the location of uniform '%s'\n", pUniformName);
    }

    return Location;
}


}