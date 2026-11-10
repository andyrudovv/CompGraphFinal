#ifndef ENGINE_WINDOW_H
#define ENGINE_WINDOW_H

#include <string>

namespace engine {

// Keys the game cares about. The window maps freeglut's keyboard events
// (ASCII keys and special keys) onto these values, so the game code does not
// depend on any windowing library.
enum Key {
    KeyEscape, KeyLeft, KeyRight, KeyUp, KeyDown, KeyF2,
    KeyW, KeyA, KeyS, KeyD, KeyP, KeyR, KeyB,
    Key1, Key2, Key3, Key4,
    KeyCount
};

// Owns the freeglut/GLEW context, the frame clock and the keyboard state.
// Input arrives through freeglut callbacks (keyboard, special, reshape, close);
// beginFrame() lets freeglut process pending events with glutMainLoopEvent().
class Window {
public:
    Window() = default;
    ~Window();

    bool create(int width, int height, const std::string& title);
    bool shouldClose() const { return closing_; }
    void close() { closing_ = true; }

    void beginFrame();  // process events (callbacks), advance the clock
    void endFrame();    // swap buffers

    bool keyDown(int key) const;     // held this frame
    bool keyPressed(int key) const;  // went down on this frame only

    float dt() const { return dt_; }
    float time() const { return static_cast<float>(time_); }
    int width() const { return width_; }
    int height() const { return height_; }
    float aspect() const;
    void setTitle(const std::string& title);

    // Called by the freeglut callbacks (see window.cpp).
    void onKey(int key, bool down);
    void onReshape(int width, int height);
    void onClose();

private:
    int id_ = 0;  // freeglut window id, 0 = no window
    bool closing_ = false;
    int width_ = 0, height_ = 0;
    double time_ = 0.0, last_ = 0.0;
    float dt_ = 0.0f;
    bool down_[KeyCount] = {};     // currently held
    bool pressed_[KeyCount] = {};  // went down since the previous beginFrame()
};

// Grabs the back framebuffer into a binary PPM (handy for the report).
bool saveScreenshot(const std::string& path, int width, int height);

}  // namespace engine

#endif
