#pragma once
#include <collections/array.h>
#include <collections/delegate.h>
#include <graphics/render_device.h>

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

    FrameContext& context;
};

struct PassAttachment
{
    RenderTargetHandle rt;
    GPU::LoadOp load_op;
    GPU::StoreOp store_op;
    GPU::ClearValue clear_value;
};

struct PassBuilder
{
    DisableCopy(PassBuilder);
    DisableMove(PassBuilder);

    struct InternalData
    {
        Mem::Allocator* allocator;
        Array<PassAttachment> attachments;
    } data;

    PassBuilder(Mem::Allocator* allocator);
    ~PassBuilder();

    PassBuilder& write_render_attachment(RenderTargetHandle rt, GPU::LoadOp load_op,
        GPU::StoreOp store_op, GPU::ClearValue clear_value);
};

struct RenderGraph
{
    DisableCopy(RenderGraph);
    DisableMove(RenderGraph);

    struct Pass
    {
        Array<PassAttachment> attachments;
        Delegate<void(PassBuilder&)> setup;
        Delegate<void(PassResources&)> execute;
    };

    struct InternalData
    {
        Mem::Allocator* allocator;
        Graphics::RenderDevice* render_device;

        FrameContext frame_context[MaxFramesInFlight];

        Array<Pass> passes;
    } data;

    RenderGraph(Mem::Allocator* allocator, Graphics::RenderDevice* render_device);
    ~RenderGraph();

    FrameContext& get_frame_context(const FrameInfo& frame_info)
    {
        return data.frame_context[frame_info.frame_index];
    }

    template<typename SetupFn, typename ExecuteFn>
    void add_raster_pass(SetupFn setup, ExecuteFn execute)
    {
        Pass& pass = data.passes.add(
            Pass
            {
                .attachments = Array<PassAttachment>::with_allocator(data.allocator),
                .setup = Delegate<void(PassBuilder&)>::create(data.allocator),
                .execute = Delegate<void(PassResources&)>::create(data.allocator)
            }
        );
        pass.setup.bind([setup](PassBuilder& builder){ setup(builder); });
        pass.execute.bind([execute](PassResources& resources){ execute(resources); });
    }

    void clear();

    void compile();
    void execute(GPU::CommandBufferID command_buffer, const FrameInfo& frame_info);
};

}