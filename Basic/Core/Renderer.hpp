#pragma once
#include <gpu/gpu.h>
#include <graphics/command_pool.h>
#include <graphics/swap_chain.h>
#include <graphics/render_device.h>

#include "Basic/Core/RenderCore.hpp"


struct GPUMemoryAllocator;

namespace Basic
{

struct RendererCreateInfo
{
    // Renderer allocator.
    Mem::Allocator* allocator;
    // The Render Device to use.
    Graphics::RenderDevice* render_device;
    // The Swap Chain containing the image to render to.
    Graphics::SwapChain* swap_chain;
};

/**
* Basic renderer. Can begin and end a frame, acquire and present images.
*/
struct Renderer
{
    DisableCopy(Renderer);
    DisableMove(Renderer);
    
    struct RenderFrame
    {
        GPU::SemaphoreID present_complete_semaphore;
        GPU::FenceID in_flight_fence;
    };

    Mem::Allocator* allocator;
    Graphics::RenderDevice* render_device;
    Graphics::SwapChain* swap_chain;

    Graphics::CommandPool command_pool;
    u32 frame_index;
        
    RenderFrame frames[MaxFramesInFlight];
    Array<GPU::SemaphoreID> render_finished_semaphores;

    Renderer(const RendererCreateInfo& info);
    ~Renderer();

    /**
    * It try to acquire a new frame and begin rendering.
    */
    FrameInfo begin_frame();

    /**
    * Ends the current frame
    */
    void end_frame();

    // TODO: This interface doesn't allow multiple command buffers.
    GPU::CommandBufferID acquire_command_buffer(const FrameInfo& frame_info);
    void submit_command_buffer(const FrameInfo& frame_info, const Slice<const GPU::PipelineStages>& wait_stages, GPU::CommandBufferID command_buffer);
    
    /**
    * Sends the current frame to the presentation engine.
    */
    void present(const FrameInfo& frame_info);
};

}

EnableBitOp(Basic::FrameFlags);

