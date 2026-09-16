#include "shader.h"

#include <glm/gtc/type_ptr.hpp>

#include <cstdio>
#include <fstream>
#include <sstream>
#include <vector>

namespace engine {

std::string assetPath(const std::string& relative) {
    const char* prefixes[] = {"", "../", "../../"};
    for (const char* p : prefixes) {
        std::string candidate = std::string(p) + relative;
        std::ifstream f(candidate);
        if (f.good()) return candidate;
    }
    return relative;
}

static bool readFile(const std::string& path, std::string& out) {
    std::ifstream file(assetPath(path));
    if (!file) {
        std::fprintf(stderr, "cannot open shader file: %s\n", path.c_str());
        return false;
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    out = ss.str();
    return true;
}

static GLuint compile(GLenum type, const std::string& src, const std::string& name) {
    GLuint shader = glCreateShader(type);
    const char* data = src.c_str();
    glShaderSource(shader, 1, &data, nullptr);
    glCompileShader(shader);

    GLint ok = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
        std::vector<char> log(len > 1 ? len : 1);
        glGetShaderInfoLog(shader, len, nullptr, log.data());
        std::fprintf(stderr, "%s: %s\n", name.c_str(), log.data());
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

Shader::~Shader() {
    if (program_) glDeleteProgram(program_);
}

bool Shader::loadFromFiles(const std::string& vertexPath, const std::string& fragmentPath) {
    std::string vsrc, fsrc;
    if (!readFile(vertexPath, vsrc) || !readFile(fragmentPath, fsrc)) return false;

    GLuint vs = compile(GL_VERTEX_SHADER, vsrc, vertexPath);
    GLuint fs = compile(GL_FRAGMENT_SHADER, fsrc, fragmentPath);
    if (!vs || !fs) return false;

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint ok = GL_FALSE;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
        std::vector<char> log(len > 1 ? len : 1);
        glGetProgramInfoLog(prog, len, nullptr, log.data());
        std::fprintf(stderr, "link %s: %s\n", vertexPath.c_str(), log.data());
        glDeleteProgram(prog);
        return false;
    }

    if (program_) glDeleteProgram(program_);
    program_ = prog;
    locations_.clear();
    return true;
}

void Shader::use() const { glUseProgram(program_); }

GLint Shader::location(const char* name) {
    auto it = locations_.find(name);
    if (it != locations_.end()) return it->second;
    GLint loc = glGetUniformLocation(program_, name);
    locations_.emplace(name, loc);
    return loc;
}

void Shader::setInt(const char* n, int v) { glUniform1i(location(n), v); }
void Shader::setFloat(const char* n, float v) { glUniform1f(location(n), v); }
void Shader::setVec2(const char* n, const glm::vec2& v) { glUniform2fv(location(n), 1, glm::value_ptr(v)); }
void Shader::setVec3(const char* n, const glm::vec3& v) { glUniform3fv(location(n), 1, glm::value_ptr(v)); }
void Shader::setVec4(const char* n, const glm::vec4& v) { glUniform4fv(location(n), 1, glm::value_ptr(v)); }
void Shader::setMat3(const char* n, const glm::mat3& m) { glUniformMatrix3fv(location(n), 1, GL_FALSE, glm::value_ptr(m)); }
void Shader::setMat4(const char* n, const glm::mat4& m) { glUniformMatrix4fv(location(n), 1, GL_FALSE, glm::value_ptr(m)); }

void Shader::setVec3Array(const char* n, const glm::vec3* v, int count) {
    if (count > 0) glUniform3fv(location(n), count, glm::value_ptr(v[0]));
}

void Shader::setFloatArray(const char* n, const float* v, int count) {
    if (count > 0) glUniform1fv(location(n), count, v);
}

}  // namespace engine
