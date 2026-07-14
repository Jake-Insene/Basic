#include "Basic/Core/RenderGraph.hpp"


namespace Basic
{

RenderGraph::RenderGraph(Mem::Allocator* allocator, Graphics::RenderDevice* render_device) :
data{
    .allocator = allocator,
    .render_device = render_device,
    .frame_context = {render_device, render_device, render_device},
    .passes = Array<Pass>::with_size(allocator, 4),
}
{
}

RenderGraph::~RenderGraph()
{
    clear();
    
    data.passes.destroy();
}

void RenderGraph::clear()
{
    for(Pass& pass : data.passes.iter())
    {
        pass.execute.destroy();
    }

    data.passes.clear();
}

void RenderGraph::begin_frame(GPU::CommandBufferID command_buffer, const FrameInfo& frame_info)
{
    FrameContext& context =  get_frame_context(frame_info);
    context.syncronize_memory(command_buffer);
}

void RenderGraph::execute(GPU::CommandBufferID command_buffer, const FrameInfo& frame_info)
{
    PassResources resources =
    {
        .command_buffer = command_buffer,
        .global_device_vertex_buffer = get_frame_context(frame_info).get_transient_vertex_buffer_local(),
    };

    for(Pass& pass : data.passes.iter())
    {
        pass.execute.call(resources);
    }
}

}