#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "game.h"       // pulls in GLEW, which must come before GLFW
#include <GLFW/glfw3.h>

int main(int argc, char** argv) {
    int frameLimit = 0;          // --frames N: quit after N frames (smoke test)
    std::string shotPath;        // --shot file.ppm: save the last frame
    float startTime = -1.0f;     // --time 0..1: where the day/night cycle starts

    for (int i = 1; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--frames") && i + 1 < argc) frameLimit = std::atoi(argv[++i]);
        else if (!std::strcmp(argv[i], "--shot") && i + 1 < argc) shotPath = argv[++i];
        else if (!std::strcmp(argv[i], "--time") && i + 1 < argc) startTime = std::atof(argv[++i]);
    }

    engine::Window window;
    if (!window.create(1280, 720, "Lane Runner - OpenGL 3.3")) return 1;

    game::Game game;
    if (!game.init()) {
        std::fprintf(stderr, "failed to initialise the game (is shaders/ next to the binary?)\n");
        return 1;
    }
    if (startTime >= 0.0f) game.setDayTime(startTime);

    int frame = 0;
    while (!window.shouldClose()) {
        window.beginFrame();
        game.update(window);
        game.render(window);

        if (window.keyPressed(GLFW_KEY_F2)) {
            engine::saveScreenshot("screenshot.ppm", window.width(), window.height());
        }
        if (frameLimit > 0 && ++frame >= frameLimit) {
            if (!shotPath.empty()) engine::saveScreenshot(shotPath, window.width(), window.height());
            window.close();
        }
        window.endFrame();
    }
    return 0;
}
