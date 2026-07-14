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
    Mem::Allocator* allocator;
    Graphics::RenderDevice* render_device;
    Graphics::SwapChain* swap_chain;
};

struct Renderer
{
    DisableCopy(Renderer);
    DisableMove(Renderer);
    
    struct RenderFrame
    {
        GPU::SemaphoreID present_complete_semaphore;
        GPU::FenceID in_flight_fence;
    };

    struct InternalData
    {
        Mem::Allocator* allocator;
        Graphics::RenderDevice* render_device;
        Graphics::SwapChain* swap_chain;

        Graphics::CommandPool command_pool;
        u32 frame_index;
        
        RenderFrame frames[MaxFramesInFlight];
        Array<GPU::SemaphoreID> render_finished_semaphores;
    } data;

    Renderer(const RendererCreateInfo& info);
    ~Renderer();

    FrameInfo begin_frame();
    void end_frame(const FrameInfo& frame_info);

    GPU::CommandBufferID acquire_command_buffer(const FrameInfo& frame_info);
    void submit_command_buffer(const FrameInfo& frame_info, const Slice<const GPU::PipelineStages>& wait_stages, GPU::CommandBufferID command_buffer);
    void present(const FrameInfo& frame_info);
};

}

EnableBitOp(Basic::FrameFlags);

