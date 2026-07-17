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

void RenderGraph::execute(GPU::CommandBufferID command_buffer, const FrameInfo& frame_info)
{
    FrameContext& context =  get_frame_context(frame_info);
    context.syncronize_memory(command_buffer);
    
    PassResources resources =
    {
        .command_buffer = command_buffer,
        .global_device_vertex_buffer = context.get_transient_vertex_buffer_local(),
        .context = context,
    };

    const GPU::PipelineTextureBarrier render_attachment_barrier =
    {
        .src_masks = GPU::AccessMasks(),
        .dest_masks = GPU::AccessMasks::RenderAttachmentWrite,
        .src_layout = GPU::TextureLayout::Unknown,
        .dest_layout = GPU::TextureLayout::RenderAttachment,
        .texture = frame_info.image,
        .subresource_range = GPU::TextureSubresourceRange::color(0, 1, 0, 1),
    };
    GPU::command_buffer_pipeline_barrier(
        command_buffer,
        GPU::PipelineBarrier::texture_barrier(
            GPU::PipelineStages::RenderOutput, GPU::PipelineStages::RenderOutput,
            Slice(&render_attachment_barrier, 1)
        )
    );

    for(Pass& pass : data.passes.iter())
    {
        const GPU::AttachmentInfo render_attachment =
        {
            .texture_view = frame_info.image_view,
            .layout = GPU::TextureLayout::RenderAttachment,
            .resolve_texture_view = GPU::TextureViewID::invalid(),
            .resolve_layout = GPU::TextureLayout::Unknown,
            .load_op = GPU::LoadOp::Clear,
            .store_op = GPU::StoreOp::Store,
            // TODO: Handle custom clear values.
            .clear_value = GPU::ClearValue::rgba(0.F, 0.F, 0.F, 1.F),
        };

        GPU::command_buffer_begin_renderpass(
            command_buffer,
            {
                .offset = Vector2I(0, 0),
                .extent = Vector3U(frame_info.image_size.x, frame_info.image_size.y, 1),
                .render_attachments = Slice(&render_attachment, 1),
                .depth_attachment = {},
                .stencil_attachment = {},
            }
        );

        GPU::Viewport viewport = GPU::Viewport::extent(frame_info.image_size.x, frame_info.image_size.y);
        GPU::Scissor scissor = GPU::Scissor::extent(frame_info.image_size.x, frame_info.image_size.y);

        GPU::command_buffer_set_viewports(command_buffer, 0, Slice(&viewport, 1));
        GPU::command_buffer_set_scissors(command_buffer, 0, Slice(&scissor, 1));

        pass.execute.call(resources);
        
        GPU::command_buffer_end_renderpass(command_buffer, {});
    }

    const GPU::PipelineTextureBarrier present_barrier =
    {
        .src_masks = GPU::AccessMasks::RenderAttachmentWrite,
        .dest_masks = GPU::AccessMasks(),
        .src_layout = GPU::TextureLayout::RenderAttachment,
        .dest_layout = GPU::TextureLayout::Present,
        .texture = frame_info.image,
        .subresource_range = GPU::TextureSubresourceRange::color(0, 1, 0, 1),
    };
    GPU::command_buffer_pipeline_barrier(
        command_buffer,
        GPU::PipelineBarrier::texture_barrier(
            GPU::PipelineStages::RenderOutput, GPU::PipelineStages::End,
            Slice(&present_barrier, 1)
        )
    );
}

}