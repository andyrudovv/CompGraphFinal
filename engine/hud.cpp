#include "hud.h"

#include <cctype>
#include <vector>

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
    glGenVertexArrays(1, &textVao_);
    glGenBuffers(1, &textVbo_);
    glBindVertexArray(textVao_);
    glBindBuffer(GL_ARRAY_BUFFER, textVbo_);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    glBindVertexArray(0);
    return true;
}

Hud::~Hud() {
    if (textVbo_) glDeleteBuffers(1, &textVbo_);
    if (textVao_) glDeleteVertexArrays(1, &textVao_);
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

// Five by seven uppercase glyphs, drawn as a single batch per text line.
void Hud::text(float x, float y, float scale, const std::string& value, const glm::vec4& color) {
    struct Glyph { char letter; unsigned char rows[7]; };
    static const Glyph font[] = {
        {'A',{14,17,17,31,17,17,17}}, {'B',{30,17,17,30,17,17,30}},
        {'C',{14,17,16,16,16,17,14}}, {'D',{30,17,17,17,17,17,30}},
        {'E',{31,16,16,30,16,16,31}}, {'F',{31,16,16,30,16,16,16}},
        {'G',{14,17,16,23,17,17,15}}, {'H',{17,17,17,31,17,17,17}},
        {'I',{14,4,4,4,4,4,14}}, {'J',{7,2,2,2,18,18,12}},
        {'K',{17,18,20,24,20,18,17}}, {'L',{16,16,16,16,16,16,31}},
        {'M',{17,27,21,21,17,17,17}}, {'N',{17,25,21,19,17,17,17}},
        {'O',{14,17,17,17,17,17,14}}, {'P',{30,17,17,30,16,16,16}},
        {'Q',{14,17,17,17,21,18,13}}, {'R',{30,17,17,30,20,18,17}},
        {'S',{15,16,16,14,1,1,30}}, {'T',{31,4,4,4,4,4,4}},
        {'U',{17,17,17,17,17,17,14}}, {'V',{17,17,17,17,17,10,4}},
        {'W',{17,17,17,21,21,21,10}}, {'X',{17,17,10,4,10,17,17}},
        {'Y',{17,17,10,4,4,4,4}}, {'Z',{31,1,2,4,8,16,31}},
        {'0',{14,17,19,21,25,17,14}}, {'1',{4,12,4,4,4,4,14}},
        {'2',{14,17,1,2,4,8,31}}, {'3',{30,1,1,14,1,1,30}},
        {'4',{2,6,10,18,31,2,2}}, {'5',{31,16,16,30,1,1,30}},
        {'6',{14,16,16,30,17,17,14}}, {'7',{31,1,2,4,8,8,8}},
        {'8',{14,17,17,14,17,17,14}}, {'9',{14,17,17,15,1,1,14}},
        {'/',{1,2,2,4,8,8,16}}, {'-',{0,0,0,31,0,0,0}},
        {':',{0,4,4,0,4,4,0}}, {'.',{0,0,0,0,0,4,4}},
        {'>',{16,8,4,2,4,8,16}}, {'+',{0,4,4,31,4,4,0}},
        {'(',{2,4,8,8,8,4,2}}, {')',{8,4,2,2,2,4,8}},
        {'?',{14,17,1,2,4,0,4}}
    };
    std::vector<float> vertices;
    for (unsigned char raw : value) {
        char c = static_cast<char>(std::toupper(raw));
        if (c != ' ') {
            const Glyph* glyph = &font[sizeof(font) / sizeof(font[0]) - 1];
            for (const Glyph& candidate : font) {
                if (candidate.letter == c) { glyph = &candidate; break; }
            }
            for (int row = 0; row < 7; ++row) {
                for (int col = 0; col < 5; ++col) {
                    if (!(glyph->rows[row] & (1 << (4 - col)))) continue;
                    float left = x + col * scale, top = y + row * scale;
                    float right = left + scale, bottom = top + scale;
                    vertices.insert(vertices.end(), {left,top, right,top, right,bottom,
                        left,top, right,bottom, left,bottom});
                }
            }
        }
        x += 6.0f * scale;
    }
    if (vertices.empty()) return;
    shader_.setVec4("uRect", glm::vec4(0, 0, 1, 1));
    shader_.setVec4("uColor", color);
    glBindVertexArray(textVao_);
    glBindBuffer(GL_ARRAY_BUFFER, textVbo_);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size() / 2));
    glBindVertexArray(vao_);
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
