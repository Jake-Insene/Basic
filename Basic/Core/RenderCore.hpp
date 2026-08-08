#pragma once
#include "gpu/gpu.h"
#include "graphics/render_device.h"
#include "graphics/gpu_memory_allocator_types.h"


namespace Basic
{

// This is not likely to change.
static constexpr u32 MaxFramesInFlight = 3;

static constexpr GPU::TextureFormat DefaultViewportFormat = GPU::TextureFormat::RGBA8Srgb;

// Virtual render target. 
struct [[nodiscard]] RenderTargetHandle
{
    static constexpr u32 BackbufferID = 0xFFFF'FFFF;

    u32 id;

    RenderTargetHandle(u32 _id) : id(_id) {}
    RenderTargetHandle() : id(0) {}

    static constexpr RenderTargetHandle backbuffer()
    {
        return BackbufferID;
    }

    bool is_backbuffer() const { return id == BackbufferID;}
    bool is_valid() const { return id != 0; }
};

enum class FrameFlags : u8
{
    // The frame was acquired.
    Acquired = Bit(0),
};

struct FrameInfo
{
    // Frame flags. Useful to know if this frame should render
    FrameFlags flags;
    // The locagical frame index. [0-2]
    u32 frame_index;
    // The image index in the swap chain. [0-N]
    u32 image_index;
    // The current image use by the frame.
    GPU::TextureID image;
    // View for the current image, uses same format as image.
    GPU::TextureViewID image_view;
    // Size of the current image.
    Vector2I image_size;
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
    DisableCopy(FrameContext);
    DisableMove(FrameContext);

    static constexpr usize InitialTransientSize = Core::MiB(16);
    // For now this shouldn't happen.
    static constexpr u32 MaxSets = 1024;
    static constexpr u32 MaxUniformBuffers = 1024;
    static constexpr u32 MaxCombinedTextureSamplers = 1024;

    struct InternalData
    {
        Graphics::RenderDevice* render_device;

        GPU::BufferID transient_vertex_buffer;
        GPUMemoryAllocationID transient_vertex_buffer_allocation;
        Slice<u8> transient_mapped;

        GPU::BufferID transient_vertex_buffer_local;
        GPUMemoryAllocationID transient_vertex_buffer_local_allocation;

        usize current_offset;

        GPU::DescriptorPoolID pool;
    } data;

    FrameContext(Graphics::RenderDevice* render_device);
    ~FrameContext();

    void reset();

    void syncronize_memory(GPU::CommandBufferID command_buffer);

    TransientAllocation allocate_transient_vertex(usize size);
    GPU::DescriptorSetID allocate_descriptor_set(GPU::DescriptorSetLayoutID set_layout);

    GPU::BufferID get_transient_vertex_buffer();
    GPU::BufferID get_transient_vertex_buffer_local();
};

}
