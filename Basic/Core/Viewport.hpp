#pragma once
#include "gpu/gpu.h"

#include "Core/RendererCore.hpp"


namespace Basic
{

struct Viewport
{

    struct InternalData
    {
        GPU::TextureID viewport_textures[RendererCore::MaxFramesInFlight];
        GPU::TextureViewID viewport_texture_views[RendererCore::MaxFramesInFlight];

        GPU::TextureFormat viewport_format;
    } data;

    static Viewport create(GPU::TextureFormat viewport_format, const Vector2I& viewport_size);

    void destroy();
};

}
