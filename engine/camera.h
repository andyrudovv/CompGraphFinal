#ifndef ENGINE_CAMERA_H
#define ENGINE_CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace engine {

struct Camera {
    glm::vec3 position{0.0f, 5.0f, 10.0f};
    glm::vec3 target{0.0f, 0.0f, 0.0f};
    glm::vec3 up{0.0f, 1.0f, 0.0f};
    float fovDegrees = 60.0f;
    float nearPlane = 0.1f;
    float farPlane = 400.0f;
    float aspect = 16.0f / 9.0f;

    glm::mat4 view() const { return glm::lookAt(position, target, up); }
    glm::mat4 projection() const {
        return glm::perspective(glm::radians(fovDegrees), aspect, nearPlane, farPlane);
    }
    glm::mat4 viewProjection() const { return projection() * view(); }
};

}  // namespace engine

#endif
