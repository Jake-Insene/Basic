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
};

struct RenderGraph
{
    DisableCopy(RenderGraph);
    DisableMove(RenderGraph);

    struct Pass
    {
        Delegate<void(PassResources& resources)> execute;
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

    template<typename ExecuteFn>
    void add_raster_pass(ExecuteFn execute)
    {
        Pass& pass = data.passes.add(
            Pass
            {
                .execute = Delegate<void(PassResources&)>::create(data.allocator)
            }
        );
        pass.execute.bind([execute](PassResources& resources){ execute(resources); });
    }

    void clear();

    void execute(GPU::CommandBufferID command_buffer, const FrameInfo& frame_info);
};

}