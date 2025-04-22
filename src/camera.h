#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace gl {
    class Camera {
    public:

        static glm::mat4 getViewMatrix();
        static glm::mat4 getProjection(float aspect);
        static void reset_camera();
        static void move(glm::vec3 direction, float velocity);
        static glm::vec3 get_position();
        static glm::vec3 get_rotation();
        static void processMouse(double xpos, double ypos, bool constrainPitch = false);
        static void processScroll(double yoffset);
        static void updateCameraVectors();
        static glm::vec3 getLook();
        static float near;
        static float far;
        static float fov;

    private:

        static glm::vec3 position;
        static glm::vec3 front;
        static glm::vec3 up;
        static glm::vec3 right;
        static glm::vec3 worldUp;
        static glm::vec3 rotation;

    };
}