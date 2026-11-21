// Argon: the scene constants (scene_constants.hpp).
#include "render/scene_constants.hpp"


namespace
{

const float kDefaultFogEnd = 3000.f;
const float kFrontEndFogEnd = 30000.f;
const float kFrontEndAmbient = 0x70 / 255.f;
const float kFrontEndLight0 = 0xcc / 255.f;

glm::vec4 colour(const std::array<float, 3> &rgb)
{
    return glm::vec4(rgb[0], rgb[1], rgb[2], 1.f);
}

}  // namespace

glm::vec3 sunDirection(const argon::TrackProps &props)
{
    glm::vec3 sun(props.sunVector[0], props.sunVector[1], props.sunVector[2]);
    if (glm::length(sun) < 1e-4f)
    {
        sun = {-0.5f, -1, 1};
    }
    return glm::normalize(sun);
}

SceneUbo raceSceneConstants(const argon::TrackProps &props, const SceneToggles &toggles, const SceneMoment &moment)
{
    SceneUbo constants{};
    constants.view = moment.view;
    constants.camTime = glm::vec4(moment.eye, moment.seconds);
    constants.toggles = glm::ivec4(toggles.fog, toggles.lighting, toggles.gameMaterials, toggles.alphaMode);
    constants.toggles2 = glm::ivec4(toggles.textures, 0, toggles.bakedLighting, 0);
    constants.ambient = colour(props.ambient);
    constants.sunColour = colour(props.sunColour);
    constants.sunDir = glm::vec4(sunDirection(props), 0);
    constants.fogColour = colour(props.fogColour);
    const float end = props.fogRange > 1.f ? props.fogRange : kDefaultFogEnd;
    constants.fogParams = glm::vec4(float(props.fogMode ? props.fogMode : 1), props.fogDensity, end, 0);
    if (moment.light1)
    {
        constants.light1Colour = glm::vec4(moment.light1->colour, 1);
        constants.light1Dir = glm::vec4(moment.light1->direction, 0);
    }
    return constants;
}

SceneUbo frontEndSceneConstants(const FrontEndLights &lights, const SceneToggles &toggles)
{
    SceneUbo constants{};
    constants.view = lights.view;
    constants.camTime = glm::vec4(glm::vec3(0, 0.01f, 0), lights.seconds);
    constants.toggles = glm::ivec4(0, 1, toggles.gameMaterials, toggles.alphaMode);
    constants.toggles2 = glm::ivec4(toggles.textures, 1, 1, 0);
    constants.ambient = glm::vec4(glm::vec3(lights.lightsOff ? 0.f : kFrontEndAmbient), 1);
    constants.sunColour = glm::vec4(glm::vec3(lights.lightsOff ? 0.f : kFrontEndLight0), 1);
    constants.sunDir = glm::vec4(glm::normalize(lights.lightDir), 0);
    constants.fogParams = glm::vec4(1, 0, kFrontEndFogEnd, 0);
    return constants;
}
