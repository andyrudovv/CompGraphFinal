#include "window.h"

#include <GL/glew.h>  // must come before freeglut, which pulls in gl.h
#if defined(_WIN32)
#include <GL/wglew.h>
#elif defined(__linux__)
#include <GL/glxew.h>
#endif
#include <GL/freeglut.h>

#include <cctype>
#include <chrono>
#include <cstdio>
#include <vector>

namespace engine {

namespace {

// freeglut callbacks are plain C functions, so they reach the window through this pointer.
Window* g_window = nullptr;

double nowSeconds() {
    using clock = std::chrono::steady_clock;
    static const clock::time_point start = clock::now();
    return std::chrono::duration<double>(clock::now() - start).count();
}

int mapAscii(unsigned char c) {
    switch (std::tolower(c)) {
    case 27: return KeyEscape;
    case 'w': return KeyW;
    case 'a': return KeyA;
    case 's': return KeyS;
    case 'd': return KeyD;
    case 'p': return KeyP;
    case 'r': return KeyR;
    case '1': return Key1;
    case '2': return Key2;
    case '3': return Key3;
    case '4': return Key4;
    default: return -1;
    }
}

int mapSpecial(int k) {
    switch (k) {
    case GLUT_KEY_LEFT: return KeyLeft;
    case GLUT_KEY_RIGHT: return KeyRight;
    case GLUT_KEY_UP: return KeyUp;
    case GLUT_KEY_DOWN: return KeyDown;
    case GLUT_KEY_F2: return KeyF2;
    default: return -1;
    }
}

void cbKeyboard(unsigned char c, int, int) { if (g_window) g_window->onKey(mapAscii(c), true); }
void cbKeyboardUp(unsigned char c, int, int) { if (g_window) g_window->onKey(mapAscii(c), false); }
void cbSpecial(int k, int, int) { if (g_window) g_window->onKey(mapSpecial(k), true); }
void cbSpecialUp(int k, int, int) { if (g_window) g_window->onKey(mapSpecial(k), false); }
void cbReshape(int w, int h) { if (g_window) g_window->onReshape(w, h); }
void cbClose() { if (g_window) g_window->onClose(); }
void cbDisplay() {}  // the game draws from its own loop, freeglut only needs a callback here

}  // namespace

Window::~Window() {
    if (id_) glutDestroyWindow(id_);
    if (g_window == this) g_window = nullptr;
}

bool Window::create(int width, int height, const std::string& title) {
    int argc = 1;
    char arg0[] = "laneRunner";
    char* argv[] = { arg0, nullptr };
    glutInit(&argc, argv);

    glutInitContextVersion(3, 3);
    glutInitContextProfile(GLUT_CORE_PROFILE);
    glutInitContextFlags(GLUT_FORWARD_COMPATIBLE);
    glutSetOption(GLUT_MULTISAMPLE, 4);
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_CONTINUE_EXECUTION);

    unsigned int mode = GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_MULTISAMPLE;
    glutInitDisplayMode(mode);
    if (!glutGet(GLUT_DISPLAY_MODE_POSSIBLE)) {  // no multisampled visual: run without MSAA
        glutInitDisplayMode(mode & ~GLUT_MULTISAMPLE);
    }

    glutInitWindowSize(width, height);
    id_ = glutCreateWindow(title.c_str());
    if (!id_) {
        std::fprintf(stderr, "cannot create an OpenGL 3.3 core window\n");
        return false;
    }
    g_window = this;

    glutDisplayFunc(cbDisplay);
    glutReshapeFunc(cbReshape);
    glutKeyboardFunc(cbKeyboard);
    glutKeyboardUpFunc(cbKeyboardUp);
    glutSpecialFunc(cbSpecial);
    glutSpecialUpFunc(cbSpecialUp);
    glutCloseFunc(cbClose);
    glutIgnoreKeyRepeat(1);  // held keys are tracked from the down/up events

    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::fprintf(stderr, "glewInit failed: %s\n", glewGetErrorString(err));
        return false;
    }
    glGetError();  // GLEW leaves a spurious INVALID_ENUM behind on core profiles

    // freeglut has no swap-interval call of its own: ask the platform for vsync.
#if defined(_WIN32)
    if (WGLEW_EXT_swap_control) wglSwapIntervalEXT(1);
#elif defined(__linux__)
    if (GLXEW_EXT_swap_control) glXSwapIntervalEXT(glXGetCurrentDisplay(), glXGetCurrentDrawable(), 1);
#endif

    width_ = glutGet(GLUT_WINDOW_WIDTH);
    height_ = glutGet(GLUT_WINDOW_HEIGHT);
    glViewport(0, 0, width_, height_);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_MULTISAMPLE);

    last_ = nowSeconds();
    return true;
}

void Window::onKey(int key, bool down) {
    if (key < 0 || key >= KeyCount) return;
    down_[key] = down;
    if (down) pressed_[key] = true;
}

void Window::onReshape(int width, int height) {
    width_ = width;
    height_ = height;
    glViewport(0, 0, width, height);
}

void Window::onClose() {
    id_ = 0;  // freeglut destroys the window itself after this callback
    closing_ = true;
}

void Window::beginFrame() {
    for (bool& p : pressed_) p = false;
    if (id_) glutMainLoopEvent();  // runs the keyboard / reshape / close callbacks

    time_ = nowSeconds();
    dt_ = static_cast<float>(time_ - last_);
    last_ = time_;
    if (dt_ > 0.1f) dt_ = 0.1f;  // bound catch-up work after a long stall
}

void Window::endFrame() {
    if (id_) glutSwapBuffers();
}

bool Window::keyDown(int key) const {
    return key >= 0 && key < KeyCount && (down_[key] || pressed_[key]);
}

bool Window::keyPressed(int key) const {
    return key >= 0 && key < KeyCount && pressed_[key];
}

float Window::aspect() const {
    return height_ > 0 ? static_cast<float>(width_) / static_cast<float>(height_) : 1.0f;
}

void Window::setTitle(const std::string& title) {
    if (id_) glutSetWindowTitle(title.c_str());
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
