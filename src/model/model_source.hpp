// Argon: a model folder (model.cmm + tex%u.dds) read and decoded from disk. Pure CPU work with no
// Vulkan, so it runs on any thread; ModelBuilder::build (render\model_builder.hpp) uploads it.
#pragma once
#include "gfx/gfx_types.hpp"
#include <glm/glm.hpp>
#include <string>
#include <vector>

#include "model/cmm.hpp"
#include "track/collision_surface.hpp"
#include "files/dds.hpp"

// one cmm mesh's part of the model's vertex / index arrays, and its bounds
struct MeshRange
{
    uint32_t firstIndex = 0, indexCount = 0;   // indexCount 0: the mesh has no triangles
    int32_t vertexOffset = 0;
    glm::vec3 placedMin{1e30f}, placedMax{-1e30f};   // on its bone (unless skinned)
    glm::vec3 localMin{1e30f}, localMax{-1e30f};     // as stored
};

struct ModelSource
{
    argon::Cmm cmm;
    bool loaded = false;                      // model.cmm parsed
    std::vector<argon::TextureData> textures;    // per cmm texture; no levels = missing (drawn white)
    std::vector<MeshRange> meshes;            // per cmm mesh
    std::vector<argon::Vertex> verts;            // every mesh's vertices, back to back
    std::vector<uint32_t> indices;            // triangle lists, relative to their mesh's vertexOffset
    std::vector<glm::vec3> collTris;          // the collidable meshes in world space (what the game collides with)
    std::vector<argon::TriangleSurface> collSurf;   // per collision triangle
    std::vector<std::string> warnings;
};

ModelSource readModel(const std::string &folder);

// how ModelBuilder::build draws a model: in the sky pass, from which descriptor pool, as a prop (ship / pickup: weapon
// bones, no collision)
struct ModelUse
{
    bool isSky = false;
    argon::gfx::SetPool pool;
    bool prop = false;
    bool craft = false;   // a craft: its shadow hull (surface flag 0x400000) only casts the ship shadow
    // re-skinned every frame (ModelBuilder::animate: the front end's models, the pickups, a story scene's actors): its
    // vertices stay in host-visible buffers it rewrites. Without it an animated model's vertices go to GPU memory and
    // only its bones are posed (the ships' weapon bones, the explosions)
    bool reskinned = false;
    // a prop's weapon and shield parts (WeaponBones) are hidden; an object without any keeps the meshes on bones that
    // only share their names (the FE AWARDS trophies' revol2..4: the cup, the stand and the backdrop discs)
    bool weaponParts = true;
};
