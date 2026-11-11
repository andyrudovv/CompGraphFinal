#ifndef ENGINE_WINDOW_H
#define ENGINE_WINDOW_H

#include <string>

struct GLFWwindow;

namespace engine {

// Owns the GLFW/GLEW context, the frame clock and the keyboard state.
class Window {
public:
    Window() = default;
    ~Window();

    bool create(int width, int height, const std::string& title);
    bool shouldClose() const;
    void close();

    void beginFrame();  // poll events, advance the clock, latch key edges
    void endFrame();    // swap buffers

    bool keyDown(int key) const;     // held this frame
    bool keyPressed(int key) const;  // went down on this frame only

    // Mouse state for the camera: held buttons (GLFW_MOUSE_BUTTON_*), cursor movement in
    // pixels since the previous frame and the wheel movement of this frame.
    bool mouseDown(int button) const;
    float mouseDX() const { return mouseDX_; }
    float mouseDY() const { return mouseDY_; }
    float scrollY() const { return scrollY_; }
    void addScroll(double dy) { scrollPending_ += dy; }  // called from the GLFW callback

    float dt() const { return dt_; }
    float time() const { return time_; }
    int width() const { return width_; }
    int height() const { return height_; }
    float aspect() const;
    void setTitle(const std::string& title);

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

// Grabs the back framebuffer into a binary PPM (handy for the report).
bool saveScreenshot(const std::string& path, int width, int height);

}  // namespace engine

#endif
