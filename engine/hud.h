#ifndef ENGINE_HUD_H
#define ENGINE_HUD_H

#include <GL/glew.h>
#include <glm/glm.hpp>

#include "shader.h"

namespace engine {

// Minimal screen-space overlay: coloured rectangles plus seven-segment numbers.
// Coordinates are pixels with the origin in the top-left corner.
class Hud {
public:
    bool init();
    ~Hud();

    void begin(int screenWidth, int screenHeight);
    void end();

    void rect(float x, float y, float w, float h, const glm::vec4& color);
    void bar(float x, float y, float w, float h, float fill, const glm::vec4& color);
    // Draws value right-aligned in `digits` cells; each cell is about size*0.7 wide.
    void number(float x, float y, float size, int value, int digits, const glm::vec4& color);
    void digitGlyph(float x, float y, float size, int digit, const glm::vec4& color);

private:
    Shader shader_;
    GLuint vao_ = 0, vbo_ = 0;
    glm::vec2 screen_{1.0f};
};

}  // namespace engine

#endif
