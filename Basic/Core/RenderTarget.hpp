#pragma once
#include <gpu/gpu.h>

#include "Basic/Core/GPUMemoryAllocator.hpp"
#include "Basic/Core/RenderCore.hpp"
#include "Basic/Core/RenderDevice.hpp"


namespace Basic
{

struct RenderTarget
{
    DisableCopy(RenderTarget);
    DisableMove(RenderTarget);
    
    struct InternalData
    {
        RenderDevice& render_device;

        GPU::TextureID textures[MaxFramesInFlight];
        GPUMemoryAllocationID allocations[MaxFramesInFlight];
        GPU::TextureViewID texture_views[MaxFramesInFlight];

        GPU::TextureFormat render_target_format;
        Vector2I size;

        InternalData(RenderDevice& render_device)
        : render_device(render_device)
        {}
    } data;

    RenderTarget(RenderDevice& render_device, GPU::TextureFormat render_target_format, const Vector2I& size);
    ~RenderTarget();

    GPU::TextureID get_texture(u32 frame_index) const;
    GPU::TextureViewID get_texture_view(u32 frame_index) const;

    GPU::TextureFormat get_format() const { return data.render_target_format; };
    Vector2I get_size() const { return data.size; };
};

}