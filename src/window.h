#pragma once


#include "mesh.h"
#include "animations/skinnedMesh.h"
#include <GLFW/glfw3.h>

namespace gl {
class Window {
public:

    ~Window();
    static void resize_window(GLFWwindow* window, int width, int height);
    static void keyboard(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void scroll(GLFWwindow * window, double xoffset, double yoffset);
    static void cursor_enter_callback(GLFWwindow* window, int entered);
    static void mouse(GLFWwindow * window, double xpos, double ypos);
    static void drag_drop(GLFWwindow * window, int count, const char** paths);
    static int initialize(const std::string& filename);
    static void display();
    static void update();
    static bool isActive();

private:
    // Variables to hold state
    static float sense;
    static bool active_cursor;
    static bool cursorInsideWindow;
    static GLuint shaderProgram;
    static int render_mode;
    static bool keys[1024];
    static int window_width, window_height;
    static int current_vp_height, current_vp_width;

    static GLFWwindow* glfwWindow;
    static std::vector<gl::DataTex> m_data;
    static SkinnedMesh sMesh;
};
}