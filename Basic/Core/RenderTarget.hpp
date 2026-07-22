#pragma once
#include <gpu/gpu.h>
#include <graphics/gpu_memory_allocator_types.h>

#include "Basic/Core/RenderCore.hpp"


namespace Basic
{

struct RenderTarget
{
    DisableCopy(RenderTarget);
    DisableMove(RenderTarget);
    
    struct InternalData
    {
        GPU::TextureID textures[MaxFramesInFlight];
        GPUMemoryAllocationID allocations[MaxFramesInFlight];
        GPU::TextureViewID texture_views[MaxFramesInFlight];

        GPU::TextureFormat render_target_format;
        Vector2I size;
    } data;

    RenderTarget(GPU::TextureFormat render_target_format, const Vector2I& size);
    ~RenderTarget();

    GPU::TextureID get_texture(u32 frame_index);
    GPU::TextureViewID get_texture_view(u32 frame_index);

    GPU::TextureFormat get_format() const { return data.render_target_format; };
    Vector2I get_size() const { return data.size; };
};

}