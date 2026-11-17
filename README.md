# Lane Runner

We built Lane Runner as an endless three-lane driving game and a small custom 3D engine using **C++17 and OpenGL 3.3 Core**. Change lanes, overtake traffic, and earn points. Lighting, the procedural sky, fog, headlights, and illuminated windows change throughout a day/night cycle.

- **Course:** Computer Graphics Fundamentals, Astana IT University.
- **Team:** Moggers.
- **Team members:** Andrey Rudov, Violetta Yun, Alikhan Mukhamed-Rakhym, Dana Tutkyshbaeva.
- **Repository:** <https://github.com/andyrudovv/CompGraphFinal>.
- **Supported platforms:** Linux, Windows, and macOS.

Our project separates reusable rendering and platform code in `engine/` from game logic in `game/`.

## Features

- Custom GLFW window loop, OpenGL function loading through GLEW, and GLM mathematics; no full game engine is used.
- Indexed VAO/VBO/EBO meshes and external GLSL 330 vertex/fragment shaders.
- Forward rendering with Blinn–Phong lighting: one directional light, up to eight point lights, and up to four spotlights in the shader. The game uses two spotlights for the player's headlights and up to eight nearby street lamps.
- Perspective camera with Chase, Free-fly, and Orbit modes; the aspect ratio follows the current framebuffer size.
- Procedural sky with the sun, moon, and stars; a 90-second lighting cycle; fixed day, sunset, and night presets.
- Procedural asphalt, grass, concrete, and facade textures; mipmaps, trilinear filtering, and up to 8× anisotropic filtering when the extension is supported.
- Hierarchical car transforms, rotating wheels, and headlights that follow the car.
- Roadside hills built from bicubic Bezier patches with exact normals; key B shows or hides them.
- Sedans and trucks, traffic following and braking, lane changes at later levels, collisions, sparks, and debris.
- Fixed 120 Hz simulation, pause, restart without closing the application, on-screen help, and PPM screenshots.
- Independent dynamic-lighting ON/OFF comparison with key 5 and a persistent HUD indicator.

## System requirements

| Component | Requirement |
| --- | --- |
| OS | macOS, Linux, or Windows 10 with an OpenGL 3.3+ graphics driver |
| Compiler | C++17 support: AppleClang/Clang, GCC, or MSVC |
| Build tools | CMake 3.16+; alternatively GNU Make and pkg-config on Linux/macOS |
| Libraries | OpenGL, GLFW 3.3+, GLEW, GLM |
| Display | Default window size: 1280×720; the actual framebuffer may be larger on Retina displays |

We use CMake to build the project on Linux, Windows, and macOS. Platform-specific dependency installation and launch commands are provided below.

## Getting the source

```bash
git clone https://github.com/andyrudovv/CompGraphFinal.git
cd CompGraphFinal
```

Build from source. Existing executables and an old `build/` directory from another system should not be reused.

## Building and running

### macOS

Install Command Line Tools for Xcode and Homebrew. If the compiler tools are not installed:

```bash
xcode-select --install
```

Install the dependencies and build:

```bash
brew install cmake pkg-config glfw glew glm
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/laneRunner
```

### Debian and Ubuntu Linux

```bash
sudo apt update
sudo apt install build-essential cmake pkg-config libgl1-mesa-dev libglfw3-dev libglew-dev libglm-dev
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/laneRunner
```

A working graphical session and OpenGL driver are required. With GLFW builds that support platform selection, the application prefers X11 when available; a Wayland session may therefore need XWayland.

### Windows 10 with MSVC and vcpkg

The following commands use **PowerShell**. Install Git, CMake, and Visual Studio 2022 Build Tools with the MSVC C++ x64 tools and Windows SDK. In Visual Studio Installer, select the “Desktop development with C++” workload. Project configuration and compilation then use the commands below.

First, install the libraries in a separate vcpkg directory:

```powershell
git clone https://github.com/microsoft/vcpkg.git C:/dev/vcpkg
& C:/dev/vcpkg/bootstrap-vcpkg.bat
& C:/dev/vcpkg/vcpkg.exe install glfw3:x64-windows glew:x64-windows glm:x64-windows
```

