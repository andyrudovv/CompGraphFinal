# Lane Runner

**Topic:** Lane Runner: Dynamic Day/Night Lighting
**Course:** Computer Graphics Fundamentals, Astana IT University
**Team Moggers:** Andrey Rudov, Violetta Yun, Alikhan Mukhamed-Rakhym, Dana Tutkyshbaeva

Lane Runner is an endless driving game on a three-lane road. You change lanes, overtake slower traffic, jump over cars and try not to crash. The whole scene runs on our own small engine written in C++17 and OpenGL 3.3 core. No game engine is used.

The main technique is **dynamic lighting**: Blinn-Phong shading with the sun or moon, street lamps and two headlight spot lights, combined with fog and a 90-second day/night cycle. Key **5** turns the dynamic lighting off and on, so the scene can be compared with and without it.

---

## Contents

1. [What is in the scene](#what-is-in-the-scene)
2. [System requirements](#system-requirements)
3. [Build and run](#build-and-run)
4. [Controls](#controls)
5. [Command-line options](#command-line-options)
6. [How it works](#how-it-works)
7. [Performance](#performance)
8. [Project structure](#project-structure)
9. [Libraries and assets](#libraries-and-assets)
10. [Team and module ownership](#team-and-module-ownership)
11. [Requirements checklist](#requirements-checklist)
12. [Known limitations](#known-limitations)

---

## What is in the scene

| Feature | Course week |
| --- | --- |
| OpenGL 3.3 core profile, VAO / VBO / EBO, shaders loaded from files | 1–2 |
| Keyboard and mouse input, double buffering | 3 |
| Model matrices for every object; car roll, yaw and jump; meshes built in code | 4 |
| Perspective camera with three modes: Chase, Free-fly and Orbit | 5 |
| Blinn-Phong lighting: sun or moon, up to 8 street lamps, 2 headlight spot lights, fog | 5 |
| Sun direction controlled from the keyboard | 5 |
| Roadside hills made of bicubic Bezier patches | 6 |
| Depth test, back-face culling, alpha blending for the HUD, MSAA, screenshots with `glReadPixels` | 7 |
| Procedural textures (asphalt, grass, concrete, facades) with mipmaps and trilinear filtering | 8 |
| Car as a transform hierarchy: body, wheels, headlamps and tail lights | 9 |
| Sparks and debris with gravity, bounce and friction; jump with gravity; wind sway in the vertex shader | 10 |

The game itself has traffic that follows and brakes, trucks, lane changes at higher levels, crashes, a score, a best score and a restart without closing the program.

---

## System requirements

| Component | Requirement |
| --- | --- |
| OS | Windows 10/11, Linux or macOS |
| GPU | Any GPU with an OpenGL 3.3 driver |
| Compiler | C++17: MSVC (Visual Studio 2022 or newer), GCC or Clang |
| Build tool | CMake 3.16 or newer (Make also works on Linux and macOS) |
| Libraries | GLFW 3.3+, GLEW, GLM |

---

## Build and run

Get the source first:

```bash
git clone https://github.com/andyrudovv/CompGraphFinal.git
cd CompGraphFinal
```

### Windows: Visual Studio (recommended)

1. In **Visual Studio Installer**, install the **Desktop development with C++** workload. It includes CMake.
2. Install the libraries with vcpkg. Run this in a normal **Windows PowerShell** window, not in a 32-bit (x86) developer prompt:

   ```powershell
   git clone https://github.com/microsoft/vcpkg C:\vcpkg
   C:\vcpkg\bootstrap-vcpkg.bat
   C:\vcpkg\vcpkg install glfw3:x64-windows glew:x64-windows glm:x64-windows
   ```

3. In Visual Studio choose **File → Open → Folder** and open the project folder.
4. Open **Project → CMake Settings**, set **CMake toolchain file** to `C:/vcpkg/scripts/buildsystems/vcpkg.cmake` and save.
5. Choose **Project → Delete Cache and Reconfigure**. The Output window (CMake) should end with `CMake generation finished`.
6. Select **laneRunner.exe** as the startup item and press **F5**.

### Windows: command line

From the project folder in PowerShell:

```powershell
cmake -S . -B build-win -A x64 -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build-win --config Release
.\build-win\Release\laneRunner.exe
```

If vcpkg is somewhere else, change `C:/vcpkg` in the commands.

### Linux (Debian / Ubuntu)

```bash
sudo apt install build-essential cmake pkg-config libgl1-mesa-dev libglfw3-dev libglew-dev libglm-dev
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/laneRunner
```

On Wayland the program may need XWayland.

### macOS

```bash
brew install cmake pkg-config glfw glew glm
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/laneRunner
```

### Make (Linux and macOS)

```bash
make
./laneRunner
```

`make run` builds and starts the game, `make clean` removes the build output.

### Where the shaders are found

CMake copies `shaders/` next to the executable and also stores the path of the build folder inside the program, so the game finds its shaders even when it is started from another folder. If you move the project, build it again.

---

## Controls

Press **F1** or **H** in the game to see all keys on screen. Letter keys work by their position on the keyboard, so the keyboard layout does not matter.

### Driving (every camera mode)

| Key | Action |
| --- | --- |
| A / Left | Move one lane left |
| D / Right | Move one lane right |
| W / Up | Accelerate while held |
| S / Down | Brake while held |
| Space | Jump (2.5 s cooldown); clears cars, not trucks |

### General

| Key | Action |
| --- | --- |
| F1 / H | Open or close the help panel (the game pauses while it is open) |
| P | Pause / resume |
| R | Restart the run |
| C | Next camera: Chase → Free-fly → Orbit → Chase |
| F2 | Save `screenshot.ppm` |
| Esc | Quit |

### Lighting and scene

| Key | Action |
| --- | --- |
| **5** | **Dynamic lighting ON / OFF (the main technique)** |
| 1 / 2 / 3 | Day / sunset / night preset (stops the automatic cycle) |
| 4 | Start or stop the automatic day/night cycle |
| [ / ] | Hold to turn the sun around the vertical axis |
| - / = | Hold to move the sun up or down along its daily path |
| B | Show / hide the Bezier hills |
| V | VSync ON / OFF (turn it off to measure real FPS) |

### Free-fly camera

| Input | Action |
| --- | --- |
| I / K | Fly forward / backward |
| J / L | Fly left / right |
| Q / E | Fly down / up |
| Shift | Fly faster |
| Mouse wheel | Fly forward / backward |
| Hold a mouse button and drag | Look around |

### Orbit camera

| Input | Action |
| --- | --- |
| Hold a mouse button and drag | Rotate around the car |
| Q / E | Rotate around the car |
| Mouse wheel or Z / X | Zoom in / out |

### Best shot for comparison

1. Start the game and press **3** for night.
2. Press **P** to pause.
3. Press **5** a few times to compare the same frame with and without dynamic lighting.
4. Press **C** twice for the Orbit camera and drag the mouse to look at the headlights from the side.
5. Press **F2** to save the frame.

---

## Command-line options

| Option | Action |
| --- | --- |
| `--time T` | Start the day/night cycle at T (0 to 1): 0.25 is noon, 0.48 sunset, 0.75 night |
| `--lighting-off` | Start with dynamic lighting off |
| `--no-vsync` | Start with VSync off |
| `--bench S` | Skip 2 s of warm-up, measure the average FPS over S seconds, print it and quit |
| `--frames N` | Quit after N frames |
| `--shot FILE` | Save the last frame as a PPM image (use together with `--frames`) |

Examples:

```bash
./build/laneRunner --time 0.75 --no-vsync --bench 10                  # FPS with lighting
./build/laneRunner --time 0.75 --no-vsync --bench 10 --lighting-off   # FPS without lighting
./build/laneRunner --frames 3 --time 0.75 --shot night.ppm            # night screenshot
./build/laneRunner --frames 3 --time 0.75 --lighting-off --shot off.ppm
```

On Windows use `laneRunner.exe` from the build folder instead of `./build/laneRunner`.

---

## How it works

### One frame

1. **Input.** GLFW events are read; key presses are stored once per frame.
2. **Simulation.** The game advances in fixed steps of 1/120 s: player, traffic, collisions, particles, jump and the day/night cycle.
3. **Sky pass.** One full-screen triangle; the fragment shader rebuilds the view direction for every pixel and draws the sky gradient, sun, moon and stars. Depth writes are off.
4. **Scene pass.** Every object is drawn with its model matrix and material. The scene shaders do texturing, lighting and fog, with depth test and back-face culling on.
5. **HUD pass.** Rectangles, numbers and text in pixel coordinates with alpha blending: score, speed, level, topic and team, FPS and the lighting state.

### Lighting (main technique)

`shaders/scene.frag` uses Blinn-Phong for every light. With the surface normal N, the direction to the light L, the direction to the camera V and the half vector H = normalize(L + V):

```text
diffuse  = lightColor * albedo * max(N · L, 0)
specular = lightColor * ks * max(N · H, 0)^shininess
```

- **Sun or moon:** one directional light. Its direction is (0.9 cos a, sin a, −0.35) with a = 2π · time of day, then turned around the vertical axis by the `[` and `]` keys. Below the horizon the moon takes over with a dim blue light.
- **Street lamps:** up to 8 point lights, attenuation `(1 − d / range)²`.
- **Headlights:** 2 spot lights; the cone is `smoothstep(cosOuter, cosInner, dot(−L, spotDir))`. They take their position and direction from the lamp nodes of the car, so they follow every turn and jump.
- **Fog:** `f = 1 − exp(−(d · density)²)`, then the colour is mixed towards the fog colour.
- **Key 5 OFF:** the shader skips the diffuse and specular terms of all lights and keeps ambient light, textures, glowing windows, fog and the sky. This is the comparison baseline.

### Bezier hills

`MeshBuilder::bezierPatch` in `engine/mesh.cpp` builds a bicubic Bezier surface from 16 control points:

```text
P(u, v) = Σi Σj Bi(u) · Bj(v) · Pij,   B0 = (1−t)³, B1 = 3t(1−t)², B2 = 3t²(1−t), B3 = t³
```

The surface is cut into 16 × 16 quads. The normal is the cross product of the partial derivatives ∂P/∂u and ∂P/∂v, so the light on the hills is smooth. All 12 border control points are at height 0, and the border of a Bezier patch depends only on its border points, so every hill meets the flat ground without a seam. One unit patch is built at startup and scaled differently for each hill.

### Jump

Space starts a jump with an upward speed of 10 m/s. Every step the speed drops by g · dt (g = 22 m/s²) and the height grows by speed · dt. The car goes up about v²/2g ≈ 2.3 m and stays in the air about 2v/g ≈ 0.9 s. The height is part of the car's model matrix, and the nose tilts up while the car rises. A car is skipped in the collision test while the player is above its roof (1.35 m for a car, 2.9 m for a truck).

### Car hierarchy

`engine::Node` keeps a local matrix for every part and computes `world = parentWorld × local`. The body is the root; wheels, headlamps and tail lights are its children. When the body moves, rolls or jumps, all parts follow, and the headlight spot lights read their position from the headlamp nodes.

### Wind

`shaders/scene.vert` moves the vertices of trees sideways with two sine waves. The higher a vertex is, the more it moves, so the trunk stays still and the top sways. Each tree gets its own phase from its position.

### Textures

`engine/texgen.cpp` creates all textures at startup from integer hashes and tileable value noise: asphalt, grass, concrete and building facades. The facade texture keeps lit windows in its alpha channel, and at night these windows glow. Textures use repeat wrapping, mipmaps, trilinear filtering and anisotropic filtering when the driver supports it. Road, grass and curb textures slide with the travelled distance, so the ground moves under the car.

### Particles and collisions

Sparks and debris have a position, velocity and lifetime, fall with gravity, bounce on the road and slow down with friction. Cars collide using boxes around their rotated shapes; a hit car is knocked away, spins and slides to the side.

### Score and difficulty

```text
score = int(distance_in_metres × 0.5) + overtakes × 25
```

The level goes up every 18 seconds. The base speed grows from 24 to 58 m/s, more traffic appears, and from level 3 the traffic starts changing lanes.

---

## Performance

The HUD shows FPS and the time of one frame in milliseconds, averaged over half a second. VSync limits FPS to the monitor refresh rate, so turn it off with **V** or `--no-vsync` before measuring.

Measured with `--bench 10` at night (`--time 0.75`), 1280×720, VSync off, Debug build on Windows 11:

| Case | Average FPS | Frame time |
| --- | --- | --- |
| Dynamic lighting ON | 344.3 | 2.90 ms |
| Dynamic lighting OFF | 344.5 | 2.90 ms |

The target is at least 30 FPS, so no quality switch is needed. In the Debug build the frame time is limited by the CPU (the game code is not optimised in Debug), which is why the cost of lighting on the GPU does not show in these numbers.

---

## Project structure

```text
engine/          reusable engine code
  window.*       GLFW window, OpenGL context, input, frame clock, screenshots
  shader.*       loading, compiling and linking GLSL programs
  mesh.*         meshes and MeshBuilder (boxes, cylinders, wedges, Bezier patches)
  renderer.*     materials, lights, environment and drawing
  camera.h       view and projection matrices
  node.h         transform hierarchy
  texgen.*       procedural textures
  texture.*      OpenGL textures with mipmaps
  hud.*          on-screen interface and bitmap font
  objloader.*    optional OBJ models
game/
  game.*         the game: scene, traffic, collisions, particles, cameras, HUD
  main.cpp       command-line options and the main loop
shaders/
  scene.vert/.frag   lighting, textures, fog, wind
  sky.vert/.frag     procedural sky
  hud.vert/.frag     interface
docs/            architecture, shaders, testing notes, course coverage map, report
screenshots/     images from our build
```

---

## Libraries and assets

| Library | Used for | Source and licence |
| --- | --- | --- |
| GLFW | Window, OpenGL context, keyboard and mouse | https://www.glfw.org/ (zlib/libpng) |
| GLEW | Loading OpenGL functions | https://glew.sourceforge.net/ (Modified BSD, MIT) |
| GLM | Vectors and matrices | https://github.com/g-truc/glm (MIT) |

The default scene uses no external models, images or fonts: all models are built from simple shapes in code and all textures are generated at startup. Optional `models/car.obj` and `models/truck.obj` can be placed in a `models/` folder; if they are missing, the built-in models are used.

---

## Team and module ownership

| Member | Module | Main work |
| --- | --- | --- |
| Andrey Rudov | Engine core and renderer | Window loop, shader loading, meshes, forward renderer, HUD, procedural sky and day/night cycle; headlight spot lights (week 5); car hierarchy `engine/node.h` (week 9); architecture, shader and testing docs |
| Dana Tutkyshbaeva | Gameplay and physics | Traffic AI, trucks, collisions and crashes, sparks and debris (week 10); OBJ loader (week 4); wind sway in the vertex shader (week 10) |
| Violetta Yun | Camera, input and interface | Procedural textures with mipmaps (week 8); fixed-step simulation and time presets; free-fly and orbit cameras with mouse input; help panel; dynamic-lighting toggle on key 5; README and CMake |
| Alikhan Mukhamed-Rakhym | Terrain, light control and performance | Bezier patch hills (week 6); keyboard control of the sun (week 5); FPS counter, VSync toggle and benchmark mode; jump; topic and team on screen; merging the team branches; course coverage map; Windows build testing |

`docs/COURSE_COVERAGE.md` maps the project to the weekly course topics. More details are in `docs/ARCHITECTURE.md`, `docs/SHADERS.md` and `docs/CONTROLS_AND_TESTING.md`.

---

## Requirements checklist

Based on the common requirements in *CG Final Project Requirements 2026–2027* (Sections 2–5).

| Requirement | Status | Where |
| --- | --- | --- |
| OpenGL 3.3+ core, own renderer, no game engine | Done | GLFW + GLEW + GLM, `engine/` |
| Windowed real-time program, ≥ 30 FPS at 1280×720 | Done | 344 FPS measured, see Performance |
| Perspective camera, resize-safe aspect ratio | Done | `engine/camera.h`, framebuffer size callback |
| Free-fly or orbit camera besides the game camera | Done | Key C |
| Vertex + fragment shaders with lighting | Done | `shaders/scene.*` |
| Depth test and back-face culling | Done | Scene pass |
| On-screen help (F1 / H) | Done | Help panel |
| Pause key | Done | P |
| Toggle key for the main technique | Done | Key 5, state shown in the HUD and window title |
| Light that can be moved or rotated | Done | `[` `]` `-` `=` keys; headlights move with the car |
| Errors printed to the console | Mostly | Shader, context and OBJ errors are printed; texture upload has no extra check |
| README with OS, compiler, build, run and keys | Done | This file |
| Windows build without undocumented clicks | Done | CMake + vcpkg, steps above |
| Shaders loaded from files | Done | `shaders/` |
| Each member owns a module | Done | Team table above |
| Start, score, fail and reset; play for over a minute | Done | Endless run, crash, R to restart |
| Team names, topic, score and technique state on screen | Done | Top bar and bottom HUD |
| Technique visible in the first 15 seconds | Done | Lighting is visible from the first frame; 3 shows night at once |

---

## Known limitations

- The best score is kept only while the program runs.
- Collisions use simple boxes; very fast cars could pass through each other in rare cases.
- Texture upload and screenshot saving do not report every possible error.
- There is no quality switch, because the measured frame rate is far above 30 FPS.
