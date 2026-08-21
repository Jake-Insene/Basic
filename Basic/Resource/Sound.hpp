#pragma once
#include <Audio/Audio.hpp>
#include <Collections/Error.hpp>

#include "Basic/Resource/Resource.hpp"


namespace Basic
{

struct Sound : Resource
{
    RESOURCE(RESOURCE_SOUND, ResourceFlags::LoadFromAssets, ResourceExtensions("wav"))

    struct InternalData
    {
        // Only use mono or stereo.
        bool mono;
        Slice<i16> samples;
    } data;

	Sound(const ResourceCreateInfo& info);
    virtual ~Sound() override;

    Error load(Collections::StringView path);

    bool is_mono() const { return data.mono; }
    bool is_stereo() const { return !is_mono(); }

    Audio::Frame get_frame(usize index) const;
    usize get_frame_count() const { return is_mono() ? data.samples.len : data.samples.len / 2; }
};

}
