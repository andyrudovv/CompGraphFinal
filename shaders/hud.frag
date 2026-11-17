#version 330 core

// HUD fragment shader. Outputs a flat colour.

uniform vec4 uColor;
out vec4 fColor;

void main() { fColor = uColor; }
