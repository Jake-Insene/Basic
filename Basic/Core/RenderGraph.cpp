#include "Basic/Core/RenderGraph.hpp"

#include <os/os.h>


namespace Basic
{

PassBuilder::PassBuilder(Mem::Allocator* allocator) :
data{
    .allocator = allocator,
    .writes = Array<PassWriteAttachment>::with_allocator(allocator),
    .textures = Array<PassTexture>::with_allocator(allocator),
}
{
}

PassBuilder::~PassBuilder()
{
    data.writes.destroy();
    data.textures.destroy();
}

PassBuilder& PassBuilder::write_render_attachment(RenderTargetHandle rt, GPU::LoadOp load_op,
    GPU::StoreOp store_op, GPU::ClearValue clear_value)
{
    (void)data.writes.add(
        {.rt = rt, .load_op = load_op, .store_op = store_op, .clear_value = clear_value}
    );
    return *this;
}

PassBuilder& PassBuilder::read_texture(RenderTargetHandle texture, GPU::ShaderStage stage)
{
    (void)data.textures.add(
        {.texture = texture, .stage = stage}
    );
    return *this;
}

void PassBuilder::clear()
{
    data.writes.clear();
    data.textures.clear();
}

RenderGraph::RenderGraph(Mem::Allocator* allocator, Graphics::RenderDevice* render_device) :
data{
    .allocator = allocator,
    .render_device = render_device,
    .frame_context = {render_device, render_device, render_device},
    .passes = Array<Pass>::with_size(allocator, 4),
    .virtual_render_targets = Array<VirtualRenderTarget>::with_allocator(allocator),
    .tmp_allocator = {},
}
{
    data.tmp_allocator.init(OS::map_memory(MiB(1), OS::MapAccess::ReadWrite));
}

RenderGraph::~RenderGraph()
{
    OS::unmap_memory(data.tmp_allocator.sp);

    clear();
    
    data.passes.destroy();
    data.virtual_render_targets.destroy();
}

FrameContext& RenderGraph::get_frame_context(const FrameInfo& frame_info)
{
    return data.frame_context[frame_info.frame_index];
}

RenderTargetHandle RenderGraph::import_render_target(GPU::TextureID texture, GPU::TextureViewID texture_view,
    const Vector2I& extent)
{
    RenderTargetHandle handle = data.virtual_render_targets.count;

    (void)data.virtual_render_targets.add({
        .handle = handle,
        .extent = extent,
        .format = GPU::TextureFormat::Unknown,
        .texture = texture,
        .texture_view = texture_view,
        .imported = true
    });

    return handle;
}

void RenderGraph::clear()
{
    for(Pass& pass : data.passes.iter())
    {
        pass.writes.destroy();
        pass.textures.destroy();
        pass.setup.destroy();
        pass.execute.destroy();
    }

    data.passes.clear();
    data.virtual_render_targets.clear();
}

void RenderGraph::compile()
{
    PassBuilder builder{data.allocator};
    for(Pass& pass : data.passes.iter())
    {
        pass.setup.call(builder);
        pass.writes.add_slice(builder.data.writes.slice());
        pass.textures.add_slice(builder.data.textures.slice());
        builder.clear();
    }
}

void RenderGraph::execute(GPU::CommandBufferID command_buffer, const FrameInfo& frame_info)
{
    FrameContext& context =  get_frame_context(frame_info);
    context.syncronize_memory(command_buffer);

    data.tmp_allocator.reset();
    Mem::Allocator* allocator = &data.tmp_allocator;
    
    Array resolved_render_targets = Array<GPU::TextureViewID>::with_size(
        data.allocator, data.virtual_render_targets.count);
    for(VirtualRenderTarget& vrt : data.virtual_render_targets.iter())
    {
        (void)resolved_render_targets.add(vrt.texture_view);
    }
    
    PassResources resources =
    {
        .command_buffer = command_buffer,
        .global_device_vertex_buffer = context.get_transient_vertex_buffer_local(),
        .resolved_render_targets = resolved_render_targets.slice(),
        .context = context,
    };

    _begin_backbuffer(command_buffer, frame_info);

    for(Pass& pass : data.passes.iter())
    {
        _execute_pass(allocator, command_buffer, pass, resources, frame_info);
    }

    resolved_render_targets.destroy();

    _end_backbuffer(command_buffer, frame_info);
}

void RenderGraph::_begin_backbuffer(GPU::CommandBufferID command_buffer, const FrameInfo& frame_info)
{
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
}

void RenderGraph::_end_backbuffer(GPU::CommandBufferID command_buffer, const FrameInfo& frame_info)
{
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

Vector2I RenderGraph::_resolve_extent_for_pass(PassResources&, Pass& pass, const FrameInfo& frame_info)
{
    Vector2I extent = Vector2I();

    for(const PassWriteAttachment& write_attachment : pass.writes.iter())
    {
        if(write_attachment.rt.is_backbuffer())
        {
            extent = frame_info.image_size;
        }
        else
        {
            extent = data.virtual_render_targets.get(write_attachment.rt.id).extent;
        }
    }

    return extent;
}

Slice<GPU::AttachmentInfo> RenderGraph::_resolve_attachments_for_pass(Mem::Allocator* allocator, 
    PassResources& resources, Pass& pass, const FrameInfo& frame_info)
{
    Slice<GPU::AttachmentInfo> resolved_attachments = allocator->array<GPU::AttachmentInfo>(
        pass.writes.count);

    for(usize i = 0; i < pass.writes.count; i ++)
    {
        const PassWriteAttachment& attachment = pass.writes.get(i);
        // TODO: get texture view from pool
        GPU::TextureViewID texture_view = attachment.rt.is_backbuffer() ?
            frame_info.image_view : resources.get_render_target_view(attachment.rt);

        resolved_attachments[i] =
        {
            .texture_view = texture_view,
            .layout = GPU::TextureLayout::RenderAttachment,
            .resolve_texture_view = GPU::TextureViewID::invalid(),
            .resolve_layout = GPU::TextureLayout::Unknown,
            .load_op = attachment.load_op,
            .store_op = attachment.store_op,
            .clear_value = attachment.clear_value,
        };
    }

    return resolved_attachments;
}

Slice<GPU::PipelineTextureBarrier> RenderGraph::_resolve_begin_barriers_for_pass(Mem::Allocator* allocator,
    PassResources&, Pass& pass, const FrameInfo&)
{
    // do not put barries for the backbuffer attachment
    usize barrier_count = 0;
    for(const PassWriteAttachment& attachment : pass.writes.iter())
    {
        if(attachment.rt.is_backbuffer())
        {
            continue;
        }
        barrier_count++;
    }
    Slice begin_barriers = allocator->array<GPU::PipelineTextureBarrier>(barrier_count + pass.textures.count);

    usize barrier_i = 0;
    for(const PassWriteAttachment& attachment : pass.writes.iter())
    {
        if(attachment.rt.is_backbuffer())
        {
            continue;
        }

        begin_barriers[barrier_i] = GPU::PipelineTextureBarrier
        {
            .src_masks = GPU::AccessMasks::RenderAttachmentWrite,
            .dest_masks = GPU::AccessMasks::RenderAttachmentWrite,
            .src_layout = GPU::TextureLayout::Unknown,
            .dest_layout = GPU::TextureLayout::RenderAttachment,
            .texture = data.virtual_render_targets.get(attachment.rt.id).texture,
            .subresource_range = GPU::TextureSubresourceRange::color(0, 1, 0, 1),
        };
        barrier_i++;
    }

    for(const PassTexture& texture : pass.textures.iter())
    {
        begin_barriers[barrier_i] = GPU::PipelineTextureBarrier
        {
            .src_masks = GPU::AccessMasks::RenderAttachmentWrite,
            .dest_masks = GPU::AccessMasks::RenderAttachmentWrite,
            .src_layout = GPU::TextureLayout::RenderAttachment,
            .dest_layout = GPU::TextureLayout::ShaderReadOnly,
            .texture = data.virtual_render_targets.get(texture.texture.id).texture,
            .subresource_range = GPU::TextureSubresourceRange::color(0, 1, 0, 1),
        };
        barrier_i++;
    }

    return begin_barriers.slice(barrier_i);
}

Slice<GPU::PipelineTextureBarrier> RenderGraph::_resolve_end_barriers_for_pass(Mem::Allocator* allocator,
    PassResources&, Pass& pass, const FrameInfo&)
{
    // do not put barries for the backbuffer attachment
    usize barrier_count = 0;
    for(const PassWriteAttachment& attachment : pass.writes.iter())
    {
        if(attachment.rt.is_backbuffer())
        {
            continue;
        }
        barrier_count++;
    }
    Slice end_barriers = allocator->array<GPU::PipelineTextureBarrier>(barrier_count + pass.textures.count);

    usize barrier_i = 0;
    for(const PassWriteAttachment& attachment : pass.writes.iter())
    {
        if(attachment.rt.is_backbuffer())
        {
            continue;
        }

        end_barriers[barrier_i] = GPU::PipelineTextureBarrier
        {
            .src_masks = GPU::AccessMasks::RenderAttachmentWrite,
            .dest_masks = GPU::AccessMasks::RenderAttachmentWrite,
            .src_layout = GPU::TextureLayout::RenderAttachment,
            .dest_layout = GPU::TextureLayout::RenderAttachment,
            .texture = data.virtual_render_targets.get(attachment.rt.id).texture,
            .subresource_range = GPU::TextureSubresourceRange::color(0, 1, 0, 1),
        };
        barrier_i++;
    }

    return end_barriers.slice(barrier_i);
}

void RenderGraph::_execute_pass(Mem::Allocator* allocator, GPU::CommandBufferID command_buffer,
    Pass& pass, PassResources& resources, const FrameInfo& frame_info)
{
    Vector2I offset = Vector2I();
    Vector2I extent = _resolve_extent_for_pass(resources, pass, frame_info);

    Slice resolved_attachments = _resolve_attachments_for_pass(allocator, resources, pass, frame_info);
    Slice begin_barriers = _resolve_begin_barriers_for_pass(allocator, resources, pass, frame_info);
    Slice end_barriers = _resolve_end_barriers_for_pass(allocator, resources, pass, frame_info);

    GPU::command_buffer_pipeline_barrier(command_buffer,
        GPU::PipelineBarrier::texture_barrier(
            GPU::PipelineStages::RenderOutput,
            GPU::PipelineStages::RenderOutput,
            begin_barriers
        )
    );
    
    GPU::command_buffer_begin_renderpass(
        command_buffer,
        {
            .offset = offset,
            .extent = Vector3U(extent.x, extent.y, 1),
            .render_attachments = resolved_attachments,
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
    GPU::command_buffer_pipeline_barrier(command_buffer,
        GPU::PipelineBarrier::texture_barrier(
            GPU::PipelineStages::RenderOutput,
            GPU::PipelineStages::RenderOutput,
            end_barriers
        )
    );
}

}