# Architecture

Describes the state of `main` (GLFW build). Work that lives only in other branches is listed in [Branch status](#branch-status).

## Layers

```
game/                       engine/
  main.cpp  ── loop ──────►   Window   (GLFW context, input edges, frame clock, PPM)
  game.cpp  ── scene ─────►   Renderer (per-frame uniforms, sky pass, draw)
                              Mesh / MeshBuilder, Node, Texture, texgen, Hud, ObjLoader
                              Shader   (file lookup, compile/link, uniform cache)
                                    │
                              shaders/  scene.*  sky.*  hud.*   (GLSL 330 core)
```

`engine/` knows nothing about traffic or scoring; `game/` owns all gameplay state and only talks to the engine through `Renderer::beginFrame`, `Renderer::draw`, `Node::draw` and `Hud`.

## Main loop (`game/main.cpp`)

```
window.beginFrame()   clock, key edges, mouse
game.update(window)   fixed 120 Hz simulation + camera + lights
game.render(window)   sky -> world -> HUD
window.endFrame()     swap buffers
```

## One frame (`Game::render`, `game/game.cpp:1115`)

```mermaid
flowchart LR
    U[Game::update] --> E[Environment: sun, sky, fog, lights]
    E --> B[Renderer::beginFrame\nclear depth, upload uniforms]
    B --> S[drawSky\nfull-screen triangle, no depth]
    S --> W[ground, road, curbs, props]
    W --> T[traffic + player via Node tree]
    T --> P[debris + sparks]
    P --> H[Hud\nblended, no depth]
```

1. `Game::update` advances the simulation in fixed 1/120 s steps, then calls `updateDayNight` and `collectLights` (`game.cpp:927`, `:972`), which fill an `engine::Environment`.
2. `Renderer::beginFrame` uploads camera, sun/moon light, ambient, fog, up to 8 point lights and 4 spot lights.
3. `drawSky` draws one attribute-less triangle with depth test/write off (`shaders/sky.frag` rebuilds the view ray from `uInvViewProj`).
4. Scene meshes are drawn with `Renderer::draw(mesh, model, Material)`. Cars are `Node` trees.
5. `Hud` draws rectangles and seven-segment digits with alpha blending.

No FBOs, shadow maps, deferred shading or post-processing are used.

## Data model

| Type | File | Role |
| --- | --- | --- |
| `Vertex {pos, normal, colour, uv}` | `engine/mesh.h` | One vertex layout for every mesh |
| `Mesh`, `MeshBuilder` | `engine/mesh.*` | VAO/VBO/EBO; builder appends boxes, planes, cylinders, wedges and generates UVs |
| `Material` | `engine/renderer.h` | tint, emissive, specular, shininess, texture, uvScale/uvOffset, textureGlow |
| `Node` | `engine/node.h` | `world = parentWorld * local`; optional mesh and material; `updateWorld`, `draw` |
| `CarRig` | `game/game.h` | Tree: body → trim, 4 wheels, 2 headlamps, tail lights. Pointers into `children`, so never copy after build |
| `Environment` | `engine/renderer.h` | Everything the shader needs besides geometry |
| `TrafficCar`, `Prop`, `Spark`, `Debris` | `game/game.h` | Simulation entities |

## Car hierarchy and headlights

`Game::buildRigs` (`game.cpp:1010`) builds one tree per vehicle type. `poseRig` (`:1076`) sets wheel rotation (`-wheelAngle` around X), lamp/tail emissive and calls `root.updateWorld(model)`. `collectLights` reads `lamp->world()` of the player's sedan to place the two spot lights, so the beams follow the car's yaw and roll.

## Simulation (summary)

- Player stays at `z = 0`; the world scrolls by `speed_ * dt`. Traffic spawns at `z = -185`, recycled at `z = 30`.
- Traffic: adaptive cruise (gap grows with closing speed), lane changes from level 3.
- Collisions: axis-aligned boxes (rotated extents), mass-weighted separation and impulse with restitution 0.3; a hit makes cars wrecks that slide to the shoulder.
- Difficulty: +1 level every 18 s.

## Branch status

| Branch | Content | In `main`? |
| --- | --- | --- |
| `main` | GLFW build, cameras (chase/free/orbit), help panel, spot headlights, `Node` hierarchy | yes |
| `alikhan` | Port of window/input to freeglut, `MeshBuilder::bezierPatch`, roadside hills (key B) | no, conflicts with `main` in `window.*` and `game.cpp` |
| (none found) | OBJ car/truck models (`engine/objloader.*` exists, `models/` does not), wind sway shader, environment mapping | not implemented in any pushed branch at the time of writing |