From the project root, run:

```powershell
cmake -S . -B build-win -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE=C:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows
cmake --build build-win --config Release --parallel
.\build-win\Release\laneRunner.exe
```

If vcpkg is installed elsewhere, replace `C:/dev/vcpkg` in all commands. If required, add the dynamic library directory to PATH before launching:

```powershell
$env:PATH = "C:\dev\vcpkg\installed\x64-windows\bin;" + $env:PATH
.\build-win\Release\laneRunner.exe
```

### Alternative Make build

On macOS/Linux, after installing the dependencies:

```bash
make
./laneRunner
```

`make run` builds and launches the game. `make clean` removes the `build/` directory and the executable in the project root, including any CMake build outputs stored in that directory.

### Shader file lookup

CMake copies `shaders/` into the build directory and embeds its absolute location in the executable. The application can then be launched from another working directory as long as that build directory remains in place. Reconfigure and rebuild after moving the project or build directory.

The Make build embeds the absolute source root. The loader also checks relative paths in the current directory and one and two levels above it. A copied executable without accessible shader files is not a standalone distribution.

## Controls

Letter keys refer to a Latin keyboard layout. Driving controls remain available in every camera mode.

| Key | Action |
| --- | --- |
| Left / A | Change to the lane on the left |
| Right / D | Change to the lane on the right |
| Up / W | Accelerate while held |
| Down / S | Brake while held |
| P | Pause / resume the simulation |
| R | Restart the run; keep the best score for the current process |
| F1 / H | Open / close on-screen help; opening help pauses the simulation |
| C | Cycle through Chase → Free-fly → Orbit → Chase |
| 1 | Set daytime (`t = 0.25`) and stop the automatic cycle |
| 2 | Set sunset (`t = 0.48`) and stop the automatic cycle |
| 3 | Set night (`t = 0.75`) and stop the automatic cycle |
| 4 | Enable / disable automatic time-of-day progression |
| 5 | Toggle dynamic Blinn–Phong lighting ON/OFF |
| B | Show / hide the Bezier hills |
| V | Toggle VSync (use OFF for FPS measurements) |
| [ / ] | Hold to turn the sun (light direction) around the vertical axis |
| - / = | Hold to move the sun along its daily arc; stops the automatic cycle |
| F2 | Save `screenshot.ppm` in the current working directory |
| Esc | Close the application |

P and R are ignored while help is open. Closing help restores the previous normal pause state. Camera controls and lighting presets remain available while the simulation is paused.

**Free-fly:** I/K move forward/backward, J/L move left/right, and Q/E move down/up. Hold Shift to move faster. Drag with either the left or right mouse button to look around. The mouse wheel moves the camera forward/backward.

**Orbit:** drag with either the left or right mouse button to rotate around the car; Q/E rotate horizontally. Use the mouse wheel or Z/X to zoom in/out.

**Technique comparison:** press `5` to enable or disable dynamic Blinn–Phong lighting. OFF skips directional, point, and spotlight diffuse/specular calculations; ambient light, textures, emissive surfaces, fog, and the sky remain visible as the baseline. The HUD and window title show `DYNAMIC LIGHTING: ON/OFF`. This works while paused, in help, after a crash, and in all camera modes. Restart preserves the selected lighting state. Key `4` independently freezes or resumes time-of-day progression.

For a visual comparison, press `P`, select `3` for fixed night lighting, then press `5` to compare the same scene with and without dynamic lighting. Close help with F1/H if needed. For FPS measurements, use the same resolution, camera, scene, VSync setting, and measurement interval in both modes. The HUD shows the current FPS and frame time (averaged over 0.5 s), and `--bench` prints a reproducible average, for example `laneRunner --time 0.75 --no-vsync --bench 10` and the same with `--lighting-off`.

## Gameplay and interface

The game starts a run immediately. Drive as far as possible and overtake traffic while avoiding collisions. There is no separate start menu. Press R after a crash to begin another run without restarting the process. Runs have no time limit.

