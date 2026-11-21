// Argon: the window's size and fullscreen, see window_mode.hpp.
#include "window/window_mode.hpp"

#include <GLFW/glfw3.h>

namespace argon
{
namespace
{
ScreenRect windowRect(GLFWwindow *window)
{
    ScreenRect rect;
    glfwGetWindowPos(window, &rect.x, &rect.y);
    glfwGetWindowSize(window, &rect.width, &rect.height);
    return rect;
}

ScreenRect monitorRect(GLFWmonitor *monitor)
{
    ScreenRect rect;
    glfwGetMonitorPos(monitor, &rect.x, &rect.y);
    const GLFWvidmode *mode = glfwGetVideoMode(monitor);
    rect.width = mode ? mode->width : 0;
    rect.height = mode ? mode->height : 0;
    return rect;
}

// the monitor the window is mostly on (the primary when there is no list)
GLFWmonitor *monitorUnder(const ScreenRect &window)
{
    int count = 0;
    GLFWmonitor **monitors = glfwGetMonitors(&count);
    std::vector<ScreenRect> rects;
    for (int i = 0; i < count; ++i)
    {
        rects.push_back(monitorRect(monitors[i]));
    }
    return rects.empty() ? glfwGetPrimaryMonitor() : monitors[monitorUnderWindow(window, rects)];
}

// the whole of that monitor (its current video mode, not only the work area: the taskbar too)
ScreenRect screenUnder(const ScreenRect &window)
{
    GLFWmonitor *monitor = monitorUnder(window);
    return monitor ? monitorRect(monitor) : window;
}

void place(GLFWwindow *window, const ScreenRect &rect)
{
    glfwSetWindowPos(window, rect.x, rect.y);
    glfwSetWindowSize(window, rect.width, rect.height);
}

bool narrowerFirst(const Resolution &first, const Resolution &second)
{
    return first.width != second.width ? first.width < second.width : first.height < second.height;
}

ScreenRect centredOn(const ScreenRect &screen, const Resolution &size)
{
    return {screen.x + (screen.width - size.width) / 2, screen.y + (screen.height - size.height) / 2, size.width, size.height};
}
}  // namespace

std::vector<Resolution> monitorResolutions()
{
    std::vector<Resolution> sizes;
    GLFWmonitor *monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode *desktop = monitor ? glfwGetVideoMode(monitor) : nullptr;
    if (!desktop)
    {
        return sizes;
    }
    const Resolution desktopSize{desktop->width, desktop->height};
    int count = 0;
    const GLFWvidmode *modes = glfwGetVideoModes(monitor, &count);
    for (int i = 0; i < count; ++i)
    {
        const Resolution size{modes[i].width, modes[i].height};
        if (size.fitsInside(desktopSize) && std::find(sizes.begin(), sizes.end(), size) == sizes.end())
        {
            sizes.push_back(size);
        }
    }
    std::sort(sizes.begin(), sizes.end(), narrowerFirst);
    return sizes;
}

void WindowMode::setResolution(GLFWwindow *window, const Resolution &size)
{
    size_ = size;
    if (fullscreen_)
    {
        glfwSetWindowMonitor(window, glfwGetWindowMonitor(window), 0, 0, size.width, size.height, GLFW_DONT_CARE);
        return;
    }
    place(window, centredOn(screenUnder(windowRect(window)), size));
}

void WindowMode::setFullscreen(GLFWwindow *window, bool on)
{
    if (on == fullscreen_)
    {
        return;
    }
    if (on)
    {
        enterFullscreen(window);
    }
    else
    {
        leaveFullscreen(window);
    }
}

// the monitor the window is mostly on, switched to the mode of that size with its highest refresh rate
void WindowMode::enterFullscreen(GLFWwindow *window)
{
    windowed_ = windowRect(window);
    glfwSetWindowMonitor(window, monitorUnder(windowed_), 0, 0, size_.width, size_.height, GLFW_DONT_CARE);
    fullscreen_ = true;
}

// back where it was; centred again when Resolution changed meanwhile
void WindowMode::leaveFullscreen(GLFWwindow *window)
{
    glfwSetWindowMonitor(window, nullptr, windowed_.x, windowed_.y, size_.width, size_.height, 0);
    fullscreen_ = false;
    if (size_.width != windowed_.width || size_.height != windowed_.height)
    {
        place(window, centredOn(screenUnder(windowRect(window)), size_));
    }
}

}  // namespace argon
