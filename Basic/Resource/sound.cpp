#include "Basic/Resource/sound.h"

#include <external/dr_wav.h>

#include "Basic/Resource/resource_manager.h"
#include "Basic/Resource/resource_manager_internal.h"


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
    if (allocator.realloc(old_mem, new_size, 16))
    {
        return mem;
    }

    Slice new_mem = allocator.alloc(new_size, 16);
    if (mem != nullptr)
    {
        allocator.free(old_mem);
    }

    return new_mem.items;
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
    drwav_init_memory(&wav, content.ptr(), content.len, &alloc_callbacks);

    if(wav.channels != 1 && wav.channels != 2)
    {
        RMDebugInfo("The WAV file({}) contains a not supported channel count, find({}) expected 1 or 2",
            path, wav.channels
        );
    }

    const usize total_samples = static_cast<usize>(wav.totalPCMFrameCount * wav.channels);
    data.mono = wav.channels == 1;
    data.samples = allocator.array<i16>(total_samples);

    // Always convert 
    (void)drwav_read_pcm_frames_s16(&wav, wav.totalPCMFrameCount, reinterpret_cast<drwav_int16*>(data.samples.ptr()));

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
