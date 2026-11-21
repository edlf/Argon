// Argon: what the 3D view draws and how, as the editor panel (ui_extras\editor_panel.cpp) and the launch options switch
// it: the mesh groups, the editor's markings, the effects, the textures, lighting, fog and the game's material states.
// The game itself draws everything with its own states; these are the viewer's.
#pragma once

// a track model's meshes by what they are (app_track.cpp sorts them)
enum Group
{
    G_COLLISION,
    G_INVISIBLE,
    G_VISUAL,
    G_ANIMATED,
    G_SKY,
    G_COUNT
};
inline const char *kGroupNames[G_COUNT] = {"Collision (road/walls)", "Invisible walls", "Visual only", "Bone pieces (animated/fx)", "Sky"};

struct SceneToggles
{
    bool groups[G_COUNT] = {true, false, true, true, true};

    // the editor's markings (render\editor_markings.cpp)
    bool starts = true, aiRoute = true, pickups = true, emitters = false, issues = true;
    bool markersOnTop = false;

    // the models and effects
    bool ships = true, pickupModels = true, trackSprites = true, water = true;
    bool shadows = true;
    bool solidShadows = true;   // filled silhouette like the game; off = textured ship image
    bool trails = true;
    int trailMode = 0;          // stage combine (the game inherits it, see trail.frag)
    float trailGain = 1.f;
    bool drops = true, spray = true;
    bool sprayVertexAlpha = false;   // sprite pass alpha from the vertex only (the game inherits the combine)

    // the meshes' shading
    bool textures = true, lighting = true, fog = true, cull = false, wireframe = false;
    bool bakedLighting = true;   // off draws the vertices' prelit colours white
    int alphaMode = 1;           // fallback when gameMaterials is off: 0 off, 1 auto, 2 all textures with alpha
    bool gameMaterials = true;   // use decoded per-mesh blend / alpha test / depth / cull state
    bool gameCull = true;        // cull back faces unless the mesh is two-sided
    bool frontCounterClockwise = false;   // game front faces are clockwise in its (right-handed) data (--cw / --ccw)
};
