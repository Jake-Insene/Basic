#pragma once
#include "gpu/gpu.h"
#include "graphics/command_pool.h"
#include "graphics/swap_chain.h"

#include "Core/RendererCore.hpp"


struct GPUMemoryAllocator;

namespace Basic
{

enum class FrameFlags : u8
{
    Acquired = Bit(0),
};

struct FrameInfo
{
    FrameFlags flags;
    u32 frame_index;
    u32 image_index;
    GPU::TextureID image;
    GPU::TextureViewID image_view;
};

struct RendererCreateInfo
{
    Mem::Allocator* allocator;
    GPU::DeviceID device;
    GPU::QueueID graphics_queue;
    GPUMemoryAllocator* gpu_memory_allocator;
    Graphics::SwapChain* swap_chain;
};

struct Renderer
{
    struct RenderFrame
    {
        GPU::SemaphoreID present_complete_semaphore;
        GPU::FenceID in_flight_fence;
    };

    struct InternalData
    {
        Mem::Allocator* allocator;
        GPU::DeviceID device;
        GPU::QueueID graphics_queue;
        GPUMemoryAllocator* gpu_memory_allocator;
        Graphics::SwapChain* swap_chain;

        Graphics::CommandPool command_pool;
        u32 frame_index;
        
        RenderFrame frames[RendererCore::MaxFramesInFlight];
        Array<GPU::SemaphoreID> render_finished_semaphores;
    } data;

    void init(const RendererCreateInfo& info);
    void destroy();

    FrameInfo begin_frame(Graphics::SwapChain* swap_chain);
    void end_frame(const FrameInfo& frame_info, const Slice<const GPU::PipelineStages>& wait_stages,
        GPU::CommandBufferID command_buffer);

    GPU::CommandBufferID acquire_command_buffer(const FrameInfo& frame_info);
    void present(const FrameInfo& frame_info);
};

}

EnableBitOp(Basic::FrameFlags);

