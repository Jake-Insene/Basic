#include "Core/Renderer.hpp"


namespace Basic
{

void Renderer::init(const RendererCreateInfo& info)
{
    data.allocator = info.allocator;
    data.device = info.device;
    data.graphics_queue = info.graphics_queue;
    data.gpu_memory_allocator = info.gpu_memory_allocator;
    data.swap_chain = info.swap_chain;

    data.command_pool.init(
        {
            .allocator = data.allocator,
            .device = data.device,
            .queue_usage = GPU::QueueUsage::Graphics
        }
    );

    data.frame_index = 0;

    ConstructObject(data.frames);
    for(RenderFrame& frame : data.frames)
    {
        frame.present_complete_semaphore = GPU::semaphore_create(data.device, {}),
        frame.in_flight_fence = GPU::FenceID::invalid();
    }

    data.render_finished_semaphores = Array<GPU::SemaphoreID>::with_size(data.allocator, data.swap_chain->get_image_count());
    data.render_finished_semaphores.resize(data.swap_chain->get_image_count());
    (void)data.render_finished_semaphores.iter().transform([&](GPU::SemaphoreID)
    {
        return GPU::semaphore_create(data.device, {});
    });
}

void Renderer::destroy()
{
    (void)data.render_finished_semaphores.iter().for_each([](GPU::SemaphoreID sem)
    {
        GPU::semaphore_destroy(sem);
    });
    data.render_finished_semaphores.destroy();

    for(const RenderFrame& frame : data.frames)
    {
        GPU::semaphore_destroy(frame.present_complete_semaphore);
    }

    data.command_pool.destroy();
}

FrameInfo Renderer::begin_frame(Graphics::SwapChain* swap_chain)
{
    RenderFrame& frame = data.frames[data.frame_index];

    if(frame.in_flight_fence != GPU::FenceID::invalid())
    {
        GPU::fence_wait_for(Slice(&frame.in_flight_fence, 1), true, MaxValue<u64>);
    }

    // Acquiring image
    GPU::PipelineStages wait_stages[] =
    {
        GPU::PipelineStages::RenderOutput,
    };

    u32 image_index = MaxValue<u32>;
    bool image_acquired = swap_chain->acquire_image(
        &image_index,
        frame.present_complete_semaphore
    );
    if(image_acquired && frame.in_flight_fence != GPU::FenceID::invalid())
    {
        data.command_pool.release_fence(frame.in_flight_fence);
        frame.in_flight_fence = GPU::FenceID::invalid();
    }

    FrameFlags frame_flags = FrameFlags(0);
    GPU::TextureID image = GPU::TextureID::invalid();
    GPU::TextureViewID image_view = GPU::TextureViewID::invalid();
    if(image_index == MaxValue<u32> && image_acquired)
    {
        frame.in_flight_fence = data.command_pool.execute_empty(
            data.graphics_queue,
            {
                .wait_semaphores = Slice(&frame.present_complete_semaphore, 1),
                .wait_stages = wait_stages,
                .signal_semaphores = {},
            }
        );
    }
    else if(image_acquired && image_index != MaxValue<u32>)
    {
        frame_flags |= FrameFlags::Acquired;
        image = swap_chain->get_image(image_index).image;    
        image_view = swap_chain->get_image(image_index).image_view;    
    }

    return FrameInfo
    {
        .flags = frame_flags,
        .frame_index = data.frame_index,
        .image_index = image_index,
        .image = image,
        .image_view = image_view,
    };
}

void Renderer::end_frame(const FrameInfo& frame_info, const Slice<const GPU::PipelineStages>& wait_stages,
    GPU::CommandBufferID command_buffer)
{
    data.frame_index = (data.frame_index + 1) % RendererCore::MaxFramesInFlight;

    RenderFrame& frame = data.frames[frame_info.frame_index];
    frame.in_flight_fence = data.command_pool.execute(
        data.graphics_queue,
        {
            .wait_semaphores = Slice(&frame.present_complete_semaphore, 1),
            .wait_stages = wait_stages,
            .command_buffer = command_buffer,
            .signal_semaphores = Slice(&data.render_finished_semaphores.get(frame_info.image_index), 1),
        }
    );
}

GPU::CommandBufferID Renderer::acquire_command_buffer(const FrameInfo&)
{
    return data.command_pool.acquire_command_buffer();
}

void Renderer::present(const FrameInfo& frame_info)
{
    data.swap_chain->present(
        frame_info.image_index,
        Slice(&data.render_finished_semaphores.get(frame_info.image_index), 1)
    );
}

}
