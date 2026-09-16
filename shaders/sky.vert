#version 330 core

// Attribute-less full-screen triangle; the fragment shader rebuilds view rays.
out vec2 vNdc;

void main() {
    vec2 corner = vec2(float((gl_VertexID << 1) & 2), float(gl_VertexID & 2));
    vNdc = corner * 2.0 - 1.0;
    gl_Position = vec4(vNdc, 1.0, 1.0);
}
