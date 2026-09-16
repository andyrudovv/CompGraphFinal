#ifndef ENGINE_SHADER_H
#define ENGINE_SHADER_H

#include <GL/glew.h>
#include <glm/glm.hpp>

#include <string>
#include <unordered_map>

namespace engine {

class Shader {
public:
    ~Shader();

    bool loadFromFiles(const std::string& vertexPath, const std::string& fragmentPath);
    void use() const;
    GLuint id() const { return program_; }

    void setInt(const char* name, int v);
    void setFloat(const char* name, float v);
    void setVec2(const char* name, const glm::vec2& v);
    void setVec3(const char* name, const glm::vec3& v);
    void setVec4(const char* name, const glm::vec4& v);
    void setMat3(const char* name, const glm::mat3& m);
    void setMat4(const char* name, const glm::mat4& m);
    void setVec3Array(const char* name, const glm::vec3* v, int count);
    void setFloatArray(const char* name, const float* v, int count);

private:
    GLint location(const char* name);

    GLuint program_ = 0;
    std::unordered_map<std::string, GLint> locations_;
};

// Resolves an asset next to the working directory or one level above it, so the
// binary runs both from the project root and from build/.
std::string assetPath(const std::string& relative);

}  // namespace engine

#endif
