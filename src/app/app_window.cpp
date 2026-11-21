// Argon: App's window and its UI at start-up (launch\startup.cpp opens GLFW and decides the order): the window at
// the size asked for or settings.json's, fullscreen as settings.json says, ImGui on the renderer, the mouse wheel.
#include "app/app.hpp"

#include <algorithm>
#include <stdexcept>

namespace
{
// in the editor the wheel changes the free camera's speed; ImGui gets it first
void scrollCallback(GLFWwindow *window, double dx, double dy)
{
    ImGui_ImplGlfw_ScrollCallback(window, dx, dy);
    static_cast<App *>(glfwGetWindowUserPointer(window))->scrollFreeCameraSpeed(dy);
}
}  // namespace

void App::loadViewerSettings()
{
    viewerSettings = ViewerSettings::load(fs::path(saves) / "settings.json");
    menu::widescreenLayout = viewerSettings.widescreen();
}

// settings.json's resolution when the monitor offers it; else (none stored, or not offered) its largest, written back
argon::Resolution App::validStoredResolution()
{
    const std::optional<argon::Resolution> stored = viewerSettings.resolution();
    const argon::Resolution valid = argon::validResolution(stored, windowMode.offeredResolutions());
    if (valid != stored)
    {
        if (stored)
        {
            std::fprintf(stderr, "settings: the monitor has no %s mode: %s\n", stored->text().c_str(), valid.text().c_str());
        }
        viewerSettings.setResolution(valid);
        viewerSettings.save(fs::path(saves) / "settings.json");
    }
    return valid;
}

// settings.json's fullscreen; never for --size or the screenshot runs (their pictures keep the window's size)
void App::openWindow(const std::optional<argon::Resolution> &requestedSize)
{
    windowMode.setOfferedResolutions(argon::monitorResolutions());
    const argon::Resolution size = requestedSize.value_or(validStoredResolution());
    window = glfwCreateWindow(size.width, size.height, "Argon", nullptr, nullptr);
    input.attach(window, screenshotRun(), tests.seats);
    if (!window)
    {
        throw std::runtime_error("window creation failed");
    }
    windowMode.setResolution(window, size);
    windowMode.setFullscreen(window, viewerSettings.fullscreen() && !requestedSize && !screenshotRun());
    glfwSetWindowUserPointer(window, this);
}

void App::startUi()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr;
    ImGui::StyleColorsDark();
    renderer.startUi();
    glfwSetScrollCallback(window, scrollCallback);
    findTranslatedLanguages();
    loadHud();
}

void App::scrollFreeCameraSpeed(double wheel)
{
    if (editor.active() && !ImGui::GetIO().WantCaptureMouse)
    {
        freeCamera.speed = std::clamp(freeCamera.speed * (wheel > 0 ? 1.25f : 0.8f), 2.f, 1000.f);
    }
}
