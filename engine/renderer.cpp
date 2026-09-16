#include "renderer.h"

#include <glm/gtc/matrix_inverse.hpp>

#include <algorithm>

namespace engine {

bool Renderer::init() {
    if (!scene_.loadFromFiles("shaders/scene.vert", "shaders/scene.frag")) return false;
    if (!sky_.loadFromFiles("shaders/sky.vert", "shaders/sky.frag")) return false;
    glGenVertexArrays(1, &emptyVao_);  // core profile needs a bound VAO even without attributes
    return true;
}

void Renderer::beginFrame(const Camera& camera, const Environment& env) {
    camera_ = camera;
    env_ = env;

    glClear(GL_DEPTH_BUFFER_BIT);
    scene_.use();
    scene_.setMat4("uViewProj", camera_.viewProjection());
    scene_.setVec3("uCamPos", camera_.position);
    scene_.setVec3("uLightDir", glm::normalize(env_.lightDirection));
    scene_.setVec3("uLightColor", env_.lightColor);
    scene_.setVec3("uAmbient", env_.ambient);
    scene_.setVec3("uFogColor", env_.fogColor);
    scene_.setFloat("uFogDensity", env_.fogDensity);

    int count = std::min<int>(static_cast<int>(env_.pointLights.size()), MAX_POINT_LIGHTS);
    glm::vec3 positions[MAX_POINT_LIGHTS];
    glm::vec3 colors[MAX_POINT_LIGHTS];
    float ranges[MAX_POINT_LIGHTS];
    for (int i = 0; i < count; ++i) {
        positions[i] = env_.pointLights[i].position;
        colors[i] = env_.pointLights[i].color;
        ranges[i] = std::max(env_.pointLights[i].range, 0.01f);
    }
    scene_.setInt("uPointCount", count);
    scene_.setVec3Array("uPointPos", positions, count);
    scene_.setVec3Array("uPointColor", colors, count);
    scene_.setFloatArray("uPointRange", ranges, count);
}

void Renderer::drawSky() {
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    sky_.use();
    sky_.setMat4("uInvViewProj", glm::inverse(camera_.viewProjection()));
    sky_.setVec3("uCamPos", camera_.position);
    sky_.setVec3("uZenith", env_.skyZenith);
    sky_.setVec3("uHorizon", env_.skyHorizon);
    sky_.setVec3("uGround", env_.groundColor);
    sky_.setVec3("uSunDir", glm::normalize(env_.sunDirection));
    sky_.setVec3("uSunColor", env_.sunColor);
    sky_.setFloat("uNight", env_.night);

    glBindVertexArray(emptyVao_);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);

    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    scene_.use();
}

void Renderer::draw(const Mesh& mesh, const glm::mat4& model, const Material& material) {
    if (mesh.empty()) return;
    scene_.use();
    scene_.setMat4("uModel", model);
    scene_.setMat3("uNormalMat", glm::inverseTranspose(glm::mat3(model)));
    scene_.setVec3("uTint", material.tint);
    scene_.setFloat("uEmissive", material.emissive);
    scene_.setFloat("uSpecular", material.specular);
    scene_.setFloat("uShininess", material.shininess);
    mesh.draw();
}

}  // namespace engine
