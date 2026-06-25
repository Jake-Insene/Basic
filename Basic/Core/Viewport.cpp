#include "Core/Viewport.hpp"

#include "engine/engine.h"


namespace Basic
{

Viewport Viewport::create(GPU::TextureFormat viewport_format, const Vector2I& viewport_size)
{
    Viewport viewport = {};
    for(u32 i = 0; i < RendererCore::MaxFramesInFlight; i++)
    {
        viewport.data.viewport_textures[i] = GPU::texture_create(
            Engine::get_render_device()->get_device(),
            GPU::TextureCreateInfo::create(
                GPU::TextureType::Texture2D, viewport_format,
                Vector3U(viewport_size.x, viewport_size.y, 1), 1, 1,
                GPU::SampleCount::Sample1,
                GPU::TextureTiling::Optimal,
                GPU::TextureUsage::Sampled | GPU::TextureUsage::RenderOutput,
                GPU::TextureLayout::Unknown,
                GPU::TextureSubresourceRange::color(0, 1, 0, 1)
            )
        );

        viewport.data.viewport_texture_views[i] = GPU::texture_view_create(
            Engine::get_render_device()->get_device(),
            GPU::TextureViewCreateInfo::create(
                GPU::TextureViewType::Texture2D, viewport_format,
                viewport.data.viewport_textures[i],
                GPU::ComponentMapping::identity(),
                GPU::TextureSubresourceRange::color(0, 1, 0, 1)
            )
        );
    }

    return viewport;
};

void Viewport::destroy()
{
    for(u32 i = 0; i < RendererCore::MaxFramesInFlight; i++)
    {
        GPU::texture_destroy(data.viewport_textures[i]);
        GPU::texture_view_destroy(data.viewport_texture_views[i]);
    }
}

}
