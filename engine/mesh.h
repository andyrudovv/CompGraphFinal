// Indexed triangle meshes and MeshBuilder. MeshBuilder adds boxes, cylinders, wedges and
// bicubic Bezier patches with normals, colours and texture coordinates, then uploads them as
// one mesh.

#ifndef ENGINE_MESH_H
#define ENGINE_MESH_H

#include <GL/glew.h>
#include <glm/glm.hpp>

#include <vector>

namespace engine {

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 color;
    glm::vec2 uv;
};

class Mesh {
public:
    Mesh() = default;
    ~Mesh();
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;

    void upload(const std::vector<Vertex>& vertices, const std::vector<GLuint>& indices);
    void draw() const;
    void destroy();
    bool empty() const { return indexCount_ == 0; }

private:
    GLuint vao_ = 0, vbo_ = 0, ebo_ = 0;
    GLsizei indexCount_ = 0;
};

class MeshBuilder {
public:
    MeshBuilder& box(const glm::mat4& xform, const glm::vec3& size, const glm::vec3& color);
    MeshBuilder& plane(const glm::mat4& xform, float width, float depth, const glm::vec3& color);
    MeshBuilder& cylinder(const glm::mat4& xform, float radius, float height, int segments,
                          const glm::vec3& color);
    MeshBuilder& wedge(const glm::mat4& xform, const glm::vec3& size, float topScale,
                       const glm::vec3& color);

    MeshBuilder& bezierPatch(const glm::mat4& xform, const glm::vec3* ctrl, int segments,
                             const glm::vec3& color);

    MeshBuilder& uvPerMeter(float repeatsPerMetre);
    MeshBuilder& facadeLayout(bool enabled);

    void build(Mesh& out) const;
    void clear();

private:
    glm::vec2 projectUv(const glm::vec3& localPos, const glm::vec3& localNormal) const;

    void addTriangle(const glm::mat4& xform, const glm::mat3& normalMat, const glm::vec3& a,
                     const glm::vec3& b, const glm::vec3& c, const glm::vec3& color);
    void addQuad(const glm::mat4& xform, const glm::mat3& normalMat, const glm::vec3& a,
                 const glm::vec3& b, const glm::vec3& c, const glm::vec3& d, const glm::vec3& color);

    std::vector<Vertex> vertices_;
    std::vector<GLuint> indices_;
    float uvPerMeter_ = 1.0f;
    bool facadeLayout_ = false;
};

}

#endif
