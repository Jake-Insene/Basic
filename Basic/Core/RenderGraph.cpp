#include "Basic/Core/RenderGraph.hpp"


namespace Basic
{

PassBuilder::PassBuilder(Mem::Allocator* allocator) :
data{
    .allocator = allocator,
    .attachments = Array<PassAttachment>::with_allocator(allocator),
}
{
}

PassBuilder::~PassBuilder()
{
    data.attachments.destroy();
}

PassBuilder& PassBuilder::write_render_attachment(RenderTargetHandle rt, GPU::LoadOp load_op,
    GPU::StoreOp store_op, GPU::ClearValue clear_value)
{
    (void)data.attachments.add(
        {.rt = rt, .load_op = load_op, .store_op = store_op, .clear_value = clear_value}
    );
    return *this;
}

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
        pass.attachments.destroy();
        pass.setup.destroy();
        pass.execute.destroy();
    }

    data.passes.clear();
}

void RenderGraph::compile()
{
    for(Pass& pass : data.passes.iter())
    {
        PassBuilder builder{data.allocator};
        pass.setup.call(builder);
        pass.attachments.add_slice(builder.data.attachments.slice());
    }
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

    // backbuffer initial layout
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
        Array resolved_attachments = Array<GPU::AttachmentInfo>::with_size(data.allocator, pass.attachments.count);
        Vector2I offset = Vector2I();
        Vector2I extent = Vector2I();

        for(const PassAttachment& attachment : pass.attachments.iter())
        {
            // TODO: get texture view from pool
            GPU::TextureViewID texture_view = attachment.rt.is_backbuffer() ?
                frame_info.image_view : GPU::TextureViewID::invalid();

            if(attachment.rt.is_backbuffer())
            {
                extent = frame_info.image_size;
            }
            
            (void)resolved_attachments.add(
                {
                    .texture_view = texture_view,
                    .layout = GPU::TextureLayout::RenderAttachment,
                    .resolve_texture_view = GPU::TextureViewID::invalid(),
                    .resolve_layout = GPU::TextureLayout::Unknown,
                    .load_op = attachment.load_op,
                    .store_op = attachment.store_op,
                    // TODO: Handle custom clear values.
                    .clear_value = attachment.clear_value,
                }
            );
        }

        GPU::command_buffer_begin_renderpass(
            command_buffer,
            {
                .offset = offset,
                .extent = Vector3U(extent.x, extent.y, 1),
                .render_attachments = resolved_attachments.slice(),
                .depth_attachment = {},
                .stencil_attachment = {},
            }
        );

        GPU::Viewport viewport = GPU::Viewport::extent(extent.x, extent.y);
        GPU::Scissor scissor = GPU::Scissor::extent(extent.x, extent.y);

        GPU::command_buffer_set_viewports(command_buffer, 0, Slice(&viewport, 1));
        GPU::command_buffer_set_scissors(command_buffer, 0, Slice(&scissor, 1));

        pass.execute.call(resources);
        
        GPU::command_buffer_end_renderpass(command_buffer, {});

        resolved_attachments.destroy();
    }

    // backbuffer final layout
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