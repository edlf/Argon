// Argon: .cms animations (cms_animation.hpp)
#include "model/cms_animation.hpp"

#include "model/cmm_tracks.hpp"
#include "original/cmm_records.hpp"
#include "original/record_read.hpp"

namespace argon
{

std::optional<size_t> readCms(const std::vector<uint8_t> &data, size_t at, CmsAnimation &out)
{
    const auto header = original::readAt<original::CmsHeader>(data, at);
    if (at + sizeof(original::CmsHeader) > data.size())
    {
        return std::nullopt;
    }
    out.seconds = header.seconds;
    return readAnimationTracks(data, AnimationBlock{at + sizeof(original::CmsHeader), header.trackCount}, out.tracks);
}

std::vector<const CmmTrack *> tracksByBone(const Cmm &model, const CmsAnimation &animation)
{
    std::vector<const CmmTrack *> bound(model.bones.size(), nullptr);
    for (size_t b = 0; b < model.bones.size(); ++b)
    {
        for (const CmmTrack &track : animation.tracks)
        {
            if (track.name == model.bones[b].name)
            {
                bound[b] = &track;
            }
        }
    }
    return bound;
}

} // namespace argon
