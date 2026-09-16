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
    float time_ = 0.0f, dt_ = 0.0f, last_ = 0.0f;
    bool curr_[KEY_COUNT] = {};
    bool prev_[KEY_COUNT] = {};
};

// Grabs the front-left framebuffer into a binary PPM (handy for the report).
bool saveScreenshot(const std::string& path, int width, int height);

}  // namespace engine

#endif
