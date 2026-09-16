#include "hud.h"

namespace engine {

// Seven-segment masks for 0..9, bit order a,b,c,d,e,f,g.
static const unsigned char SEGMENTS[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66,
                                           0x6D, 0x7D, 0x07, 0x7F, 0x6F};

bool Hud::init() {
    if (!shader_.loadFromFiles("shaders/hud.vert", "shaders/hud.frag")) return false;

    const float quad[] = {0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1};
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    glBindVertexArray(0);
    return true;
}

Hud::~Hud() {
    if (vbo_) glDeleteBuffers(1, &vbo_);
    if (vao_) glDeleteVertexArrays(1, &vao_);
}

void Hud::begin(int screenWidth, int screenHeight) {
    screen_ = glm::vec2(static_cast<float>(screenWidth), static_cast<float>(screenHeight));
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);  // the y flip reverses the quad winding
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    shader_.use();
    shader_.setVec2("uScreen", screen_);
    glBindVertexArray(vao_);
}

void Hud::end() {
    glBindVertexArray(0);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
}

void Hud::rect(float x, float y, float w, float h, const glm::vec4& color) {
    shader_.setVec4("uRect", glm::vec4(x, y, w, h));
    shader_.setVec4("uColor", color);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void Hud::bar(float x, float y, float w, float h, float fill, const glm::vec4& color) {
    rect(x, y, w, h, glm::vec4(0.0f, 0.0f, 0.0f, 0.35f));
    float clamped = fill < 0.0f ? 0.0f : (fill > 1.0f ? 1.0f : fill);
    rect(x + 2.0f, y + 2.0f, (w - 4.0f) * clamped, h - 4.0f, color);
}

void Hud::digitGlyph(float x, float y, float size, int digit, const glm::vec4& color) {
    if (digit < 0 || digit > 9) return;
    unsigned char mask = SEGMENTS[digit];
    float t = size * 0.14f;         // stroke thickness
    float w = size * 0.6f;          // glyph width
    float mid = y + size * 0.5f;

    if (mask & 0x01) rect(x + t, y, w - 2 * t, t, color);                       // a
    if (mask & 0x02) rect(x + w - t, y + t, t, size * 0.5f - 1.5f * t, color);  // b
    if (mask & 0x04) rect(x + w - t, mid + t * 0.5f, t, size * 0.5f - 1.5f * t, color);  // c
    if (mask & 0x08) rect(x + t, y + size - t, w - 2 * t, t, color);            // d
    if (mask & 0x10) rect(x, mid + t * 0.5f, t, size * 0.5f - 1.5f * t, color); // e
    if (mask & 0x20) rect(x, y + t, t, size * 0.5f - 1.5f * t, color);          // f
    if (mask & 0x40) rect(x + t, mid - t * 0.5f, w - 2 * t, t, color);          // g
}

void Hud::number(float x, float y, float size, int value, int digits, const glm::vec4& color) {
    if (value < 0) value = 0;
    float advance = size * 0.75f;
    for (int i = digits - 1; i >= 0; --i) {
        digitGlyph(x + advance * i, y, size, value % 10, color);
        value /= 10;
        if (value == 0) break;  // no leading zeros
    }
}

}  // namespace engine
