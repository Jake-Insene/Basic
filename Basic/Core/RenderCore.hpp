#pragma once
#include "gpu/gpu.h"
#include "graphics/render_device.h"


namespace Basic
{

static constexpr u32 MaxFramesInFlight = 3;
static constexpr GPU::TextureFormat DefaultViewportFormat = GPU::TextureFormat::RGBA8Srgb;

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

struct TransientAllocation
{
    // Size of allocated space.
    usize size;
    // Offset in transient buffer.
    usize offset;
    // CPU Mapped data of at least size's bytes
    Slice<u8> mapped;
};

struct FrameContext
{
    // 16 MB
    static constexpr usize InitialTransientSize = 1024 * 1024 * 16;

    struct InternalData
    {
        Graphics::RenderDevice* render_device;

        GPU::BufferID transient_vertex_buffer;
        GPUMemoryAllocationID transient_vertex_buffer_allocation;
        Slice<u8> transient_mapped;

        GPU::BufferID transient_vertex_buffer_local;
        GPUMemoryAllocationID transient_vertex_buffer_local_allocation;

        usize current_offset;
    } data;

    FrameContext(Graphics::RenderDevice* render_device);
    ~FrameContext();

    void reset();

    void syncronize_memory(GPU::CommandBufferID command_buffer);

    TransientAllocation allocate_transient_vertex(usize size);

    GPU::BufferID get_transient_vertex_buffer();
    GPU::BufferID get_transient_vertex_buffer_local();
};

}