The player's car stays near `z = 0`, while the environment and traffic move relative to it. New traffic appears approximately 185 metres ahead and is removed after passing behind the camera. Road markings and scenery are recycled.

The score is calculated in `Game::update`:

```text
score = int(distance_in_metres * 0.5) + overtakes * 25
```

The difficulty level increases every 18 seconds. Base speed rises from 24 to 58 m/s; acceleration and braking apply an additional multiplier. The base traffic spawn interval decreases from 1.15 to 0.30 seconds, with an additional random multiplier. Traffic can change lanes starting at level 3. Space checks reduce the risk of blocking the road, but do not guarantee that every collision scenario is survivable.

The HUD shows the score, speed in km/h and a speed bar, difficulty level, night intensity, and overtakes. After a crash, it displays the final score and the best score for the current process. The best score is not saved to disk. An F1/H hint and the dynamic-lighting ON/OFF state appear at the bottom; the window title also shows the automatic cycle and camera mode. Team names and the exact assigned topic title are not yet displayed on screen.

## Graphics implementation

### Rendering sequence

1. The camera builds a view matrix with `glm::lookAt` and a perspective projection using the current aspect ratio.
2. The renderer clears the depth buffer and uploads environment and light parameters to the scene shader.
3. A full-screen triangle draws the sky. Its fragment shader reconstructs the view ray using the inverse view-projection matrix and evaluates the gradient, sun, moon, and stars. Depth testing and depth writes are temporarily disabled.
4. The road, scenery, traffic, debris, and sparks render into the window framebuffer with depth testing and back-face culling. The normal matrix is the inverse transpose of the model matrix's linear part.
5. The HUD renders over the scene with alpha blending; depth testing and culling are temporarily disabled, then restored.

This version does not use separate FBOs, deferred shading, shadow maps, or bloom. Bright windows and lamps use emissive terms, which do not themselves implement bloom.

### Lighting and fog

`shaders/scene.frag` uses Blinn–Phong lighting. For the direction toward the light `L`, surface normal `N`, and direction toward the camera `V`:

```text
H = normalize(L + V)
diffuse = lightColor * albedo * max(dot(N, L), 0)
specular = lightColor * materialSpecular * max(dot(N, H), 0)^shininess
```

Specular lighting is added only on the lit side. Point lights use attenuation of `clamp(1 - distance / range, 0, 1)^2`. Spotlights multiply this by a smooth cone mask computed with `smoothstep(cosOuter, cosInner, dot(-L, spotDirection))`.

Exponential-squared fog is applied after lighting, emission, and window glow:

```text
fog = 1 - exp(-(distanceToCamera * fogDensity)^2)
finalColor = mix(surfaceColor, fogColor, clamp(fog, 0, 1))
```

### Day and night

In `Game::updateDayNight`, time of day advances by `dt / 90`. The sun's position is calculated with sine and cosine; its elevation controls the directional light's colour and intensity, ambient lighting, sky, and fog. At night, the main light direction is reversed to represent moonlight. Headlights and street lamps turn on gradually.

### Textures and transforms

`engine/texgen.cpp` generates RGBA textures at startup. UVs are created while building meshes. Road, grass, and curb UV offsets follow the distance travelled; facade UV scales account for building dimensions. Facade alpha is an illuminated-window mask, not surface transparency.

`engine::Node` computes `world = parentWorld * local`. The car hierarchy updates wheels and headlight mounts; spotlight directions are derived from the mounts' world transforms.

### Bezier hills

`MeshBuilder::bezierPatch` (`engine/mesh.cpp`) evaluates a bicubic Bezier surface `P(u,v) = Σ Σ B_i(u) B_j(v) P_ij` from 16 control points with the cubic Bernstein basis, on a 16×16 grid. Normals come from the cross product of the partial derivatives `∂P/∂u` and `∂P/∂v`. The 12 border control points are at height 0, so each hill meets the flat ground without a seam. One unit patch is built at startup and scaled per hill.

### Simulation

