// Argon: what a front-end page shows in 3D (FePage::scene): the models a page puts into the shell's render
// lists (+0x6958 seen by camera 1, +0x690c by camera 2), where, and how they are lit. App::recordFrame draws it.
#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct Track;

struct FeScene
{
    // the shell's cameras (fe_shell_ctor 0x2f4b0, fov 0x2f9a9 / 0x2f9c0), both 4:3: camera 1 (+0x60c0) the menu
    // items' and the globe's, camera 2 (+0x64c0) the characters' and ships'
    enum class Camera
    {
        Menu,
        Characters
    };

    Camera camera = Camera::Menu;
    Track *models[2]{};                 // drawn in this order; none: the page shows no model
    float clocks[2] = {-1.f, -1.f};     // each model's animation time (s); below 0: its stored pose
    glm::mat4 placement{1.f};
    glm::mat4 offsets[2]{glm::mat4(1.f), glm::mat4(1.f)};   // each model's own, after the placement (FE AWARDS: two)
    // one bone of the first model turned in its own space (0xb99f0: the speedometer's SPEEDO_NEEDLE)
    const char *turnedBone = nullptr;
    glm::mat4 boneTurn{1.f};
    // the game's near plane is 0.001 (far 5000); with our depth buffer that leaves ~0.4 units of depth resolution at
    // the character (80 away): fingers and hair cards z-fight. Pages with nothing nearer than ~7 use 1
    float zNear = 1.f;
    glm::vec3 lightDir{-0.5f, -1.f, -1.f};   // light 0 0xcccccc (0x2ffa0 / 0xa7f60); FE COURSE SELECT turns it
    bool lightsOff = false;                  // the locked character (0x15db0 -> 0xaf3d0): drawn black
    // the model fade (shell draw 0x30303 -> 0xc2810): the feedback texture over the model at 1 - clamp(now - this, 0,
    // 1), so a new character or ship comes out of the background over a second; below 0: no fade
    double fadeInFrom = -1.0;

    // a camera of its own in a part of the screen (multiplayer FE CHARSELECT's boxes, 0x178a0): the viewport in
    // pixels, the eye at target + eyeOffset; the app aims target at the first model's skel_head bone once posed
    struct Box
    {
        float x = 0, y = 0, w = 0, h = 0;
        glm::vec3 eyeOffset{0.f}, target{0.f};
        float fovY = 0.f;
    };
    bool boxed = false;
    Box box;

    bool empty() const
    {
        return models[0] == nullptr;
    }

    float fovY() const
    {
        if (boxed)
        {
            return box.fovY;
        }
        return camera == Camera::Menu ? 3.14159265f * 0.2857143f : 20.f * 3.14159265f / 180.f;
    }

    // camera 1: eye (0, 0.01, 0) (sway speed 0, 0x316e0) looking at (0, 0, 1); camera 2: eye (-0.79, 4.68, 7.57)
    // (0xae640) looking at (-0.8, 4.79, 6.58)
    glm::mat4 view() const
    {
        if (boxed)
        {
            return glm::lookAt(box.target + box.eyeOffset, box.target, glm::vec3(0, 1, 0));
        }
        if (camera == Camera::Menu)
        {
            return glm::lookAt(glm::vec3(0, 0.01f, 0), glm::vec3(0, 0, 1), glm::vec3(0, 1, 0));
        }
        return glm::lookAt(glm::vec3(-0.79f, 4.68f, 7.57f), glm::vec3(-0.8f, 4.79f, 6.58f), glm::vec3(0, 1, 0));
    }
};
