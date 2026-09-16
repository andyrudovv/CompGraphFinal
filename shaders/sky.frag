#version 330 core

in vec2 vNdc;
out vec4 fColor;

uniform mat4 uInvViewProj;
uniform vec3 uCamPos;
uniform vec3 uZenith;
uniform vec3 uHorizon;
uniform vec3 uGround;
uniform vec3 uSunDir;
uniform vec3 uSunColor;
uniform float uNight;

vec3 hash33(vec3 p) {
    p = vec3(dot(p, vec3(127.1, 311.7, 74.7)),
             dot(p, vec3(269.5, 183.3, 246.1)),
             dot(p, vec3(113.5, 271.9, 124.6)));
    return fract(sin(p) * 43758.5453);
}

// Cell-based star field: one candidate star per cell, most of them rejected.
float stars(vec3 dir) {
    vec3 p = dir * 110.0;
    vec3 cell = floor(p);
    vec3 f = fract(p);
    vec3 r = hash33(cell);
    float present = step(0.93, r.z);
    float d = length(f - r);
    float twinkle = 0.6 + 0.4 * r.x;
    return present * smoothstep(0.22, 0.0, d) * twinkle;
}

void main() {
    vec4 far = uInvViewProj * vec4(vNdc, 1.0, 1.0);
    vec3 dir = normalize(far.xyz / far.w - uCamPos);

    float h = dir.y;
    vec3 sky = mix(uHorizon, uZenith, pow(clamp(h, 0.0, 1.0), 0.5));
    sky = mix(sky, uGround, smoothstep(0.0, -0.08, h));

    sky += vec3(stars(dir)) * uNight * smoothstep(-0.02, 0.15, h);

    float sunUp = smoothstep(-0.06, 0.04, uSunDir.y);
    float sunDot = dot(dir, uSunDir);
    sky += uSunColor * pow(max(sunDot, 0.0), 220.0) * 0.6 * sunUp;        // glow
    sky += uSunColor * smoothstep(0.9975, 0.9990, sunDot) * 1.5 * sunUp;  // disc

    float moonUp = smoothstep(-0.06, 0.04, -uSunDir.y);
    float moonDot = dot(dir, -uSunDir);
    sky += vec3(0.85, 0.87, 0.95) * smoothstep(0.9982, 0.9992, moonDot) * moonUp;

    fColor = vec4(sky, 1.0);
}
