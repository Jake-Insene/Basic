#include "Basic/Core/Renderer.hpp"


namespace Basic
{

Renderer::Renderer(const RendererCreateInfo& info)
: allocator(info.allocator), render_device(info.render_device),
swap_chain(info.swap_chain),
command_pool(
    {
        .allocator = info.allocator,
        .device = info.render_device->get_device(),
        .queue_usage = GPU::QueueUsage::Graphics,
    }
),
frame_index(), frames(),
render_finished_semaphores(info.allocator, info.swap_chain->get_image_count(), {})
{
    frame_index = 0;

    Core::Mem::Placement(frames);
    for(RenderFrame& frame : frames)
    {
        frame.present_complete_semaphore = GPU::semaphore_create(render_device->get_device(), {}),
        frame.in_flight_fence = GPU::FenceID::invalid();
    }

    render_finished_semaphores.resize(swap_chain->get_image_count());
    (void)render_finished_semaphores.iter().transform([&](GPU::SemaphoreID)
    {
        return GPU::semaphore_create(render_device->get_device(), {});
    });
}

Renderer::~Renderer()
{
    // Work may be on flight.
    GPU::queue_wait_idle(render_device->get_graphics_queue());

    (void)render_finished_semaphores.iter().for_each([](GPU::SemaphoreID sem)
    {
        GPU::semaphore_destroy(sem);
    });

    for(const RenderFrame& frame : frames)
    {
        GPU::semaphore_destroy(frame.present_complete_semaphore);
    }
}

FrameInfo Renderer::begin_frame()
{
    RenderFrame& frame = frames[frame_index];

    if(frame.in_flight_fence != GPU::FenceID::invalid())
    {
        GPU::fence_wait_for(Slice(&frame.in_flight_fence, 1), true, Core::MaxValue<u64>);
    }

    // Acquiring image
    GPU::PipelineStages wait_stages[] =
    {
        GPU::PipelineStages::RenderOutput,
    };

    u32 image_index = Core::MaxValue<u32>;
    bool image_acquired = swap_chain->acquire_image(
        &image_index,
        frame.present_complete_semaphore
    );
    if(image_acquired && frame.in_flight_fence != GPU::FenceID::invalid())
    {
        command_pool.release_fence(frame.in_flight_fence);
        frame.in_flight_fence = GPU::FenceID::invalid();
    }

    FrameFlags frame_flags = FrameFlags();
    GPU::TextureID image = GPU::TextureID::invalid();
    GPU::TextureViewID image_view = GPU::TextureViewID::invalid();
    if(image_index == Core::MaxValue<u32> && image_acquired)
    {
        frame.in_flight_fence = command_pool.execute_empty(
            render_device->get_graphics_queue(),
            {
                .wait_semaphores = Slice(&frame.present_complete_semaphore, 1),
                .wait_stages = wait_stages,
                .signal_semaphores = {},
            }
        );
    }
    else if(image_acquired && image_index != Core::MaxValue<u32>)
    {
        frame_flags |= FrameFlags::Acquired;
        image = swap_chain->get_image(image_index).image;    
        image_view = swap_chain->get_image(image_index).image_view;    
    }

    return FrameInfo
    {
        .flags = frame_flags,
        .frame_index = frame_index,
        .image_index = image_index,
        .image = image,
        .image_view = image_view,
        .image_size = swap_chain->get_image_size(),
    };
}

void Renderer::end_frame()
{
    frame_index = (frame_index + 1) % MaxFramesInFlight;
}

GPU::CommandBufferID Renderer::acquire_command_buffer(const FrameInfo&)
{
    return command_pool.acquire_command_buffer();
}

void Renderer::submit_command_buffer(const FrameInfo& frame_info, const Slice<const GPU::PipelineStages>& wait_stages,
    GPU::CommandBufferID command_buffer)
{
    RenderFrame& frame = frames[frame_info.frame_index];
    frame.in_flight_fence = command_pool.execute(
        render_device->get_graphics_queue(),
        {
            .wait_semaphores = Slice(&frame.present_complete_semaphore, 1),
            .wait_stages = wait_stages,
            .command_buffer = command_buffer,
            .signal_semaphores = Slice(&render_finished_semaphores.get(frame_info.image_index), 1),
        }
    );
}

void Renderer::present(const FrameInfo& frame_info)
{
    swap_chain->present(
        frame_info.image_index,
        Slice(&render_finished_semaphores.get(frame_info.image_index), 1)
    );
}

}
