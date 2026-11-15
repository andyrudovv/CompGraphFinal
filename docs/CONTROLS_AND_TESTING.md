# Controls, best shot, testing

Keys are authoritative in the README (`## Controls`); this page adds how to reproduce the demo and how to measure.

## Best-looking shot

```bash
./build/laneRunner            # starts at dayTime 0.12 with the cycle ON
```

1. Press `3` (night): headlight spot cones on the road, street-lamp point lights, lit windows on the towers, stars.
2. Press `C` until `CAMERA: ORBIT`, drag with the mouse to a three-quarter view of the car.
3. Press `P` to freeze the action, `F2` to save `screenshot.ppm`.

Deterministic captures (no input needed):

```bash
./build/laneRunner --frames 3 --time 0.25 --shot day.ppm
./build/laneRunner --frames 3 --time 0.48 --shot sunset.ppm
./build/laneRunner --frames 3 --time 0.75 --shot night.ppm
```

Headline-technique comparison available today: `1` / `2` / `3` switch lighting presets (day, sunset, night); `4` freezes the cycle. There is **no** single ON/OFF key for the headline technique yet (see README, requirements status).

## Testing methodology

There are no automated unit tests in the repository. What exists:

| Check | How | Result source |
| --- | --- | --- |
| Smoke test, start and exit | `./build/laneRunner --frames 3` must exit with code 0 | exit code |
| Scene regression by eye | Compare the three `--shot` images with `screenshots/*.png` | manual |
| Shader/OBJ/context errors | Written to stderr at start-up | console |
| Resize | Drag the window edge; projection must stay undistorted | manual |
| Pause / restart / crash flow | `P`, crash into traffic, `R`; `F1` pauses while help is open | manual |
| Long run | Play > 60 s without restarting the process | manual |

## FPS measurement (to be filled in by the team)

No measurements were taken by the author of this file, so the table is intentionally empty. Do not invent numbers.

How to measure on the lab PC (1280×720, default scene):

- Linux/Mesa: `vblank_mode=0 ./build/laneRunner` to disable vsync, with an FPS overlay (for example MangoHud: `mangohud ./build/laneRunner`).
- Windows: GPU-Z/MSI Afterburner overlay or the NVIDIA/AMD overlay. Note that `glfwSwapInterval(1)` in `engine/window.cpp` caps FPS at the monitor refresh rate; set it to 0 for the benchmark and write that down.
- Record each case for 30 s and take the average.

| Case | Camera | Time of day | Avg FPS | Min FPS | GPU / driver |
| --- | --- | --- | --- | --- | --- |
| Day | chase | `1` | | | |
| Night (spots + lamps) | chase | `3` | | | |
| Night | orbit | `3` | | | |
| Crash scene (sparks + debris) | chase | `3` | | | |

Requirement: ≥ 30 FPS at 1280×720. If a case is below, a quality toggle is required (none exists yet).

## Known issues

See "Current limitations and remaining work" in the README. The freeglut port and Bezier hills are on branch `alikhan` and untested together with `main`.
