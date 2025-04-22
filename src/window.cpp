
#include <vector>
#include <GL/glew.h>
#include <glm/gtc/type_ptr.hpp>
#include "window.h"

#include "shaders.h"
#include "mesh.h"
#include "camera.h"
#include <imgui.h>

#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"



glm::vec4 light_position(0.0f, 0.0f, 0.0f, 1.0f);
glm::vec4 light_color(1.0f, 0.0f, 0.0f, 1.0f);

int sAnim = 0;
int eAnim = 0;
float blendFact = 0.5f;

namespace gl {

    float Window::sense = 1.0f;
    bool Window::active_cursor = false;
    bool Window::cursorInsideWindow = true;
    GLuint Window::shaderProgram = 0;

    int Window::render_mode = 0;
    bool Window::keys[1024] = { false };
    int Window::window_width = 1920;
    int Window::window_height = 1080;
    int Window::current_vp_width = window_width / 6;
    int Window::current_vp_height = window_height;

    std::vector<gl::DataTex> Window::m_data = std::vector<gl::DataTex>();
    GLFWwindow* Window::glfwWindow = nullptr;
    SkinnedMesh Window::sMesh = SkinnedMesh();

    Window::~Window() {
        if (glfwWindow) {
            glfwDestroyWindow(glfwWindow);
        }
        glfwTerminate();
    }

    bool Window::isActive() {
        return !glfwWindowShouldClose(glfwWindow);
    }
    void Window::resize_window(GLFWwindow* window, int width, int height) {
        window_width = width;
        window_height = height;

        int vp = width / 6;

        current_vp_height = height;
        current_vp_width = vp;

        glViewport(vp, 0, width - vp, height);

        float aspect = (float) (width - vp) / (float) height;
//        glm::mat4 proj = gl::Camera::getProjection(aspect);
//        glm::mat4 modelView = gl::Camera::getViewMatrix();
//        glm::mat4 MVP = proj * modelView;
//        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "uMVP"),
//                                    1, GL_FALSE, glm::value_ptr(MVP));
    }
    void Window::keyboard(GLFWwindow* window, int key, int scancode, int action, int mods) {

        if (action == GLFW_PRESS){
            keys[key] = true;
            switch(key){

                case GLFW_KEY_ESCAPE: exit(0); break;
                case GLFW_KEY_SPACE: {
                    active_cursor = !active_cursor;

                    active_cursor ? glfwSetInputMode(glfwWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED) :
                                    glfwSetInputMode(glfwWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                } break;
                case GLFW_KEY_R: {
                    gl::Camera::reset_camera();
                    sense = 1.0f;
                } break;
                default: std::cout << "Key not covered!\n"; break;
            }
        }
        else if (action == GLFW_RELEASE) {
            keys[key] = false;
        }
    }

    void Window::scroll(GLFWwindow * window, double xoffset, double yoffset) {
        gl::Camera::processScroll(yoffset);
    }

    void Window::cursor_enter_callback(GLFWwindow* window, int entered) {
        if (entered) {
            Window::cursorInsideWindow = true;  // Cursor is inside the window
        } else {
            Window::cursorInsideWindow = false; // Cursor left the window
        }
    }

    void Window::mouse(GLFWwindow * window, double xpos, double ypos) {
        if (!Window::cursorInsideWindow) return;
        if(!active_cursor) return;
        Window::sense = 0.1f;
        gl::Camera::processMouse(xpos * Window::sense, ypos * Window::sense);
    }

    void Window::drag_drop(GLFWwindow* window, int count, const char** paths) {
        std::cout << "Dropped files: " << count << std::endl;
        for (int i = 0; i < count; i++) {
            std::cout << "File " << i + 1 << ": " << paths[i] << std::endl;
            m_data.push_back(gl::Mesh::load_obj(paths[i]));
        }
    }

    int Window::initialize(const std::string& filename) {

        // =========== INITIALIZING CAMERA ===========

        gl::Camera::updateCameraVectors();

        // =========== INITIALIZING WINDOW ===========

        if (!glfwInit()) return -1;
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#ifdef __APPLE__
        glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_FALSE); // Disable Retina scaling
#endif

        glfwWindow = glfwCreateWindow(window_width, window_height, "Scene Viewer", nullptr, nullptr);
        if (!glfwWindow) {
            const char* errorMsg;
            int err = glfwGetError(&errorMsg);
            std::cerr << "Failed to create GLFW window: " << errorMsg << std::endl;
            glfwTerminate();
            return err;
        }
        glfwMakeContextCurrent(glfwWindow);
        glfwSwapInterval(2);

        glfwSetDropCallback(glfwWindow, drag_drop);
        glfwSetCursorPosCallback(glfwWindow, mouse);
        glfwSetScrollCallback(glfwWindow, scroll);
        glfwSetKeyCallback(glfwWindow, keyboard);
        glfwSetInputMode(glfwWindow, GLFW_STICKY_KEYS, GLFW_TRUE);
        glfwSetInputMode(glfwWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        glfwSetCursorEnterCallback(glfwWindow, cursor_enter_callback);

        glfwSetWindowSizeCallback(glfwWindow, resize_window);

        // =========== INITIALIZING OPENGL ===========
        glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK) {
            std::cerr << "GLEW Initialization Failed\n";
            return -1;
        }

        std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << "\n";

        // =========== INITIALIZING IMGUI ===========
        ImGui::CreateContext();
        ImGui_ImplGlfw_InitForOpenGL(glfwWindow, true);
        ImGui_ImplOpenGL3_Init("#version 410 core");
        ImGui::StyleColorsDark();
        ImGuiStyle& style = ImGui::GetStyle();
        style.Colors[ImGuiCol_WindowBg].w = 0.7f;

        // =========== INITIALIZING SHADERS ===========
        GLuint vertexShader = gl::Shader::init_shaders(GL_VERTEX_SHADER, "../res/shaders/vertex.glsl");
        GLuint fragmentShader = gl::Shader::init_shaders(GL_FRAGMENT_SHADER, "../res/shaders/fragment.glsl");
        shaderProgram = gl::Shader::init_program(vertexShader, fragmentShader);
        glUseProgram(shaderProgram);


        sMesh.init();
        // sMesh.LoadMesh("../hip_hop/Hip_Hop_Dancing.dae");
        // sMesh.LoadMesh("../StrutWalking/StrutWalking.dae");
        sMesh.LoadMesh(filename);

//        Debug::checkGLError();
        // =========== LOADING .OBJ ===========
//        DataTex newObj = gl::Mesh::load_obj(filename);
//        m_data.push_back(newObj);

        return 1;
    }

