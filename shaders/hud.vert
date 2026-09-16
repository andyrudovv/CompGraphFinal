#version 330 core

layout(location = 0) in vec2 aCorner;  // unit quad

uniform vec4 uRect;    // x, y, width, height in pixels, origin top-left
uniform vec2 uScreen;

void main() {
    vec2 pixel = uRect.xy + aCorner * uRect.zw;
    vec2 ndc = vec2(pixel.x / uScreen.x * 2.0 - 1.0, 1.0 - pixel.y / uScreen.y * 2.0);
    gl_Position = vec4(ndc, 0.0, 1.0);
}
