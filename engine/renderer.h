// Forward renderer data: materials, point lights, spot lights and the environment (sun,
// ambient light, fog, sky colours, lighting toggle).

#ifndef ENGINE_RENDERER_H
#define ENGINE_RENDERER_H

#include <glm/glm.hpp>

#include <string>
#include <vector>

#include "camera.h"
#include "mesh.h"
#include "shader.h"
#include "texture.h"

namespace engine {

const int MAX_POINT_LIGHTS = 8;

struct Material {
    glm::vec3 tint{1.0f};
    float emissive = 0.0f;
    float specular = 0.25f;
    float shininess = 32.0f;

    const Texture* texture = nullptr;
    glm::vec2 uvScale{1.0f};
    glm::vec2 uvOffset{0.0f};
    float textureGlow = 0.0f;
    float sway = 0.0f;
};

struct PointLight {
    glm::vec3 position{0.0f};
    glm::vec3 color{1.0f};
    float range = 20.0f;
};

const int MAX_SPOT_LIGHTS = 4;

struct SpotLight {
    glm::vec3 position{0.0f};
    glm::vec3 direction{0.0f, 0.0f, -1.0f};
    glm::vec3 color{1.0f};
    float range = 40.0f;
    float innerAngle = 0.26f;
    float outerAngle = 0.55f;
};

struct Environment {
    bool lightingEnabled = true;
    glm::vec3 sunDirection{0.0f, 1.0f, 0.0f};
    glm::vec3 sunColor{1.0f};
    glm::vec3 lightDirection{0.0f, 1.0f, 0.0f};
    glm::vec3 lightColor{1.0f};
    glm::vec3 ambient{0.3f};
    glm::vec3 fogColor{0.6f, 0.7f, 0.8f};
    float fogDensity = 0.006f;
    glm::vec3 skyZenith{0.3f, 0.5f, 0.9f};
    glm::vec3 skyHorizon{0.8f, 0.85f, 0.95f};
    glm::vec3 groundColor{0.15f, 0.16f, 0.14f};
    float night = 0.0f;
    float time = 0.0f;
    std::vector<PointLight> pointLights;
    std::vector<SpotLight> spotLights;
};

class Renderer {
public:
    bool init();

    void beginFrame(const Camera& camera, const Environment& env);
    void drawSky();
    void draw(const Mesh& mesh, const glm::mat4& model, const Material& material);

private:
    Shader scene_;
    Shader sky_;
    GLuint emptyVao_ = 0;
    Camera camera_;
    Environment env_;
};

}

#endif