    void Window::display() {
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        const int num_lights = 5;

        // Define your lights
        std::array<glm::vec4, num_lights> lightPosn {
                light_position,
                glm::vec4(3.f, 4.f, 5.f, 1.f),
                glm::vec4(-2.f, 1.f, 0.f, 1.f),
                glm::vec4(2.f, 2.f, 2.f, 1.f),
                glm::vec4(0.f, 0.f, 8.f, 1.f)
        };

        std::array<glm::vec4, num_lights> lightCol {
                light_color,
                glm::vec4(1.f, 1.f, 1.f, 0.2f),
                glm::vec4(1.f, 1.f, 1.f, 0.2f),
                glm::vec4(1.f, 1.f, 1.f, 0.2f),
                glm::vec4(1.f, 1.f, 1.f, 0.2f)
        };

        // Send arrays (positions & colors)
        glUniform4fv(glGetUniformLocation(shaderProgram, "light_posn"), num_lights, glm::value_ptr(lightPosn[0]));
        glUniform4fv(glGetUniformLocation(shaderProgram, "light_col"), num_lights, glm::value_ptr(lightCol[0]));

        for(DataTex& data : m_data) {
            // Compute scaling factor
            glm::mat2x3 borders = {data.m_draw_objects[0].bmin, data.m_draw_objects[0].bmax};
            float maxExtent = std::max({0.5f * (borders[1][0] - borders[0][0]),
                                        0.5f * (borders[1][1] - borders[0][1]),
                                        0.5f * (borders[1][2] - borders[0][2])});

            glm::mat4 view = gl::Camera::getViewMatrix();
            glm::mat4 proj = gl::Camera::getProjection(1920.0f / 1080.0f);
            glm:: mat4 model = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f / maxExtent));
            glm::mat4 MVP = proj * view * model;

