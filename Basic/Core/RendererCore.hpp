#pragma once
#include "gpu/gpu.h"

namespace Basic
{

struct RendererCore
{
    static constexpr u32 MaxFramesInFlight = 3;

    static constexpr GPU::TextureFormat DefaultViewportFormat = GPU::TextureFormat::RGBA8Srgb;
};

}
