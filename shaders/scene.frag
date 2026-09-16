#version 330 core

#define MAX_POINT_LIGHTS 8

in vec3 vWorldPos;
in vec3 vNormal;
in vec3 vColor;

out vec4 fColor;

uniform vec3 uCamPos;
uniform vec3 uTint;
uniform float uEmissive;
uniform float uSpecular;
uniform float uShininess;

uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform vec3 uAmbient;

uniform int uPointCount;
uniform vec3 uPointPos[MAX_POINT_LIGHTS];
uniform vec3 uPointColor[MAX_POINT_LIGHTS];
uniform float uPointRange[MAX_POINT_LIGHTS];

uniform vec3 uFogColor;
uniform float uFogDensity;

void main() {
    vec3 albedo = vColor * uTint;
    vec3 N = normalize(vNormal);
    vec3 V = normalize(uCamPos - vWorldPos);

    vec3 lit = uAmbient * albedo;

    float ndl = max(dot(N, uLightDir), 0.0);
    lit += uLightColor * albedo * ndl;
    if (ndl > 0.0) {
        vec3 H = normalize(uLightDir + V);
        lit += uLightColor * uSpecular * pow(max(dot(N, H), 0.0), uShininess);
    }

    for (int i = 0; i < uPointCount; ++i) {
        vec3 delta = uPointPos[i] - vWorldPos;
        float dist = length(delta);
        vec3 L = delta / max(dist, 0.0001);
        float att = clamp(1.0 - dist / uPointRange[i], 0.0, 1.0);
        att *= att;
        float nl = max(dot(N, L), 0.0);
        lit += uPointColor[i] * albedo * nl * att;
        if (nl > 0.0) {
            vec3 H = normalize(L + V);
            lit += uPointColor[i] * uSpecular * pow(max(dot(N, H), 0.0), uShininess) * att;
        }
    }

    vec3 color = mix(lit, albedo, uEmissive);

    float dist = length(uCamPos - vWorldPos);
    float fog = 1.0 - exp(-pow(dist * uFogDensity, 2.0));
    color = mix(color, uFogColor, clamp(fog, 0.0, 1.0));

    fColor = vec4(color, 1.0);
}
