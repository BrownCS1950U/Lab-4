#include "camera.h"
#include <iostream>
namespace gl {

    glm::vec3 Camera::position(0.0f, 0.0f, 0.0f);
    glm::vec3 Camera::worldUp(0.0f, 1.0f, 0.0f);
    glm::vec3 Camera::rotation(0.0f, -90.0f, 0.0f);

    glm::vec3 Camera::front(0.0f, 0.0f, 1.0f);
    glm::vec3 Camera::up(0.0f, 1.0f, 0.0f);
    glm::vec3 Camera::right(1.0f, 0.0f, 0.0f);

    float Camera::near = 0.1f;
    float Camera::far = 100.0f;
    float Camera::fov = 45.0f;

    glm::mat4 Camera::getViewMatrix() {
        return glm::lookAt(position, position + front, up);
    }

    glm::mat4 Camera::getProjection(float aspect) {
        return glm::perspective(glm::radians(fov), aspect, near, far);
    }

    void Camera::reset_camera() {
        Camera::position = glm::vec3(0.0f, 5.0f, 0.0f);
        Camera::worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
        Camera::rotation = glm::vec3(0.0f, -90.0f, 0.0f);

        Camera::near = 0.1f;
        Camera::far = 100.0f;
        Camera::fov = 45.0f;
    }

    void Camera::move(glm::vec3 direction, float velocity) {
        position += front * direction.z * velocity;   // Move forward/backward
        position += right * direction.x * velocity;   // Strafe left/right
        position += up * direction.y * velocity;
    }

    glm::vec3 Camera::get_position() {
        return Camera::position;
    }

    glm::vec3 Camera::get_rotation() {
        return Camera::rotation;
    }

    void Camera::processMouse(double xpos, double ypos, bool constrainPitch) {
        static bool firstMouse = true;
        static double lastX = xpos, lastY = ypos;

        if (firstMouse) {
            lastX = xpos;
            lastY = ypos;
            firstMouse = false;
        }

        float xOffset = (float) lastX - xpos;
        float yOffset = (float) lastY - ypos;
        lastX = xpos;
        lastY = ypos;

        rotation.x += yOffset;
        rotation.y += xOffset;

        if (constrainPitch) {
            rotation.x = glm::clamp(rotation.x, -89.0f, 89.0f);
        }

        updateCameraVectors();
    }

    void Camera::processScroll(double yoffset) {
        fov -= (float) yoffset;
        if (fov < 1.0f) fov = 1.0f;
        if (fov > 45.0f) fov = 75.0f;
    }

    void Camera::updateCameraVectors() {
        glm::vec3 newFront;
        newFront.x = sin(glm::radians(rotation.y)) * cos(glm::radians(rotation.x));
        newFront.y = sin(glm::radians(rotation.x));
        newFront.z = cos(glm::radians(rotation.y)) * cos(glm::radians(rotation.x));

        front = glm::normalize(newFront);

        right = glm::normalize(glm::cross(front, worldUp));
        up = glm::normalize(glm::cross(right, front));
    }

    glm::vec3 Camera::getLook() {
        return front;
    }
}

