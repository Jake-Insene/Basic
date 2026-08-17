#include "Basic/Core/RenderTarget.hpp"



namespace Basic
{

RenderTarget::RenderTarget(RenderDevice& render_device, GPU::TextureFormat render_target_format,
    const Vector2I& size)
: data(render_device)
{
    for(u32 i = 0; i < MaxFramesInFlight; i++)
    {
        data.textures[i] = GPU::texture_create(
            render_device.get_device(),
            GPU::TextureCreateInfo::create(
                GPU::TextureType::Texture2D, render_target_format,
                Vector3U(size.x, size.y, 1), 1, 1,
                GPU::SampleCount::Sample1,
                GPU::TextureTiling::Optimal,
                GPU::TextureUsage::Sampled | GPU::TextureUsage::RenderOutput,
                GPU::TextureLayout::Unknown,
                GPU::TextureSubresourceRange::color(0, 1, 0, 1)
            )
        );

        data.allocations[i] = render_device.get_gpu_memory_allocator().allocate(
            AllocationTag::Texture,
            GPU::texture_get_memory_requirements(data.textures[i])
        );

        GPU::texture_bind_memory_heap(data.textures[i],
            GPU::BindMemoryInfo::create(
                render_device.get_gpu_memory_allocator().allocation_get_heap(data.allocations[i]),
                render_device.get_gpu_memory_allocator().allocation_get_offset(data.allocations[i])
            )
        );

        data.texture_views[i] = GPU::texture_view_create(
            render_device.get_device(),
            GPU::TextureViewCreateInfo::create(
                GPU::TextureViewType::Texture2D, render_target_format,
                data.textures[i],
                GPU::ComponentMapping::identity(),
                GPU::TextureSubresourceRange::color(0, 1, 0, 1)
            )
        );
    }

    data.render_target_format = render_target_format;
    data.size = size;
}

RenderTarget::~RenderTarget()
{
    for(u32 i = 0; i < MaxFramesInFlight; i++)
    {
        GPU::texture_view_destroy(get_texture_view(i));
     
        data.render_device.get_gpu_memory_allocator().free(data.allocations[i]);
        GPU::texture_destroy(get_texture(i));
    }
}

GPU::TextureID RenderTarget::get_texture(u32 frame_index) const
{
    return data.textures[frame_index];
}

GPU::TextureViewID RenderTarget::get_texture_view(u32 frame_index) const
{
    return data.texture_views[frame_index];
}

}
