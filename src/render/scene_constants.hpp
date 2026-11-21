// Argon: the scene constants (set 1) a view is drawn with: the camera, the clock, the toggles, the track's lights
// and fog, or the front end's (render\scene_buffers.hpp holds their buffers). Pure rules.
#pragma once
#include "movie/scene_light.hpp"
#include "render/scene_toggles.hpp"
#include "track/trackdata.hpp"

#include <glm/glm.hpp>

#include <optional>

struct SceneUbo
{
    glm::vec4 ambient, sunColour, sunDir, fogColour, fogParams;
    glm::mat4 view;
    glm::vec4 camTime;      // xyz camera position, w time in seconds
    glm::ivec4 toggles;     // x fog, y lighting, z game materials, w fallback alpha mode
    glm::ivec4 toggles2;    // x textures, y light prelit meshes (front end), z the vertices' baked colours
    glm::vec4 light1Colour, light1Dir;   // D3D light 1: a story scene's own light (black otherwise)
};

// prop.dat's sun, unit length; none (zero): (-0.5, -1, 1)
glm::vec3 sunDirection(const argon::TrackProps &props);

// what a race view is drawn at: its camera, the eye, the clock; light1: a story scene's light (0x9a810 turns D3D
// light 1 on with the scene's direction and colour, 0x96308 -> 0xa7ea0)
struct SceneMoment
{
    glm::mat4 view{1.f};
    glm::vec3 eye{0.f};
    float seconds = 0.f;
    std::optional<argon::story::SceneLight> light1;
};

SceneUbo raceSceneConstants(const argon::TrackProps &props, const SceneToggles &toggles, const SceneMoment &moment);

// Scene lights (0x2ffa0 / 0xa7f60 / 0xa9be0): light 0 0xcccccc along the scene's direction, light 1 off, ambient
// 0x707070 (left by 0xa83c0 the frame before), black for a locked character; no fog. seconds: the shell's 60 Hz clock
struct FrontEndLights
{
    glm::mat4 view{1.f};
    glm::vec3 lightDir{0.f, -1.f, 0.f};
    bool lightsOff = false;
    float seconds = 0.f;
};

SceneUbo frontEndSceneConstants(const FrontEndLights &lights, const SceneToggles &toggles);
