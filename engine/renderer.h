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

// Keep in sync with MAX_POINT_LIGHTS in shaders/scene.frag.
const int MAX_POINT_LIGHTS = 8;

struct Material {
    glm::vec3 tint{1.0f};
    float emissive = 0.0f;   // 1 = ignore lighting, used for lamps and tail lights
    float specular = 0.25f;
    float shininess = 32.0f;

    // Optional texture. The final surface colour is vertex colour * tint *
    // texture, with the UVs computed as uv * uvScale + uvOffset (repeat
    // wrapping), so one mesh can be tiled differently per draw call and a moving
    // surface only needs its offset changed.
    const Texture* texture = nullptr;
    glm::vec2 uvScale{1.0f};
    glm::vec2 uvOffset{0.0f};
    // 0..1: how strongly the texture's alpha channel (lit windows) glows,
    // independent of the lighting. Driven by the day/night cycle.
    float textureGlow = 0.0f;
};

struct PointLight {
    glm::vec3 position{0.0f};
    glm::vec3 color{1.0f};
    float range = 20.0f;
};

// Everything the scene shader needs besides the geometry itself.
struct Environment {
    glm::vec3 sunDirection{0.0f, 1.0f, 0.0f};  // astronomical sun, drives the sky disc
    glm::vec3 sunColor{1.0f};
    glm::vec3 lightDirection{0.0f, 1.0f, 0.0f};  // towards the sun by day, the moon by night
    glm::vec3 lightColor{1.0f};
    glm::vec3 ambient{0.3f};
    glm::vec3 fogColor{0.6f, 0.7f, 0.8f};
    float fogDensity = 0.006f;
    glm::vec3 skyZenith{0.3f, 0.5f, 0.9f};
    glm::vec3 skyHorizon{0.8f, 0.85f, 0.95f};
    glm::vec3 groundColor{0.15f, 0.16f, 0.14f};
    float night = 0.0f;  // 0 day .. 1 night, drives stars and the moon
    std::vector<PointLight> pointLights;
};

class Renderer {
public:
    bool init();

    void beginFrame(const Camera& camera, const Environment& env);
    void drawSky();  // full-screen gradient with sun, moon and stars
    void draw(const Mesh& mesh, const glm::mat4& model, const Material& material);

private:
    Shader scene_;
    Shader sky_;
    GLuint emptyVao_ = 0;
    Camera camera_;
    Environment env_;
};

}  // namespace engine

#endif
