#include "Basic/Resource/Sound.hpp"

#include <External/dr_wav.h>

#include "Basic/Resource/ResourceManager.hpp"
#include "Basic/Resource/ResourceManagerInternal.hpp"


namespace Basic
{

static inline void* dr_alloc(size_t size, [[maybe_unused]] void* user_data)
{
    ResourceManager* rm = reinterpret_cast<ResourceManager*>(user_data);
    Mem::Allocator& allocator = rm->get_allocator();
    return allocator.alloc(size, 16).items;
}

static inline void* dr_realloc(void* mem, size_t new_size, [[maybe_unused]] void* user_data)
{
    Slice old_mem = Slice(reinterpret_cast<u8*>(mem), 1);

    ResourceManager* rm = reinterpret_cast<ResourceManager*>(user_data);
    Mem::Allocator& allocator = rm->get_allocator();
    if(mem == nullptr)
    {
        return dr_alloc(new_size, user_data);
    }

    if (allocator.realloc(old_mem, new_size, 16))
    {
        return mem;
    }

    return allocator.remap(old_mem, new_size, 16).ptr();
}

static inline void dr_free(void* mem, [[maybe_unused]] void* user_data)
{
    ResourceManager* rm = reinterpret_cast<ResourceManager*>(user_data);
    Mem::Allocator& allocator = rm->get_allocator();
    allocator.free(
        Slice(reinterpret_cast<u8*>(mem), 1)
    );
}

static inline drwav_allocation_callbacks alloc_callbacks =
{
    .pUserData = nullptr,
    .onMalloc = &dr_alloc,
    .onRealloc = &dr_realloc,
    .onFree = &dr_free,
};

Sound::Sound(const ResourceCreateInfo& info)
: Resource(info)
{
    data.mono = false;
    data.samples = {};
}

Sound::~Sound()
{
    if(!data.samples.null())
    {
        allocator.free(Mem::to_bytes(data.samples));
    }
}

Error Sound::load(Collections::StringView path)
{
    if (!IO::File::exists(allocator, path))
    {
        RMDebugInfo("Couldn't load the font '{}'", path);
        return MakeError(ErrorCode::FileNotFound);
    }
    
    Resource::path.set(path);

    Slice content = IO::File::read_all(allocator, path);

    drwav wav = {};
    alloc_callbacks.pUserData = &resource_manager;
    drwav_init_memory(&wav, content.ptr(), content.len, &alloc_callbacks);

    if(wav.channels != 1 && wav.channels != 2)
    {
        RMDebugInfo("The WAV file({}) contains a not supported channel count, find({}) expected 1 or 2",
            path, wav.channels
        );
    }

    const usize total_frames = static_cast<usize>(wav.totalPCMFrameCount);
    data.mono = wav.channels == 1;

    Collections::Array<i16> loaded_samples{allocator, total_frames * Audio::OutputChannels, {}};
    loaded_samples.resize(total_frames * wav.channels);

    // Always convert
    (void)drwav_read_pcm_frames_s16(&wav, wav.totalPCMFrameCount,
        reinterpret_cast<drwav_int16*>(loaded_samples.slice().ptr()));

    // resampling
    const f32 ratio = f32(wav.sampleRate) / f32(Audio::output_get_samples_per_sec());
    const usize new_total_frames = static_cast<usize>(f32(total_frames) / f32(ratio)); // total_samples * SampleRate/Target
    data.samples = allocator.array<i16>(new_total_frames * wav.channels);

    for(usize out_frame = 0; out_frame < new_total_frames; out_frame++)
    {
        f32 normalized = f32(out_frame) / new_total_frames;
        usize source_frame = usize(normalized * total_frames);

        usize clamped_frame = Math::clamp(source_frame, 0ULL, total_frames);

        for(usize channel = 0; channel < wav.channels; channel++)
        {
            i16 value = loaded_samples.get(clamped_frame * wav.channels + channel);
            data.samples[out_frame * wav.channels + channel] = value;
        }
    }

    drwav_uninit(&wav);
    allocator.free(content);

    return ErrorCode::Ok;
}

Audio::Frame Sound::get_frame(usize index) const
{
    if(is_stereo())
    {
        return Audio::Frame(
            data.samples[(index * Audio::OutputChannels) + 0],
            data.samples[(index * Audio::OutputChannels) + 1]
        );
    }

    return Audio::Frame(
        data.samples[index],
        data.samples[index]
    );
}

}
