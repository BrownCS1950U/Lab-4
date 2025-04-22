#pragma once

#include <GL/glew.h>

#include <iostream>
#include <chrono>
#include <string>
#include <utility>

namespace Debug{
    inline GLenum checkGLError(const char *file, int line)
    {
        GLenum errorCode;
        while ((errorCode = glGetError()) != GL_NO_ERROR)
        {
            std::string error;
            switch (errorCode)
            {
                case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
                case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
                case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
                case GL_STACK_OVERFLOW:                error = "STACK_OVERFLOW"; break;
                case GL_STACK_UNDERFLOW:               error = "STACK_UNDERFLOW"; break;
                case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
                case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
                default: error = "Error not covered in debug class!"; break;
            }
            std::cout << error << " | " << file << " (" << line << ")" << std::endl;
        }
        return errorCode;
    }

    #define checkGLError() checkGLError(__FILE__, __LINE__)

    class Timer {
    public:
        explicit Timer(std::string message = "")
                : m_message(std::move(message)),
                  m_start_timepoint(std::chrono::high_resolution_clock::now()) {
        }

        inline ~Timer() {
            auto end_timepoint = std::chrono::high_resolution_clock::now();
            long long start = std::chrono::time_point_cast<std::chrono::microseconds>(m_start_timepoint).time_since_epoch().count();
            long long end = std::chrono::time_point_cast<std::chrono::microseconds>(end_timepoint).time_since_epoch().count();
            long long duration = end - start;
            if (!m_message.empty()){
                std::cout << m_message << " took: ";
            }
            std::cout << duration << "us\n";
        }

    private:
        std::string m_message;
        std::chrono::time_point<std::chrono::high_resolution_clock> m_start_timepoint;
    };

}