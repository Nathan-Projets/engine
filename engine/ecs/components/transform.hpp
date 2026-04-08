#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "../component.hpp"

class Transform : public Component
{
public:
    Transform() : position(0.0f), rotation(0.0f), scale(1.0f) {}

    Transform(glm::vec3 pos, glm::vec3 rot = glm::vec3(0.0f), glm::vec3 scl = glm::vec3(1.0f)) : position(pos), rotation(rot), scale(scl)
    {
    }

    glm::mat4 GetMatrix() const
    {
        glm::mat4 mat = glm::mat4(1.0f);
        mat = glm::translate(mat, position);
        if (hasRotationOverride)
        {
            mat *= glm::mat4_cast(rotationOverride);
        }
        else
        {
            mat = glm::rotate(mat, glm::radians(rotation.x), glm::vec3(1, 0, 0));
            mat = glm::rotate(mat, glm::radians(rotation.y), glm::vec3(0, 1, 0));
            mat = glm::rotate(mat, glm::radians(rotation.z), glm::vec3(0, 0, 1));
        }
        mat = glm::scale(mat, scale);
        return mat;
    }

    void SetRotationOverride(const glm::quat &value)
    {
        rotationOverride = glm::normalize(value);
        hasRotationOverride = true;
    }

    void ClearRotationOverride()
    {
        rotationOverride = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        hasRotationOverride = false;
    }

    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;
    glm::quat rotationOverride = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    bool hasRotationOverride = false;
};
