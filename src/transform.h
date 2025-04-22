#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

namespace gl {

    class Transform {
    public:
        Transform();

        void setScale(float scale);
        void setRotation(float x, float y, float z);
        void setRotation(const glm::vec3 &rot);
        void setPosition(float x, float y, float z);
        void setPosition(const glm::vec3 &pos);
        void rotate(float x, float y, float z);
        glm::mat4 getMatrix() const;
        glm::mat4 getReversedTranslationMatrix() const;
        glm::mat4 getReversedRotationMatrix() const;
        glm::vec3 worldPosToLocalPos(const glm::vec3& worldPos) const;
        glm::vec3 worldDirToLocalDir(const glm::vec3& worldDir) const;
        static glm::mat3 RotateMatrix(float degrees, const glm::vec3& axis);
        static void LeftRotation(float degrees, glm::vec3& eye, const glm::vec3& up);
        static void UpRotation(float degrees, glm::vec3& eye, glm::vec3& up);
        static glm::mat4 ScaleMatrix(const glm::vec3& scaleVec);
        static glm::mat4 TranslateMatrix(const glm::vec3& translateVec);
        static glm::vec3 UpVector(const glm::vec3 &up, const glm::vec3 &zvec);

    private:
        float m_scale;         // Uniform scale.
        glm::vec3 m_rotation;  // Euler angles (radians).
        glm::vec3 m_position;  // Position in world space.


    };

}


