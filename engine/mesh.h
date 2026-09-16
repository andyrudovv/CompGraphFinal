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
};

// Indexed triangle mesh living in one VAO/VBO/EBO pair.
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

// Accumulates transformed primitives into a single mesh, so a whole car or the
// whole road is one draw call.
class MeshBuilder {
public:
    MeshBuilder& box(const glm::mat4& xform, const glm::vec3& size, const glm::vec3& color);
    MeshBuilder& plane(const glm::mat4& xform, float width, float depth, const glm::vec3& color);
    MeshBuilder& cylinder(const glm::mat4& xform, float radius, float height, int segments,
                          const glm::vec3& color);
    MeshBuilder& wedge(const glm::mat4& xform, const glm::vec3& size, float topScale,
                       const glm::vec3& color);

    void build(Mesh& out) const;
    void clear();

private:
    void addTriangle(const glm::mat4& xform, const glm::mat3& normalMat, const glm::vec3& a,
                     const glm::vec3& b, const glm::vec3& c, const glm::vec3& color);
    void addQuad(const glm::mat4& xform, const glm::mat3& normalMat, const glm::vec3& a,
                 const glm::vec3& b, const glm::vec3& c, const glm::vec3& d, const glm::vec3& color);

    std::vector<Vertex> vertices_;
    std::vector<GLuint> indices_;
};

}  // namespace engine

#endif
