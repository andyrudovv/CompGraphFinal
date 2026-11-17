// GLFW window and input. Creates the OpenGL 3.3 core context, keeps the frame clock and stores
// which keys and mouse buttons are held or were just pressed.

#ifndef ENGINE_WINDOW_H
#define ENGINE_WINDOW_H

#include <string>

struct GLFWwindow;

namespace engine {

class Window {
public:
    Window() = default;
    ~Window();

    bool create(int width, int height, const std::string& title);
    bool shouldClose() const;
    void close();

    void beginFrame();
    void endFrame();

    bool keyDown(int key) const;
    bool keyPressed(int key) const;

    bool mouseDown(int button) const;
    float mouseDX() const { return mouseDX_; }
    float mouseDY() const { return mouseDY_; }
    float scrollY() const { return scrollY_; }
    void addScroll(double dy) { scrollPending_ += dy; }

    float dt() const { return dt_; }
    float time() const { return time_; }
    int width() const { return width_; }
    int height() const { return height_; }
    float aspect() const;
    void setTitle(const std::string& title);
    void setVsync(bool on);

private:
    static const int KEY_COUNT = 512;

    GLFWwindow* handle_ = nullptr;
    int width_ = 0, height_ = 0;
    double time_ = 0.0, last_ = 0.0;
    float dt_ = 0.0f;
    bool curr_[KEY_COUNT] = {};
    bool prev_[KEY_COUNT] = {};

    static const int MOUSE_BUTTONS = 8;
    bool mouse_[MOUSE_BUTTONS] = {};
    double cursorX_ = 0.0, cursorY_ = 0.0;
    bool haveCursor_ = false;
    float mouseDX_ = 0.0f, mouseDY_ = 0.0f;
    double scrollPending_ = 0.0;
    float scrollY_ = 0.0f;
};

bool saveScreenshot(const std::string& path, int width, int height);

}

#endif
