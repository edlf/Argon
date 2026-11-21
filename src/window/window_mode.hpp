// Argon: the window's size (Argon Options' Resolution) and fullscreen (Argon Options' Fullscreen, F11). Fullscreen
// switches the monitor the window is mostly on to the mode of that size (its highest refresh rate); out of it the
// window has that size, centred on its monitor, and goes back where it was.
#pragma once
#include "window/resolution.hpp"
#include "window/window_placement.hpp"

struct GLFWwindow;

namespace argon
{

class WindowMode
{
  public:
    // fullscreen: the monitor's mode; else the window, centred on the monitor it is mostly on
    void setResolution(GLFWwindow *window, const Resolution &size);
    void setFullscreen(GLFWwindow *window, bool on);
    // the sizes Argon Options' Resolution offers: the monitor's
    void setOfferedResolutions(std::vector<Resolution> sizes)
    {
        offered_ = std::move(sizes);
    }
    const std::vector<Resolution> &offeredResolutions() const
    {
        return offered_;
    }
    bool fullscreen() const
    {
        return fullscreen_;
    }

  private:
    void enterFullscreen(GLFWwindow *window);
    void leaveFullscreen(GLFWwindow *window);

    bool fullscreen_ = false;
    Resolution size_;
    std::vector<Resolution> offered_;
    ScreenRect windowed_;   // where the window was before fullscreen
};

// the primary monitor's video modes that fit on its desktop, narrowest first, each size once (glfwInit first)
std::vector<Resolution> monitorResolutions();

}  // namespace argon