The simulation uses a time accumulator with a fixed step of `1/120` second. Key press edges are handled once per rendered frame, pausing clears the accumulator, and incoming frame delta is capped at 0.1 seconds. Collisions use conservative axis-aligned bounds around rotated cars and simplified impulses. Physics is arcade-style; continuous collision detection is not implemented.

## Source layout

| Path | Purpose |
| --- | --- |
| `game/main.cpp` | Entry point, command-line arguments, main loop, screenshots |
| `game/game.h`, `game/game.cpp` | Scene, gameplay loop, traffic, collisions, lighting, cameras, and HUD |
| `engine/window.*` | GLFW/OpenGL context, input, resizing, frame clock, PPM output |
| `engine/shader.*` | File lookup, GLSL compilation/linking, uniform location cache |
| `engine/camera.h` | View and projection matrices |
| `engine/mesh.*` | GPU meshes and primitive geometry builders |
| `engine/node.h` | Local/world transform hierarchy |
| `engine/renderer.*` | Sky pass, materials, lights, and draw calls |
| `engine/texgen.*`, `engine/texture.*` | Texture generation, OpenGL upload, and filtering |
| `engine/objloader.*` | Optional OBJ geometry and MTL material colour loading |
| `engine/hud.*` | Rectangles, numeric indicators, and built-in text glyphs |
| `shaders/scene.*` | Texturing, Blinn–Phong, point/spot lights, and fog |
| `shaders/sky.*` | Procedural sky |
| `shaders/hud.*` | On-screen interface |
| `screenshots/` | Saved day, sunset, and night scene images |
| `CMakeLists.txt`, `Makefile` | Build configuration and dependencies |

## Command-line options and scene reproduction

| Option | Action |
| --- | --- |
| `--frames N` | Exit after N rendered frames when N > 0 |
| `--shot FILE` | Save the final frame as PPM when exiting through `--frames` |
| `--no-vsync` | Start with VSync off so the frame rate is not capped by the display |
| `--bench S` | Skip 2 s of warm-up, measure the average FPS over S seconds, print it to the console and exit |
| `--time T` | Set the initial time of day; the normal range is `[0, 1)` |
| `--lighting-off` | Start with dynamic lighting disabled; key 5 can turn it back on |

`--time` does not disable the automatic cycle. Without a positive `--frames` value, `--shot` does not save an image on normal exit. Changing the filename extension does not change the PPM output format.

```bash
./build/laneRunner --frames 3 --time 0.25 --shot day.ppm
./build/laneRunner --frames 3 --time 0.48 --shot sunset.ppm
./build/laneRunner --frames 3 --time 0.75 --shot night.ppm
./build/laneRunner --frames 3 --time 0.75 --lighting-off --shot night-off.ppm
```

To explore the scene, launch the game, press P, select 1/2/3, and switch cameras with C. The night preset immediately shows headlights, street lamps, and illuminated windows without waiting for a full cycle. Open help with F1/H and capture a screenshot with F2.

## Libraries and assets

| Library | Purpose | Source and licence |
| --- | --- | --- |
| GLFW | Window, context, keyboard, and mouse | <https://www.glfw.org/> — zlib/libpng |
| GLEW | OpenGL function loading | <https://glew.sourceforge.net/> — Modified BSD, MIT, and licences of included components |
| GLM | Vectors and matrices | <https://github.com/g-truc/glm> — MIT |

We link GLFW, GLEW, and GLM separately rather than bundling them with our source. Our default scene builds models from primitives and generates textures procedurally, so it needs no external models, images, or fonts. We keep our GLSL shader code in the `shaders/` directory.

Optional `models/car.obj` and `models/truck.obj` files can be placed in a `models/` directory at the project root and used when launching from that root. The loader supports OBJ geometry and MTL `Kd` colours, not full material and texture import. CMake automatically copies only `shaders/`, so external models should not be assumed to ship with the executable. Retain source information and redistribution permission for any models added later.

## Team and module ownership

