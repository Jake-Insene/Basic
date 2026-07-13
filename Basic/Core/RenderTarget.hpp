#pragma once
#include "gpu/gpu.h"

#include "Core/RenderCore.hpp"


namespace Basic
{

struct RenderTarget
{
    DisableCopy(RenderTarget);
    DisableMove(RenderTarget);
    
    struct InternalData
    {
        GPU::TextureID textures[MaxFramesInFlight];
        GPU::TextureViewID texture_views[MaxFramesInFlight];

        GPU::TextureFormat render_target_format;
    } data;

    RenderTarget(GPU::TextureFormat render_target_format, const Vector2I& size);
    ~RenderTarget();

    GPU::TextureID get_texture(u32 frame_index);
    GPU::TextureViewID get_texture_view(u32 frame_index);
};

}