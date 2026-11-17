# Shaders and the math that is actually coded

All GLSL is in `shaders/`, version 330 core, loaded from files at start-up (`Shader::loadFromFiles`). Compile and link errors are printed to stderr.

## scene.vert / scene.frag

Vertex stage (`scene.vert`): `world = uModel * pos`, `vNormal = uNormalMat * normal`, `gl_Position = uViewProj * world`. The CPU passes `uNormalMat = inverseTranspose(mat3(model))` (`renderer.cpp`, `Renderer::draw`), so non-uniform scale (towers) keeps correct normals.

Fragment stage inputs: world position, normal, vertex colour, UV.

### Albedo and texture (`scene.frag:49-57`)

```
albedo = vertexColour * tint * texel.rgb * 1.25      (TEX_GAIN, textures are authored ~0.8 grey)
glow   = texel.rgb * texel.a * uTexGlow              (alpha = lit-window mask)
uv     = vUV * uUvScale + uUvOffset
```

### Blinn–Phong (`scene.frag:62-67`)

With `N` normal, `L` direction to the light, `V` direction to the camera:

```
H        = normalize(L + V)
diffuse  = lightColor * albedo * max(N·L, 0)
specular = lightColor * uSpecular * max(N·H, 0)^uShininess      (only if N·L > 0)
```

Ambient is `uAmbient * albedo`. The directional light is the sun by day and a dim blue "moon" light by night (see `Game::updateDayNight`).

### Point lights (`scene.frag:68-83`), up to 8

```
att = clamp(1 - dist / range, 0, 1)^2
lit += color * albedo * max(N·L,0) * att  +  specular term * att
```

Used for the nearest street lamps at night (`Game::collectLights`).

### Spot lights (`scene.frag:85-97`), up to 4

Same as a point light multiplied by a cone mask:

```
cone = smoothstep(cosOuter, cosInner, dot(-L, spotDir))
att  = clamp(1 - dist / range, 0, 1)^2 * cone
```

`cosInner/cosOuter` are computed on the CPU from `innerAngle = 0.26 rad`, `outerAngle = 0.55 rad` (`renderer.cpp`). `spotDir` is the beam axis `(0, -0.16, -1)` transformed by the headlamp node's world matrix, so the beam follows the car (`game.cpp:972`).

### Emission and fog (`scene.frag:99-103`)

```
color = mix(lit, albedo, uEmissive) + glow
fog   = 1 - exp(-(dist * uFogDensity)^2)
color = mix(color, uFogColor, clamp(fog, 0, 1))
```

`uEmissive = 1` ignores lighting entirely (lamps, tail lights, sparks).

## sky.vert / sky.frag

One triangle without vertex attributes. The fragment shader reconstructs the view ray:

```
far = uInvViewProj * (ndc, 1, 1);   dir = normalize(far.xyz / far.w - uCamPos)
sky = mix(horizon, zenith, sqrt(clamp(dir.y, 0, 1)))        (sky.frag:39)
sky = mix(sky, ground, 1 - smoothstep(-0.08, 0, dir.y))
```

- Sun: glow `pow(max(dot(dir, sunDir), 0), 220) * 0.6` plus a disc `smoothstep(0.9975, 0.9990, dot)`; both faded by `sunUp`.
- Moon: a disc opposite the sun, `smoothstep(0.9982, 0.9992, dot(dir, -sunDir))`.
- Stars (`sky.frag:23-33`): `p = dir * 110`; one hashed candidate per integer cell, kept when `hash.z > 0.93`; brightness falls off with the distance to the hashed point inside the cell, scaled by `uNight` and a horizon fade.

## hud.vert / hud.frag

Pixel coordinates to NDC, flat colour with alpha. Digits are seven-segment rectangles built in `engine/hud.cpp`; there is no font.

## Day/night model (CPU, `Game::updateDayNight`, `game.cpp:927`)

```
angle     = dayTime * 2π                       dayTime += dt / 90
sunDir    = normalize(cos(angle) * 0.9, sin(angle), -0.35)
day       = smoothstep(-0.12, 0.18, sunDir.y)  night = 1 - day
sunset    = exp(-(3 * sunDir.y)^2)             (orange band near the horizon)
```

Sun power, moon power, ambient, fog colour/density and sky colours are mixes driven by `night` and `sunset`. Headlights and lamps fade in with `smoothstep(0.25, 0.65, night)`.

## Not implemented

Shadow maps, bloom, SSAO, environment/reflection mapping, normal mapping, instancing and PBR are not part of the `main` build.
