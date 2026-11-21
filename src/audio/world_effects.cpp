// Argon: the listener filter, the crosstalk canceller and the speaker mixbins on the game's sounds, see
// world_effects.hpp.
#include "audio/world_effects.hpp"

#include <algorithm>

#include "audio/audio_output.hpp"
#include "miniaudio.h"

namespace argon
{
namespace
{
constexpr ma_uint32 kStereo = 2;
enum Input : ma_uint32
{
    kFiltered3d,
    kDirect3d,
    kFiltered2d,
    kDirect2d,
    kFront,
    kInputCount
};

void addInto(const float *from, float *to, size_t samples)
{
    for (size_t i = 0; i < samples; ++i)
    {
        to[i] += from[i];
    }
}
}  // namespace

struct WorldEffects::Graph
{
    struct Node
    {
        ma_node_base base;   // first: miniaudio hands the callback this address
        WorldEffects *owner = nullptr;
    } node{};
    ma_sound_group groups[kInputCount]{};
    bool nodeReady = false, groupReady[kInputCount] = {};

    ~Graph()
    {
        for (ma_uint32 i = 0; i < kInputCount; ++i)
        {
            if (groupReady[i])
            {
                ma_sound_group_uninit(&groups[i]);
            }
        }
        if (nodeReady)
        {
            ma_node_uninit(&node.base, nullptr);
        }
    }
};

WorldEffects::WorldEffects() = default;
WorldEffects::~WorldEffects() = default;

bool WorldEffects::init(ma_engine *engine, mix::SpeakerLayout layout)
{
    layout_ = layout;
    channels_ = size_t(mix::channelCount(layout));
    const ma_uint32 output = ma_uint32(channels_), mixbins = ma_uint32(mix::kSpeakerCount);
    static ma_node_vtable vtable = {&WorldEffects::processNode, nullptr, kInputCount, 1, 0};
    const ma_uint32 inputChannels[kInputCount] = {output, output, mixbins, mixbins, kStereo}, outputChannels[1] = {output};
    auto graph = std::make_unique<Graph>();
    graph->node.owner = this;
    ma_node_config config = ma_node_config_init();
    config.vtable = &vtable;
    config.pInputChannels = inputChannels;
    config.pOutputChannels = outputChannels;
    graph->nodeReady = ma_node_init(ma_engine_get_node_graph(engine), &config, nullptr, &graph->node.base) == MA_SUCCESS;
    if (!graph->nodeReady)
    {
        return false;
    }
    ma_node_attach_output_bus(&graph->node.base, 0, ma_engine_get_endpoint(engine), 0);
    for (ma_uint32 i = 0; i < kInputCount; ++i)
    {
        ma_sound_group_config group = ma_sound_group_config_init_2(engine);
        group.channelsIn = group.channelsOut = inputChannels[i];
        group.flags = MA_SOUND_FLAG_NO_SPATIALIZATION | MA_SOUND_FLAG_NO_DEFAULT_ATTACHMENT;
        graph->groupReady[i] = ma_sound_group_init_ex(engine, &group, &graph->groups[i]) == MA_SUCCESS;
        if (!graph->groupReady[i])
        {
            return false;
        }
        ma_node_attach_output_bus(&graph->groups[i], 0, &graph->node.base, i);
    }
    sampleRate_ = int(ma_engine_get_sample_rate(engine));
    frontCrosstalk_.configure(sampleRate_);
    rearCrosstalk_.configure(sampleRate_);
    graph_ = std::move(graph);
    return true;
}

ma_sound *WorldEffects::group(bool filtered, VoicePath path)
{
    if (!graph_)
    {
        return nullptr;
    }
    Input input = kFront;
    if (path == VoicePath::Hrtf)
    {
        input = filtered ? kFiltered3d : kDirect3d;
    }
    else if (path == VoicePath::SpeakerFiles)
    {
        input = filtered ? kFiltered2d : kDirect2d;
    }
    return &graph_->groups[input];
}

void WorldEffects::setFilters(const ListenerFilters &filters)
{
    if (filtersReady_.load(std::memory_order_acquire))
    {
        return;
    }
    filters_ = filters;
    filtersReady_.store(true, std::memory_order_release);
}

void WorldEffects::select(Filter filter)
{
    wantedFilter_.store(int(filter), std::memory_order_relaxed);
}

// on the audio thread: a new filter reconfigures both low-passes
void WorldEffects::applyChanges()
{
    if (!filtersReady_.load(std::memory_order_acquire))
    {
        return;
    }
    const int filter = wantedFilter_.load(std::memory_order_relaxed);
    if (filter == filter_)
    {
        return;
    }
    filter_ = filter;
    const LowPassSetting none;
    const LowPassSetting &setting = filter == int(Filter::Internal) ? filters_.internal : filter == int(Filter::Underwater) ? filters_.underwater : none;
    filter3d_.configure(setting, sampleRate_);
    filter2d_.configure(setting, sampleRate_);
}

// out = crosstalk(filter(filtered 3D) + direct 3D) + fold(filter(filtered 2D) + direct 2D) + front pair
void WorldEffects::mix(const float *const *in, float *out, size_t frames)
{
    applyChanges();
    std::fill(out, out + frames * channels_, 0.f);
    addVoices3d(in[kFiltered3d], in[kDirect3d], out, frames);
    addSpeakerMixbins(in[kFiltered2d], in[kDirect2d], out, frames);
    addFrontPair(in[kFront], out, frames);
}

// the 3D voices into the crosstalk's mixbins: the front pair, in 5.1 the rear pair too (one canceller, 4 in, 4 out)
void WorldEffects::addVoices3d(const float *filtered, const float *direct, float *out, size_t frames)
{
    const size_t samples = frames * channels_;
    voices_.assign(filtered, filtered + samples);
    filter3d_.process(voices_.data(), frames, channels_);
    addInto(direct, voices_.data(), samples);
    frontCrosstalk_.process(voices_.data() + mix::kFrontLeft, frames, channels_);
    if (layout_ == mix::SpeakerLayout::Surround)
    {
        rearCrosstalk_.process(voices_.data() + mix::kBackLeft, frames, channels_);
    }
    addInto(voices_.data(), out, samples);
}

void WorldEffects::addSpeakerMixbins(const float *filtered, const float *direct, float *out, size_t frames)
{
    const size_t samples = frames * mix::kSpeakerCount;
    mixbins_.assign(filtered, filtered + samples);
    filter2d_.process(mixbins_.data(), frames, mix::kSpeakerCount);
    addInto(direct, mixbins_.data(), samples);
    folded_.resize(frames * channels_);
    mix::foldSpeakers(mixbins_.data(), folded_.data(), frames, layout_);
    addInto(folded_.data(), out, frames * channels_);
}

void WorldEffects::addFrontPair(const float *front, float *out, size_t frames) const
{
    for (size_t frame = 0; frame < frames; ++frame)
    {
        out[frame * channels_ + mix::kFrontLeft] += front[frame * kStereo];
        out[frame * channels_ + mix::kFrontRight] += front[frame * kStereo + 1];
    }
}

void WorldEffects::processNode(void *node, const float **in, unsigned *inFrames, float **out, unsigned *outFrames)
{
    auto *self = static_cast<Graph::Node *>(node)->owner;
    const size_t frames = std::min(size_t(*outFrames), size_t(*inFrames));   // one count for every input bus
    self->mix(in, out[0], frames);
    *inFrames = ma_uint32(frames);
    *outFrames = ma_uint32(frames);
}

}  // namespace argon
