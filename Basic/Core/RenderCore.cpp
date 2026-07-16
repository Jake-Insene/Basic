#include "Basic/Core/RenderCore.hpp"


namespace Basic
{

FrameContext::FrameContext(Graphics::RenderDevice* render_device)
{
    data.render_device = render_device;

    data.transient_vertex_buffer = GPU::buffer_create(render_device->get_device(),
        GPU::BufferCreateInfo::create(GPU::BufferUsage::VertexBuffer | GPU::BufferUsage::TransferSource, InitialTransientSize));
    
    data.transient_vertex_buffer_allocation = render_device->get_gpu_memory_allocator()->allocate(
        Graphics::GPUMemoryAllocator::AllocationTag::Staging,
        GPU::buffer_get_memory_requirements(data.transient_vertex_buffer));
    
    data.transient_mapped = render_device->get_gpu_memory_allocator()->allocation_map(
        data.transient_vertex_buffer_allocation);
    GPU::buffer_bind_memory_heap(data.transient_vertex_buffer,
        GPU::BindMemoryInfo::create(
            render_device->get_gpu_memory_allocator()->allocation_get_heap(data.transient_vertex_buffer_allocation),
            render_device->get_gpu_memory_allocator()->allocation_get_offset(data.transient_vertex_buffer_allocation)
        )
    );
    
    data.transient_vertex_buffer_local = GPU::buffer_create(render_device->get_device(),
        GPU::BufferCreateInfo::create(GPU::BufferUsage::VertexBuffer | GPU::BufferUsage::TransferDestination, InitialTransientSize));
    
    data.transient_vertex_buffer_local_allocation = render_device->get_gpu_memory_allocator()->allocate(
        Graphics::GPUMemoryAllocator::AllocationTag::Buffer,
        GPU::buffer_get_memory_requirements(data.transient_vertex_buffer_local));

    GPU::buffer_bind_memory_heap(data.transient_vertex_buffer_local,
        GPU::BindMemoryInfo::create(
            render_device->get_gpu_memory_allocator()->allocation_get_heap(data.transient_vertex_buffer_local_allocation),
            render_device->get_gpu_memory_allocator()->allocation_get_offset(data.transient_vertex_buffer_local_allocation)
        )
    );
    
    data.current_offset = 0;
}

FrameContext::~FrameContext()
{
    data.render_device->get_gpu_memory_allocator()->free(data.transient_vertex_buffer_local_allocation);
    GPU::buffer_destroy(data.transient_vertex_buffer_local);

    data.render_device->get_gpu_memory_allocator()->free(data.transient_vertex_buffer_allocation);
    GPU::buffer_destroy(data.transient_vertex_buffer);
}

void FrameContext::reset()
{
    data.current_offset = 0;
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

GPU::BufferID FrameContext::get_transient_vertex_buffer()
{
    return data.transient_vertex_buffer;
}

GPU::BufferID FrameContext::get_transient_vertex_buffer_local()
{
    return data.transient_vertex_buffer_local;
}

}