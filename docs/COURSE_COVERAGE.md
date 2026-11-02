# Course coverage map

How Lane Runner lines up with the weekly topics of Computer Graphics Fundamentals.
Status: OK = covered, PART = partly, MISSING = not in the project yet.

| Week | Lecture topic | Where it is in the project | Status |
|---|---|---|---|
| 1 | OpenGL background: retained vs immediate mode, state machine, pipeline | Core profile only, no immediate mode. `engine/mesh.cpp`: VAO / VBO / EBO | OK |
| 2 | Math basics, shaders and GLSL: read / compile / link, attributes, uniforms | `engine/shader.cpp` (compile, link, uniform cache), `shaders/*.vert/.frag`; vectors and matrices via GLM | OK |
| 3 | Input and interaction (GLUT callbacks, double buffering), geometry and primitives | Keyboard in `engine/window.cpp` through GLFW polling, double buffering via swap; triangles and indexed primitives | PART: the course uses GLUT/freeglut callbacks, the project uses GLFW |
| 4 | Transformations, building models (vertex lists, meshes) | Model matrices (translate / rotate / scale, spinning wheels, car roll and yaw), `MeshBuilder`, `engine/objloader.cpp` (no model files in the repo yet) | OK, but no .obj models yet |
| 5 | Viewing (lookAt, projection), shading (Phong model) | `engine/camera.h` chase camera, perspective with variable FOV; Blinn-Phong in `scene.frag`: 1 directional + 8 point lights, materials, fog | OK (no flat / Gouraud mode, no spot light) |
| 6 | Curves and surfaces (Bezier, splines) | Nothing yet | MISSING |
| 7 | Buffers: depth, blending, reading buffers, fragment tests | Depth test, back buffer, alpha blending in the HUD, MSAA, `glReadPixels` screenshot | PART: no stencil, no framebuffer objects |
| 8 | Texture mapping: mipmaps, filtering, mapping methods | `engine/texgen.cpp`, `engine/texture.cpp`: procedural textures, repeat wrap, mipmaps, trilinear + anisotropic filtering, scrolling UVs, lit-window mask | OK (no environment or bump mapping) |
| 9 | Hierarchical models (tree / DAG), solid models | Car body + wheels + lights drawn from one parent matrix; no general scene-graph structure | PART |
| 10 | Procedural methods: particle systems, Newtonian dynamics, collisions | Sparks and debris with gravity, bounce, restitution and friction; wrecks and car-to-car collisions; procedural sky and textures | OK (no wave-motion vertex shader, no fractals) |

## Gaps to close

1. Week 3: port window / input / main loop from GLFW to freeglut (callbacks), add a Visual Studio project and update the CMake build.
2. Week 6: add a Bezier patch to `MeshBuilder` and use it in the scene (for example, roadside hills).
3. Week 4: put real .obj car models into `models/` so the loader is actually used.
4. Week 9: a small tree-structured model (car -> wheels, lights) instead of ad-hoc matrices.
5. Optional: spot light for the headlights (week 5), stencil or framebuffer demo (week 7), wave-motion vertex shader (week 10).
6. Report in research-paper format, README with a "who did what" section.
