#include "mesh.h"

#include <glm/gtc/matrix_inverse.hpp>

#include <cmath>
#include <cstddef>
#include <utility>

namespace engine {

Mesh::~Mesh() { destroy(); }

Mesh::Mesh(Mesh&& other) noexcept
    : vao_(other.vao_), vbo_(other.vbo_), ebo_(other.ebo_), indexCount_(other.indexCount_) {
    other.vao_ = other.vbo_ = other.ebo_ = 0;
    other.indexCount_ = 0;
}

Mesh& Mesh::operator=(Mesh&& other) noexcept {
    if (this != &other) {
        destroy();
        vao_ = other.vao_; vbo_ = other.vbo_; ebo_ = other.ebo_;
        indexCount_ = other.indexCount_;
        other.vao_ = other.vbo_ = other.ebo_ = 0;
        other.indexCount_ = 0;
    }
    return *this;
}

void Mesh::destroy() {
    if (ebo_) glDeleteBuffers(1, &ebo_);
    if (vbo_) glDeleteBuffers(1, &vbo_);
    if (vao_) glDeleteVertexArrays(1, &vao_);
    vao_ = vbo_ = ebo_ = 0;
    indexCount_ = 0;
}

void Mesh::upload(const std::vector<Vertex>& vertices, const std::vector<GLuint>& indices) {
    if (!vao_) glGenVertexArrays(1, &vao_);
    if (!vbo_) glGenBuffers(1, &vbo_);
    if (!ebo_) glGenBuffers(1, &ebo_);

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(),
                 GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));

    glBindVertexArray(0);
    indexCount_ = static_cast<GLsizei>(indices.size());
}

