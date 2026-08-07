#include "Basic/Core/RenderCore.hpp"

#include <engine/engine.h>


namespace Basic
{

FrameContext::FrameContext(Graphics::RenderDevice* render_device)
{
    data.render_device = render_device;

    data.transient_vertex_buffer = GPU::buffer_create(render_device->get_device(),
        GPU::BufferCreateInfo::create(GPU::BufferUsage::VertexBuffer | GPU::BufferUsage::TransferSource, InitialTransientSize));
    
    data.transient_vertex_buffer_allocation = Engine::get_gpu_memory_allocator()->allocate(
        Graphics::GPUMemoryAllocator::AllocationTag::Staging,
        GPU::buffer_get_memory_requirements(data.transient_vertex_buffer));
    
    data.transient_mapped = Engine::get_gpu_memory_allocator()->allocation_map(
        data.transient_vertex_buffer_allocation);
    GPU::buffer_bind_memory_heap(data.transient_vertex_buffer,
        GPU::BindMemoryInfo::create(
            Engine::get_gpu_memory_allocator()->allocation_get_heap(data.transient_vertex_buffer_allocation),
            Engine::get_gpu_memory_allocator()->allocation_get_offset(data.transient_vertex_buffer_allocation)
        )
    );
    
    data.transient_vertex_buffer_local = GPU::buffer_create(render_device->get_device(),
        GPU::BufferCreateInfo::create(GPU::BufferUsage::VertexBuffer | GPU::BufferUsage::TransferDestination, InitialTransientSize));
    
    data.transient_vertex_buffer_local_allocation = Engine::get_gpu_memory_allocator()->allocate(
        Graphics::GPUMemoryAllocator::AllocationTag::Buffer,
        GPU::buffer_get_memory_requirements(data.transient_vertex_buffer_local));

    GPU::buffer_bind_memory_heap(data.transient_vertex_buffer_local,
        GPU::BindMemoryInfo::create(
            Engine::get_gpu_memory_allocator()->allocation_get_heap(data.transient_vertex_buffer_local_allocation),
            Engine::get_gpu_memory_allocator()->allocation_get_offset(data.transient_vertex_buffer_local_allocation)
        )
    );
    
    data.current_offset = 0;

    const GPU::DescriptorPoolSize pool_sizes[] =
    {
        GPU::DescriptorPoolSize::uniform_buffer(MaxUniformBuffers),
        GPU::DescriptorPoolSize::combined_texture_sampler(MaxCombinedTextureSamplers),
    };
    
    data.pool = GPU::descriptor_pool_create(render_device->get_device(),
        GPU::DescriptorPoolCreateInfo::create(MaxSets, pool_sizes));
}

FrameContext::~FrameContext()
{
    Engine::get_gpu_memory_allocator()->free(data.transient_vertex_buffer_local_allocation);
    GPU::buffer_destroy(data.transient_vertex_buffer_local);

    Engine::get_gpu_memory_allocator()->free(data.transient_vertex_buffer_allocation);
    GPU::buffer_destroy(data.transient_vertex_buffer);

    GPU::descriptor_pool_destroy(data.pool);
}

void FrameContext::reset()
{
    data.current_offset = 0;

    GPU::descriptor_pool_reset(data.pool);
}

void FrameContext::syncronize_memory(GPU::CommandBufferID command_buffer)
{
    if(data.current_offset == 0)
    {
        return;
    }

    const GPU::BufferCopyRegion copy_regions[] =
    {
        GPU::BufferCopyRegion::create(0, 0, data.current_offset),
    };

    GPU::command_buffer_copy_buffer(command_buffer,
        GPU::CopyBufferInfo::create(data.transient_vertex_buffer,
            data.transient_vertex_buffer_local, copy_regions));
}

TransientAllocation FrameContext::allocate_transient_vertex(usize size)
{
    TransientAllocation allocation =
    {
        .size = size,
        .offset = data.current_offset,
        .mapped = data.transient_mapped.add(data.current_offset),
    };

    data.current_offset += size;
    return allocation;
}

GPU::DescriptorSetID FrameContext::allocate_descriptor_set(GPU::DescriptorSetLayoutID set_layout)
{
    GPU::DescriptorSetID set = GPU::DescriptorSetID::invalid();
    
    GPU::descriptor_set_allocate(data.render_device->get_device(),
        GPU::DescriptorSetAllocateInfo::create(data.pool, Slice(&set_layout, 1)),
        Slice(&set, 1));

    return set;
}

GPU::BufferID FrameContext::get_transient_vertex_buffer()
{
    return data.transient_vertex_buffer;
}

GPU::BufferID FrameContext::get_transient_vertex_buffer_local()
{
    return data.transient_vertex_buffer_local;
}

}