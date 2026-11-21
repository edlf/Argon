// Argon: the order-independent translucency (render\order_independent_translucency.hpp).
#include "render/order_independent_translucency.hpp"

using argon::gfx::DepthCompare;
using argon::gfx::RenderTarget;
using argon::gfx::ShaderProgram;

namespace
{
const argon::MeshState kCompositeBlend = argon::MeshState::alphaBlended();
const argon::MeshState kColourKept = argon::MeshState::blended(argon::gfx::BlendFactor::Zero, argon::gfx::BlendFactor::One);
}  // namespace

OrderIndependentTranslucency::OrderIndependentTranslucency(argon::gfx::Device &device, argon::gfx::Presenter &presenter,
                                                           std::span<const argon::gfx::SceneSet> sceneSets)
    : device_(device), presenter_(presenter), sceneSets_(sceneSets)
{
    mesh_ = device_.createPipeline({.program = ShaderProgram::MeshOit, .target = RenderTarget::Oit});
    solid_ = device_.createPipeline({.program = ShaderProgram::MeshOitOpaque});
    depth_ = device_.createPipeline({.program = ShaderProgram::Mesh, .blend = &kColourKept});   // mesh.frag, every pixel
    composite_ = device_.createPipeline({.program = ShaderProgram::OitComposite, .blend = &kCompositeBlend, .depth = DepthCompare::Always});
}

void OrderIndependentTranslucency::prepare(bool on)
{
    presenter_.keepOitTargets(on);
    on_ = on;
    if (!on || presenter_.oitTargetsMade() == boundMade_)
    {
        return;
    }
    device_.waitForIdle();
    const argon::gfx::OitTargets targets = presenter_.oitTargets();
    for (const argon::gfx::SceneSet set : sceneSets_)
    {
        device_.useOitTargets(set, targets);
    }
    boundMade_ = presenter_.oitTargetsMade();
}

void OrderIndependentTranslucency::begin()
{
    presenter_.beginOitPass();
}

// one triangle over the target, clipped to the view's viewport; pixels nothing translucent covered are discarded
void OrderIndependentTranslucency::composite(argon::gfx::CommandList &commands)
{
    presenter_.endOitPass();
    commands.setDepthCull({.test = false, .write = false, .cullBack = false});
    commands.bindPipeline(composite_);
    commands.draw({.count = 3, .first = 0});
}
