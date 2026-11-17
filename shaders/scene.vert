#version 330 core

// Scene vertex shader. Moves vertices to world and clip space, passes normal, colour and UV
// on, and bends trees in the wind.

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aColor;
layout(location = 3) in vec2 aUV;

uniform mat4 uModel;
uniform mat4 uViewProj;
uniform mat3 uNormalMat;
uniform float uTime;
uniform float uSway;

out vec3 vWorldPos;
out vec3 vNormal;
out vec3 vColor;
out vec2 vUV;

void main() {
    vec4 world = uModel * vec4(aPosition, 1.0);

    if (uSway > 0.0) {
        float h = clamp(aPosition.y / 4.2, 0.0, 1.0);
        h *= h;
        float phase = uModel[3].x * 1.7;
        float wave = sin(uTime * 1.1 + phase) + 0.25 * sin(uTime * 2.3 + phase * 1.7 + aPosition.x);
        world.x += uSway * h * wave;
        world.z += uSway * h * 0.4 * sin(uTime * 0.8 + phase * 0.6);
    }
    vWorldPos = world.xyz;
    vNormal = uNormalMat * aNormal;
    vColor = aColor;
    vUV = aUV;
    gl_Position = uViewProj * world;
}
