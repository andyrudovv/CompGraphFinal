// Screen-space HUD: rectangles, bars, seven-segment numbers and a small built-in bitmap font.

#ifndef ENGINE_HUD_H
#define ENGINE_HUD_H

#include <string>

#include <GL/glew.h>
#include <glm/glm.hpp>

#include "shader.h"

namespace engine {

class Hud {
public:
    bool init();
    ~Hud();

    void begin(int screenWidth, int screenHeight);
    void end();

    void rect(float x, float y, float w, float h, const glm::vec4& color);
    void bar(float x, float y, float w, float h, float fill, const glm::vec4& color);
    void number(float x, float y, float size, int value, int digits, const glm::vec4& color);
    void text(float x, float y, float scale, const std::string& value, const glm::vec4& color);
    void digitGlyph(float x, float y, float size, int digit, const glm::vec4& color);

private:
    Shader shader_;
    GLuint vao_ = 0, vbo_ = 0;
    GLuint textVao_ = 0, textVbo_ = 0;
    glm::vec2 screen_{1.0f};
};

}

#endif
