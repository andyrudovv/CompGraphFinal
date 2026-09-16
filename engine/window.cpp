#include "window.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <cstdio>
#include <vector>

namespace engine {

static void onResize(GLFWwindow* w, int width, int height) {
    glViewport(0, 0, width, height);
    (void)w;
}

Window::~Window() {
    if (handle_) glfwDestroyWindow(handle_);
    glfwTerminate();
}

bool Window::create(int width, int height, const std::string& title) {
#ifdef GLFW_PLATFORM
    // GLEW resolves entry points through GLX, so prefer X11/XWayland when present.
    if (glfwPlatformSupported(GLFW_PLATFORM_X11)) glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
#endif
    if (!glfwInit()) {
        std::fprintf(stderr, "glfwInit failed\n");
        return false;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    handle_ = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!handle_) {
        std::fprintf(stderr, "cannot create an OpenGL 3.3 core window\n");
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(handle_);
    glfwSwapInterval(1);
    glfwSetFramebufferSizeCallback(handle_, onResize);

    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::fprintf(stderr, "glewInit failed: %s\n", glewGetErrorString(err));
        return false;
    }
    glGetError();  // GLEW leaves a spurious INVALID_ENUM behind on core profiles

    glfwGetFramebufferSize(handle_, &width_, &height_);
    glViewport(0, 0, width_, height_);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_MULTISAMPLE);

    last_ = static_cast<float>(glfwGetTime());
    return true;
}

bool Window::shouldClose() const { return !handle_ || glfwWindowShouldClose(handle_); }

void Window::close() { if (handle_) glfwSetWindowShouldClose(handle_, GLFW_TRUE); }

void Window::beginFrame() {
    glfwPollEvents();
    glfwGetFramebufferSize(handle_, &width_, &height_);

    time_ = static_cast<float>(glfwGetTime());
    dt_ = time_ - last_;
    last_ = time_;
    if (dt_ > 0.1f) dt_ = 0.1f;  // clamp hitches so nothing tunnels through a car

    for (int k = 0; k < KEY_COUNT; ++k) {
        prev_[k] = curr_[k];
        curr_[k] = glfwGetKey(handle_, k) == GLFW_PRESS;
    }
}

void Window::endFrame() { glfwSwapBuffers(handle_); }

bool Window::keyDown(int key) const {
    return key >= 0 && key < KEY_COUNT && curr_[key];
}

bool Window::keyPressed(int key) const {
    return key >= 0 && key < KEY_COUNT && curr_[key] && !prev_[key];
}

float Window::aspect() const {
    return height_ > 0 ? static_cast<float>(width_) / static_cast<float>(height_) : 1.0f;
}

void Window::setTitle(const std::string& title) {
    if (handle_) glfwSetWindowTitle(handle_, title.c_str());
}

bool saveScreenshot(const std::string& path, int width, int height) {
    std::vector<unsigned char> pixels(static_cast<size_t>(width) * height * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

    std::FILE* f = std::fopen(path.c_str(), "wb");
    if (!f) return false;
    std::fprintf(f, "P6\n%d %d\n255\n", width, height);
    for (int y = height - 1; y >= 0; --y) {  // OpenGL reads bottom-up
        std::fwrite(pixels.data() + static_cast<size_t>(y) * width * 3, 1, width * 3, f);
    }
    std::fclose(f);
    return true;
}

}  // namespace engine
