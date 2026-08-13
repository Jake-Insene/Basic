#pragma once
#include <Collections/Array.hpp>
#include <Collections/Delegate.hpp>
#include <graphics/render_device.h>
#include <Mem/LinearAllocator.hpp>

#include "Basic/Core/RenderCore.hpp"


namespace Basic
{

/**
* Current frame pass resources.
*/
struct PassResources
{
    GPU::CommandBufferID command_buffer;

    GPU::BufferID global_device_vertex_buffer;
    Slice<GPU::TextureViewID> resolved_render_targets;

    FrameContext& context;

    GPU::TextureViewID get_render_target_view(RenderTargetHandle handle)
    {
        return resolved_render_targets[handle.id];
    }
};

struct PassWriteAttachment
{
    RenderTargetHandle rt;
    GPU::LoadOp load_op;
    GPU::StoreOp store_op;
    GPU::ClearValue clear_value;
};

struct PassTexture
{
    RenderTargetHandle texture;
    GPU::ShaderStage stage;
};

struct PassBuilder
{
    DisableCopy(PassBuilder);
    DisableMove(PassBuilder);

    struct InternalData
    {
        Mem::Allocator& allocator;
        Collections::Array<PassWriteAttachment> writes;
        Collections::Array<PassTexture> textures;
    } data;

    PassBuilder(Mem::Allocator& allocator);
    ~PassBuilder();

    void clear();

    PassBuilder& write_render_attachment(RenderTargetHandle texture, GPU::LoadOp load_op,
        GPU::StoreOp store_op, GPU::ClearValue clear_value);

    PassBuilder& read_texture(RenderTargetHandle rt, GPU::ShaderStage stage);
};

struct RenderGraph
{
    DisableCopy(RenderGraph);
    DisableMove(RenderGraph);

    struct Pass
    {
        Collections::Array<PassWriteAttachment> writes;
        Collections::Array<PassTexture> textures;
        Delegate<void(PassBuilder&)> setup;
        Delegate<void(PassResources&)> execute;

        Pass(Mem::Allocator& allocator)
        : writes(allocator, 4, {}), textures(allocator, 4, {}),
        setup(allocator), execute(allocator)
        {}
    };

    struct VirtualRenderTarget
    {
        RenderTargetHandle handle;
        Vector2I extent;
        GPU::TextureFormat format;
        GPU::TextureID texture;
        GPU::TextureViewID texture_view;
        bool imported;
    };

    struct InternalData
    {
        Mem::Allocator& allocator;
        Graphics::RenderDevice* render_device;

        FrameContext frame_context[MaxFramesInFlight];

        Collections::Array<Pass> passes;
        Collections::Array<VirtualRenderTarget> virtual_render_targets;

        Mem::LinearAllocator tmp_allocator;
    } data;

    RenderGraph(Mem::Allocator& allocator, Graphics::RenderDevice* render_device);
    ~RenderGraph();

    FrameContext& get_frame_context(const FrameInfo& frame_info);

    RenderTargetHandle import_render_target(GPU::TextureID texture, GPU::TextureViewID texture_view,
        const Vector2I& extent);

    template<typename SetupFn, typename ExecuteFn>
    void add_raster_pass(SetupFn setup, ExecuteFn execute)
    {
        Pass& pass = data.passes.emplace(data.allocator);
        pass.setup.bind([setup](PassBuilder& builder){ setup(builder); });
        pass.execute.bind([execute](PassResources& resources){ execute(resources); });
    }

    void clear();

    void compile();
    void execute(GPU::CommandBufferID command_buffer, const FrameInfo& frame_info);

    void _begin_backbuffer(GPU::CommandBufferID command_buffer, const FrameInfo& frame_info);
    void _end_backbuffer(GPU::CommandBufferID command_buffer, const FrameInfo& frame_info);

    Vector2I _resolve_extent_for_pass(PassResources& resources, Pass& pass, const FrameInfo& frame_info);
    Slice<GPU::AttachmentInfo> _resolve_attachments_for_pass(Mem::Allocator& allocator, PassResources& resources,
        Pass& pass, const FrameInfo& frame_info);

    Slice<GPU::PipelineTextureBarrier> _resolve_begin_barriers_for_pass(Mem::Allocator& allocator, PassResources& resources, Pass& pass,
        const FrameInfo& frame_info);
    Slice<GPU::PipelineTextureBarrier> _resolve_end_barriers_for_pass(Mem::Allocator& allocator, PassResources& resources, Pass& pass,
        const FrameInfo& frame_info);

    void _execute_pass(Mem::Allocator& allocator, GPU::CommandBufferID command_buffer, Pass& pass,
        PassResources& resources, const FrameInfo& frame_info);
};

}