            // Send MVP to shader

            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "uMVP"),
                               1, GL_FALSE, glm::value_ptr(MVP));

            if (gl::Window::render_mode == 0){
                gl::Mesh::draw(GL_FRONT_AND_BACK, GL_FILL, shaderProgram, data);
            }
            if (render_mode == 1){
                glLineWidth(1);
                gl::Mesh::draw(GL_FRONT_AND_BACK, GL_LINE, shaderProgram, data);
            }
            if (render_mode == 2){
                glPointSize(5);
                gl::Mesh::draw(GL_FRONT_AND_BACK, GL_POINT, shaderProgram, data);
            }
        }
    }

    void Window::update() {

        static double lastTime = glfwGetTime();
        double currentTime = glfwGetTime();
        float deltaTime = static_cast<float>(currentTime - lastTime);
        lastTime = currentTime;

        // Handle continuous movement
        float speed = 15.0f; // Adjust speed as needed

        glm::vec3 direction(0.0f);
        if (keys[GLFW_KEY_W]) direction.z += 1.0f; // Forward
        if (keys[GLFW_KEY_S]) direction.z -= 1.0f; // Backward
        if (keys[GLFW_KEY_A]) direction.x -= 1.0f; // Left
        if (keys[GLFW_KEY_D]) direction.x += 1.0f; // Right
        if (keys[GLFW_KEY_E]) direction.y += 1.0f; // Up
        if (keys[GLFW_KEY_Q]) direction.y -= 1.0f; // Down

        if (glm::length(direction) > 0) {
            direction = glm::normalize(direction);
            gl::Camera::move(direction, speed * deltaTime);
        }
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::SetNextWindowPos({0, 0});

        ////////////////////////////////////////////////////////////////////////////////////////////////
//        display();
        glm::mat4 view = gl::Camera::getViewMatrix();
        glm::mat4 proj = gl::Camera::getProjection(1920.0f / 1080.0f);

        glm::mat4 model = glm::rotate(
          glm::scale(glm::mat4(1.0f), glm::vec3(0.1f)),
          glm::radians(180.0f), glm::vec3(1,0,-1)
        );
        model = glm::translate(model, glm::vec3(-100.0f, 50.0f, 500.0f));
        sMesh.Render(model, view, proj, true, sAnim, eAnim, blendFact);
        ImGui::Begin("Object Properties");
        ImGui::Text("Application %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::Text(" ");

        ////////////////////////////////////////////////////////////////////////////////////////////////

        ImGui::Separator(); ImGui::TextColored({0.0f,1.0f,1.0f,1.0f}, "Render Mode"); ImGui::Separator();
        ImGui::Text("Primitive object ");
        ImGui::Button("Smooth", ImVec2(50.0f, 25.0f)) ? render_mode = 0 : 0; ImGui::SameLine();
        ImGui::Button("Lines", ImVec2(50.0f, 25.0f)) ? render_mode = 1 : 0; ImGui::SameLine();
        ImGui::Button("Pnt Cld", ImVec2(50.0f, 25.0f)) ? render_mode = 2 : 0; ImGui::SameLine();

        ////////////////////////////////////////////////////////////////////////////////////////////////

        ImGui::Separator(); ImGui::TextColored({0.0f, 1.0f, 1.0f, 1.0f}, "Control Instructions"); ImGui::Separator();
        ImGui::Text("");
        ImGui::Text("Drag & Drop Your .OBJ file!");
        ImGui::Text("");
        ImGui::Text("   Up : W | S : Down");
        ImGui::Text(" Left : A | D : Right");
        ImGui::Text(" Back : Q | E : Front");
        ImGui::Text("ESC to Close");
        ImGui::Text("SPACE to activate mouse");
        ImGui::Text("Mouse for camera rotations");

        ImGui::Separator();

        ////////////////////////////////////////////////////////////////////////////////////////////////
        auto pos = gl::Camera::get_position();
        auto rot = gl::Camera::get_rotation();
        ImGui::Text(" ");
        ImGui::Separator(); ImGui::TextColored({0.0f, 1.0f, 1.0f, 1.0f}, "Camera"); ImGui::Separator();
        ImGui::SliderFloat("Field of view", &gl::Camera::fov, 0.0f, 180.0f);
        ImGui::SliderFloat("Frustum near plane", &gl::Camera::near, 0.0f, 15.0f);
        ImGui::SliderFloat("Frustum far plane", &gl::Camera::far, 0.0f, 150.0f); ImGui::Separator();
        ImGui::Text("Camera Position: ");
        ImGui::Text("X: %.1f, Y: %.1f, Z: %.1f", pos.x, pos.y, pos.z); ImGui::Separator();
        ImGui::Text("Camera Rotation: ");
        ImGui::Text("Roll: %.1f, Pitch: %.1f, Yaw: %.1f", rot.x, rot.y, rot.z); ImGui::Separator();
        ////////////////////////////////////////////////////////////////////////////////////////////////

        ImGui::Separator(); ImGui::TextColored({0.0f, 1.0f, 1.0f, 1.0f}, "Lighting"); ImGui::Separator();
        ImGui::Text("Most lights are switched off by default, and the below");
        ImGui::Text("sliders can play with the light positions and color intensities");
        ImGui::SliderFloat("Light position x: ", &light_position.x, -1000.0f, 1000.0f);
        ImGui::SliderFloat("Light position y: ", &light_position.y, -1000.0f, 1000.0f);
        ImGui::SliderFloat("Light position z: ", &light_position.z, -1000.0f, 1000.0f);
        ////////////////////////////////////////////////////////////////////////////////////////////////
        ImGui::Separator(); ImGui::TextColored({0.0f, 1.0f, 1.0f, 1.0f}, "Animation"); ImGui::Separator();
        ImGui::Text("Change the animations below");
        ImGui::Text("Change the starting and ending animation and the blend factor individually");
        ImGui::SliderInt("Starting Animation: ", &sAnim, 0, 3);
        ImGui::SliderInt("Ending Animation: ", &eAnim, 0, 3);
        ImGui::SliderFloat("Blend Factor: ", &blendFact, 0.0f, 1.0f);

        ////////////////////////////////////////////////////////////////////////////////////////////////

        ImGui::End();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glViewport(current_vp_width, 0, window_width - current_vp_width, window_height);

        ////////////////////////////////////////////////////////////////////////////////////////////////

        glfwSwapBuffers(glfwWindow);

        glfwPollEvents();
    }
}