| Member | Graphics contribution (course topic) | Other |
| --- | --- | --- |
| Alikhan Mukhamed-Rakhym | Port to freeglut (week 3) and Bezier patch `bezierPatch` with roadside hills (week 6): branch `alikhan`, not merged into `main` yet | Windows testing, demo video |
| Andrey Rudov | Headlight spot lights in `scene.frag` (week 5); hierarchical car model body → wheels → lights, `engine/node.h` (week 9) | PR review, merges into `main`, build, report |
| Dana Tutkyshbaeva | Planned: `.obj` car and truck models (week 4), wind sway in the vertex shader (week 10). `engine/objloader.*` exists; no models or wind shader are pushed yet | Gameplay logic, slides |
| Violetta Yun | Planned: sky reflection on the car body via environment mapping (week 8), not pushed yet. Done: free-fly/orbit cameras, mouse input, help panel | Texture fixes, README |

Status of every course topic: `docs/COURSE_COVERAGE.md`. Details: `docs/ARCHITECTURE.md`, `docs/SHADERS.md`, `docs/CONTROLS_AND_TESTING.md`.

## Requirements status

The table below summarizes our current implementation against the common technical and README requirements in `CG_Final_Project_Requirements_2026-2027.docx`.

| Requirement | Status | Implementation or remaining work |
| --- | --- | --- |
| OpenGL 3.3+ Core and a custom renderer | Implemented | GLFW 3.3 Core context, GLSL 330, and our own rendering code |
| Windowed real-time application | Implemented | Default 1280×720 window and update/render loop |
| ≥30 FPS on a typical lab PC at 1280×720 | Implemented | The team measured more than 1000 FPS in the default scene at 1280×720; GPU, driver and per-case minimum values are not recorded yet, so no quality toggle is needed |
| Perspective/view matrices and resize-safe aspect ratio | Implemented | Framebuffer dimensions update and the current aspect ratio is passed to the camera |
| Free-fly or Orbit in addition to the game camera | Implemented | C cycles through Chase, Free-fly, and Orbit |
| Lit programmable vertex/fragment shaders | Implemented | Scene shaders with Blinn–Phong lighting and textures |
| Depth testing and consistent back-face culling | Implemented | Enabled for 3D; temporarily changed for sky/HUD |
| F1/H on-screen control help | Implemented | Help panel and bottom-screen hint |
| Movable light or rotatable light direction | Implemented | [ and ] turn the sun around the vertical axis, - and = move it along its arc; headlights follow car transforms |
| Dynamic lighting ON/OFF toggle | Implemented | 5 toggles directional, point and spot lighting; state displayed in HUD and window title |
| Movable light or rotatable light direction | Implemented | Sun direction changes; headlights follow car transforms |
| Missing-resource and shader-error diagnostics | Partial | Shader/OBJ/context errors are reported; texture uploads and screenshots need complete error reporting; no custom FBOs are used |
| README: OS, compiler, build, run, and exact keys | Included | Dependencies, platform-specific commands, controls, and launch options are listed above |
| Windows build path | Included | CMake supports MSVC; Windows 10 build instructions use vcpkg |
| Shaders loaded from separate files | Implemented | Three shader pairs in `shaders/` |
| Named owner for each module | Included | See "Team and module ownership" |
| Initial state, score, failure, and reset | Implemented | Playing state at launch, score, Crashed state, and restart with R |
| Play for at least one minute without restarting the process | Supported | Endless gameplay with no time limit and an in-process restart |
| On-screen team names, exact topic, and technique state | Partial | Score, Cycle ON/OFF and Dynamic Lighting ON/OFF are displayed; team names and the exact assigned topic title remain to be added |
| Technique visible within the first 15 seconds | Scene visible immediately | The scene appears at launch, and 1/2/3 select lighting presets directly |

### Current limitations and remaining work

- We still need to add team names and the exact assigned topic title to the on-screen interface and document module ownership.
- Texture upload errors and screenshot write failures need more complete diagnostic messages.
- We still need to record the GPU, driver, and minimum FPS for the reported >1000 FPS result. The current version has no quality toggle, which is not needed at this frame rate.
- The best score is stored only for the current process; collision physics uses simplified bounds and impulses without continuous collision detection.

The supplied requirements file references Section 6 but does not contain it. Our topic-specific feature checklist therefore still needs to be matched against the official assigned topic.
