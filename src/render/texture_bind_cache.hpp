// Argon: the texture-bind cache of d3d_set_texture (0xcbb40; docs\models\materials.md "Texture table"). The game remembers the
// texture slot bound on each of the four stages (table 0x50e374) and skips the SetTexture when the slot asked for is
// already there (0xcbbd2). An animated slot's frame, round(clock x the stage's rate) mod frames, is computed before
// the check but only used by a bind, so a run of meshes binding one animated slot on one stage all show the frame
// the first of them picked, with its rate. A stage is emptied (0xa9af0) when a mesh has fewer stages than it, before
// every item of the opaque render list (stage 0 only, 0xbbc99) and by the scene setup (all four, 0xa9be0 -> 0xa8750).
#pragma once
#include <array>
#include <optional>

namespace argon
{
constexpr int kTextureStages = 4;

// one model's texture slot (instance +0x210 + index x 0x28): what 0xcbbd2 compares
struct TextureSlot
{
    const void *model = nullptr;
    int texture = -1;

    bool operator==(const TextureSlot &other) const = default;
};

// a stage's bind: the slot and the rate its frame was picked with
struct StageBind
{
    TextureSlot slot;
    float rate = 0.f;
};

// what one mesh binds (0xb7290..0xb736b): its texture list (mesh +0x34 count, +0x38 indices) and each stage's rate,
// copied to the slot from the material just before (0xba716)
struct MeshTextures
{
    int count = 0;
    std::array<int, kTextureStages> texture{};
    std::array<float, kTextureStages> rate{};
};

class TextureBindCache
{
  public:
    // cmm_draw_mesh's binds: stages below the mesh's count bind its slots, the others are emptied
    void bindMesh(const void *model, const MeshTextures &mesh);
    void bind(int stage, const StageBind &request);
    void empty(int stage);
    void emptyAll();
    std::optional<StageBind> bound(int stage) const;

  private:
    std::array<std::optional<StageBind>, kTextureStages> bound_;
};
} // namespace argon