void Mesh::draw() const {
    if (!indexCount_) return;
    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void MeshBuilder::clear() {
    vertices_.clear();
    indices_.clear();
    uvPerMeter_ = 1.0f;
    facadeLayout_ = false;
}

MeshBuilder& MeshBuilder::uvPerMeter(float repeatsPerMetre) {
    uvPerMeter_ = repeatsPerMetre;
    return *this;
}

MeshBuilder& MeshBuilder::facadeLayout(bool enabled) {
    facadeLayout_ = enabled;
    return *this;
}

// The face is projected along its dominant normal axis. The signs are chosen so
// that v grows towards -Z on every face of the road layout; the road, curbs and
// grass then scroll in one direction when the texture offset is increased.
glm::vec2 MeshBuilder::projectUv(const glm::vec3& p, const glm::vec3& n) const {
    glm::vec3 a = glm::abs(n);
    glm::vec2 uv;
    if (a.y >= a.x && a.y >= a.z) {
        uv = glm::vec2(p.x, -p.z);                                   // floor / roof
    } else if (a.x >= a.z) {
        uv = facadeLayout_ ? glm::vec2(p.z, p.y) : glm::vec2(p.y, -p.z);  // side walls
    } else {
        uv = glm::vec2(p.x, p.y);                                    // front / back walls
    }
    return uv * uvPerMeter_;
}

void MeshBuilder::addTriangle(const glm::mat4& xform, const glm::mat3& normalMat,
                              const glm::vec3& a, const glm::vec3& b, const glm::vec3& c,
                              const glm::vec3& color) {
    glm::vec3 pa = glm::vec3(xform * glm::vec4(a, 1.0f));
    glm::vec3 pb = glm::vec3(xform * glm::vec4(b, 1.0f));
    glm::vec3 pc = glm::vec3(xform * glm::vec4(c, 1.0f));
    glm::vec3 localNormal = glm::normalize(glm::cross(b - a, c - a));
    glm::vec3 n = glm::normalize(normalMat * localNormal);

    GLuint base = static_cast<GLuint>(vertices_.size());
    vertices_.push_back({pa, n, color, projectUv(a, localNormal)});
    vertices_.push_back({pb, n, color, projectUv(b, localNormal)});
    vertices_.push_back({pc, n, color, projectUv(c, localNormal)});
    indices_.push_back(base);
    indices_.push_back(base + 1);
    indices_.push_back(base + 2);
}

void MeshBuilder::addQuad(const glm::mat4& xform, const glm::mat3& normalMat, const glm::vec3& a,
                          const glm::vec3& b, const glm::vec3& c, const glm::vec3& d,
                          const glm::vec3& color) {
    addTriangle(xform, normalMat, a, b, c, color);
    addTriangle(xform, normalMat, a, c, d, color);
}

MeshBuilder& MeshBuilder::box(const glm::mat4& xform, const glm::vec3& size,
                              const glm::vec3& color) {
    glm::mat3 nm = glm::inverseTranspose(glm::mat3(xform));
    glm::vec3 h = size * 0.5f;
    glm::vec3 p000(-h.x, -h.y, -h.z), p100(h.x, -h.y, -h.z), p110(h.x, h.y, -h.z),
        p010(-h.x, h.y, -h.z), p001(-h.x, -h.y, h.z), p101(h.x, -h.y, h.z), p111(h.x, h.y, h.z),
        p011(-h.x, h.y, h.z);

    addQuad(xform, nm, p001, p101, p111, p011, color);  // +Z
    addQuad(xform, nm, p100, p000, p010, p110, color);  // -Z
    addQuad(xform, nm, p101, p100, p110, p111, color);  // +X
    addQuad(xform, nm, p000, p001, p011, p010, color);  // -X
    addQuad(xform, nm, p011, p111, p110, p010, color);  // +Y
    addQuad(xform, nm, p000, p100, p101, p001, color);  // -Y
    return *this;
}

MeshBuilder& MeshBuilder::plane(const glm::mat4& xform, float width, float depth,
                                const glm::vec3& color) {
    glm::mat3 nm = glm::inverseTranspose(glm::mat3(xform));
    float w = width * 0.5f, d = depth * 0.5f;
    addQuad(xform, nm, glm::vec3(-w, 0, d), glm::vec3(w, 0, d), glm::vec3(w, 0, -d),
            glm::vec3(-w, 0, -d), color);
    return *this;
}

// Cylinder around the local X axis: the orientation wheels need.
MeshBuilder& MeshBuilder::cylinder(const glm::mat4& xform, float radius, float height, int segments,
                                   const glm::vec3& color) {
    glm::mat3 nm = glm::inverseTranspose(glm::mat3(xform));
    float h = height * 0.5f;
    for (int i = 0; i < segments; ++i) {
        float a0 = 2.0f * 3.14159265f * i / segments;
        float a1 = 2.0f * 3.14159265f * (i + 1) / segments;
        glm::vec3 r0(0.0f, std::sin(a0) * radius, std::cos(a0) * radius);
        glm::vec3 r1(0.0f, std::sin(a1) * radius, std::cos(a1) * radius);
        glm::vec3 xl(-h, 0, 0), xr(h, 0, 0);
        addQuad(xform, nm, xl + r0, xr + r0, xr + r1, xl + r1, color);
        addTriangle(xform, nm, xr, xr + r1, xr + r0, color);
        addTriangle(xform, nm, xl, xl + r0, xl + r1, color);
    }
    return *this;
}

// Box whose +Z face is scaled down: a windscreen-like cabin slope.
MeshBuilder& MeshBuilder::wedge(const glm::mat4& xform, const glm::vec3& size, float topScale,
                                const glm::vec3& color) {
    glm::mat3 nm = glm::inverseTranspose(glm::mat3(xform));
    glm::vec3 h = size * 0.5f;
    float tx = h.x * topScale, tz = h.z * topScale;
    glm::vec3 b0(-h.x, -h.y, -h.z), b1(h.x, -h.y, -h.z), b2(h.x, -h.y, h.z), b3(-h.x, -h.y, h.z);
    glm::vec3 t0(-tx, h.y, -tz), t1(tx, h.y, -tz), t2(tx, h.y, tz), t3(-tx, h.y, tz);

    addQuad(xform, nm, b3, b2, t2, t3, color);
    addQuad(xform, nm, b1, b0, t0, t1, color);
    addQuad(xform, nm, b2, b1, t1, t2, color);
    addQuad(xform, nm, b0, b3, t3, t0, color);
    addQuad(xform, nm, t0, t3, t2, t1, color);
    addQuad(xform, nm, b0, b1, b2, b3, color);
    return *this;
}

void MeshBuilder::build(Mesh& out) const { out.upload(vertices_, indices_); }

}  // namespace engine
