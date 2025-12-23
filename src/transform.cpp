#include "transform.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp> // (Optional) additional glm functionality if needed

namespace gl {

// Constructor: initializes scale to 1 and rotation and position to zero vectors.
    Transform::Transform()
            : m_scale(1.0f), m_rotation(0.0f), m_position(0.0f)
    {}

// Instance method implementations

    void Transform::setScale(float scale) {
        m_scale = scale;
    }

    void Transform::setRotation(float x, float y, float z) {
        m_rotation = glm::vec3(x, y, z);
    }

    void Transform::setRotation(const glm::vec3 &rot) {
        m_rotation = rot;
    }

    void Transform::setPosition(float x, float y, float z) {
        m_position = glm::vec3(x, y, z);
    }

    void Transform::setPosition(const glm::vec3 &pos) {
        m_position = pos;
    }

    void Transform::rotate(float x, float y, float z) {
        m_rotation += glm::vec3(x, y, z);
    }

    glm::mat4 Transform::getMatrix() const {
        glm::mat4 scaleMatrix = glm::mat4(m_scale);
        glm::mat4 rotationMatrix = glm::mat4(1.0f);
        // Apply rotations in X, Y, Z order (angles assumed to be in radians)
        rotationMatrix = glm::rotate(rotationMatrix, m_rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
        rotationMatrix = glm::rotate(rotationMatrix, m_rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
        rotationMatrix = glm::rotate(rotationMatrix, m_rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));

        glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), m_position);
        return translationMatrix * rotationMatrix * scaleMatrix;
    }

    glm::mat4 Transform::getReversedTranslationMatrix() const {
        return glm::translate(glm::mat4(1.0f), -m_position);
    }

    glm::mat4 Transform::getReversedRotationMatrix() const {
        glm::mat4 reversed = glm::mat4(1.0f);
        // Reverse the rotations in the opposite order.
        reversed = glm::rotate(reversed, -m_rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
        reversed = glm::rotate(reversed, -m_rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
        reversed = glm::rotate(reversed, -m_rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
        return reversed;
    }

    glm::vec3 Transform::worldPosToLocalPos(const glm::vec3& worldPos) const {
        glm::mat4 worldToLocal = getReversedRotationMatrix() * getReversedTranslationMatrix();
        glm::vec4 localPos4f = worldToLocal * glm::vec4(worldPos, 1.0f);
        return glm::vec3(localPos4f);
    }

    glm::vec3 Transform::worldDirToLocalDir(const glm::vec3& worldDir) const {
        // Extract the upper-left 3x3 matrix from the world matrix.
        glm::mat3 worldMat3(getMatrix());
        // For rotation (with uniform scale) the inverse is the transpose.
        glm::mat3 worldToLocal = glm::transpose(worldMat3);
        glm::vec3 localDir = worldToLocal * worldDir;
        return glm::normalize(localDir);
    }

// Static utility function implementations

    glm::mat3 Transform::RotateMatrix(float degrees, const glm::vec3& axis) {
        const float radian = degrees * (glm::pi<float>() / 180.0f);
        glm::mat3 dot = glm::outerProduct(axis, axis);
        glm::mat3 cross = glm::mat3(
                0,      -axis.z,  axis.y,
                axis.z,  0,      -axis.x,
                -axis.y,  axis.x,  0
        );
        return glm::mat3(cos(radian)) * glm::mat3(1.0f)
               + (1.0f - cos(radian)) * dot
               + sin(radian) * glm::transpose(cross);
    }

    void Transform::LeftRotation(float degrees, glm::vec3& eye, const glm::vec3& up) {
        glm::mat3 rotation = RotateMatrix(degrees, glm::normalize(up));
        eye = rotation * eye;
    }

    void Transform::UpRotation(float degrees, glm::vec3& eye, glm::vec3& up) {
        glm::vec3 axis_for_up = glm::cross(glm::normalize(up), glm::normalize(eye));
        glm::mat3 rotation = RotateMatrix(degrees, glm::normalize(axis_for_up));
        // Multiply using the convention consistent with your application.
        eye = eye * rotation;
        up = up * rotation;
    }

    glm::mat4 Transform::ScaleMatrix(const glm::vec3& scaleVec) {
        glm::mat4 ret = glm::transpose(glm::mat4(
                scaleVec.x, 0,          0,          0,
                0,          scaleVec.y, 0,          0,
                0,          0,          scaleVec.z, 0,
                0,          0,          0,          1
        ));
        return ret;
    }

    glm::mat4 Transform::TranslateMatrix(const glm::vec3& translateVec) {
        glm::mat4 ret = glm::transpose(glm::mat4(
                1, 0, 0, translateVec.x,
                0, 1, 0, translateVec.y,
                0, 0, 1, translateVec.z,
                0, 0, 0, 1
        ));
        return ret;
    }

    glm::vec3 Transform::UpVector(const glm::vec3 &up, const glm::vec3 &zvec) {
        glm::vec3 x = glm::cross(up, zvec);
        glm::vec3 y = glm::cross(zvec, x);
        return glm::normalize(y);
    }

} // namespace gl
