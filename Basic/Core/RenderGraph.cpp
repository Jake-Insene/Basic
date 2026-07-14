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

    if(context.data.current_offset == 0)
    {
        return;
    }

    const GPU::BufferCopyRegion copy_regions[] =
    {
        GPU::BufferCopyRegion::create(0, 0, context.data.current_offset),
    };

    GPU::command_buffer_copy_buffer(command_buffer,
        GPU::CopyBufferInfo::create(context.data.transient_vertex_buffer,
            context.data.transient_vertex_buffer_local, copy_regions));
}

void RenderGraph::execute(GPU::CommandBufferID command_buffer, const FrameInfo& frame_info)
{
    PassResources resources =
    {
        .command_buffer = command_buffer,
        .global_device_vertex_buffer = get_frame_context(frame_info).data.transient_vertex_buffer_local,
    };

    for(Pass& pass : data.passes.iter())
    {
        pass.execute.call(resources);
    }
}

}