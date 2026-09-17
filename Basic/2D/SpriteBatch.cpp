#include "Basic/2D/SpriteBatch.hpp"

#include "Basic/Core/Shader.hpp"
#include "Basic/Core/RenderGraph.hpp"


namespace Basic
{

static Vector2 _transform_around_point(const Transform2D& transform,
    const Vector2& point, const Vector2& pivot)
{
    Vector2 local = point - pivot;
    Vector2 transformed = transform * local;
    return transformed + pivot;
}

SpriteBatch::SpriteBatch(Mem::Allocator& allocator, RenderDevice& render_device,
    GPU::TextureViewID white_texture, GPU::TextureFormat render_attachment_format)
: allocator(allocator), white_texture(white_texture), vertices(allocator, 4, {}), batches(allocator, 4, {}),
current_texture_view(GPU::TextureViewID::invalid()), current_filter(SpriteFilter::MaxCount),
state(RecordingState::End)
{
    const GPU::ConstantBlock blocks[] =
    {
        GPU::ConstantBlock::create(GPU::ShaderStage::Vertex, 0, sizeof(BatchBlock)),
    };

    const GPU::DescriptorBinding set_bindings[] =
    {
        GPU::DescriptorBinding::combined_texture_sampler(0, 1, GPU::ShaderStage::Fragment),
    };

    set_layout = GPU::descriptor_set_layout_create(render_device.get_device(),
        GPU::DescriptorSetLayoutCreateInfo::create(set_bindings));

    Basic::Shader sprite_shader{
        allocator,
        {
            .path = "shaders/packages/2D/SpriteBatch.slang.spirv",
            .vertex_name = "VertexMain",
            .fragment_name = "FragmentMain",
        }
    };

    { // Sprite
        const GPU::VertexBinding vertex_bindings[] =
        {
            GPU::VertexBinding::create(0, sizeof(Vertex), GPU::InputRate::Vertex),
        };

        const GPU::VertexAttribute vertex_attributes[] =
        {
            GPU::VertexAttribute::create(0, 0, GPU::VertexFormat::RGBA32Float, 0),
            GPU::VertexAttribute::create(1, 0, GPU::VertexFormat::RGBA32Float, sizeof(Vector4)),
        };

        const GPU::DescriptorSetLayoutID pipeline_set_layouts[] =
        {
            set_layout
        };
        pipeline_layout = GPU::pipeline_layout_create(render_device.get_device(),
            GPU::PipelineLayoutCreateInfo::create(blocks, pipeline_set_layouts)
        );

        const GPU::ColorBlendAttachmentState color_blend_attachments[] =
        {
            GPU::ColorBlendAttachmentState::create(
                true, GPU::BlendFactor::SrcAlpha, GPU::BlendFactor::OneMinusSrcAlpha, GPU::BlendOp::Add,
                GPU::BlendFactor::One, GPU::BlendFactor::OneMinusSrcAlpha, GPU::BlendOp::Add,
                GPU::ColorComponentFlags(0xFF)
            ),
        };

        const GPU::TextureFormat pipeline_render_attachments[] =
        {
            render_attachment_format,
        };

        pipeline = GPU::pipeline_create(render_device.get_device(),
            GPU::PipelineCreateInfo::create(
                GPU::PipelineBindPoint::Graphics,
                sprite_shader.get_stages(),
                GPU::VertexInput::create(vertex_bindings, vertex_attributes),
                GPU::InputAssembly::create(GPU::PrimitiveTopology::TriangleList),
                GPU::RasterizerState::state(GPU::PolygonMode::Fill, GPU::CullMode::Front, GPU::FrontFace::ClockWise),
                GPU::MultisampleState::disable(),
                GPU::DepthStencilState::depth_stencil_disable(),
                GPU::ColorBlendState::create(false, GPU::LogicOp::Copy, color_blend_attachments, Vector4()),
                pipeline_layout, GPU::RenderingInfo::render_attachments(pipeline_render_attachments)
            )
        );
    }

    static constexpr GPU::Filter gpu_filters[] =
    {
        GPU::Filter::Linear,
        GPU::Filter::Nearest,
    };

    static constexpr GPU::SamplerMipMapMode gpu_mimap_modes[] =
    {
        GPU::SamplerMipMapMode::Linear,
        GPU::SamplerMipMapMode::Nearest,
    };

    for(usize i = 0; i < u32(SpriteFilter::MaxCount); i++)
    {
        samplers[i] = GPU::sampler_create(render_device.get_device(),
            GPU::SamplerCreateInfo::create(gpu_filters[i], gpu_filters[i],
                gpu_mimap_modes[i], GPU::SamplerAddressMode::Repeat, GPU::SamplerAddressMode::Repeat,
                GPU::SamplerAddressMode::Repeat, 0.F, false, 1.F, false, GPU::CompareOp::Always,
                0.F, 0.F));
    }
}

SpriteBatch::~SpriteBatch()
{
    for(usize i = 0; i < u32(SpriteFilter::MaxCount); i++)
    {
        GPU::sampler_destroy(samplers[i]);
    }

    GPU::pipeline_destroy(pipeline);
    GPU::pipeline_layout_destroy(pipeline_layout);
    GPU::descriptor_set_layout_destroy(set_layout);
}

void SpriteBatch::begin(Mat4 projection)
{
    DebugAssert(state == RecordingState::End, "batcher is still open");

    vertices.clear();
    batches.clear();
    current_texture_view = GPU::TextureViewID::invalid();
    current_filter = SpriteFilter::MaxCount;
    state = RecordingState::Begin;

    block =
    {
        .projection = projection,
    };
}

void SpriteBatch::end()
{
    DebugAssert(state == RecordingState::Begin, "batcher is already end");
    state = RecordingState::End;
}

void SpriteBatch::draw_triangle_vertex(const Vertex& v1, const Vertex& v2, const Vertex& v3,
    GPU::TextureViewID texture_view, SpriteFilter filter)
{
    DebugAssert(state == RecordingState::Begin, "batcher is not open");
    _bind_to_batch(texture_view, filter);

    (void)vertices.add(v1);
    (void)vertices.add(v2);
    (void)vertices.add(v3);
    batches.last().vertex_count += 3;
}

void SpriteBatch::draw_texture_transformed(const Rect2D& rect, const Transform2D& transform, const Vector2& pivot,
    const Rect2D& uv_rect, const Color& color, GPU::TextureViewID texture_view, const Vector2& texture_size,
    SpriteFilter filter)
{
    Rect2D normalized_uv = Rect2D(
        uv_rect.position / texture_size,
        uv_rect.size / texture_size
    );

    const Vector2 vp1 = rect.position;
    const Vector2 vp2 = rect.position + rect.size;
    const Vector2 vp3 = rect.position + Vector2(rect.size.x, 0);
    const Vector2 vp4 = rect.position + Vector2(0, rect.size.y);

    const Vertex v1 =
    {
        .position = _transform_around_point(transform, vp1, pivot),
        .uv = normalized_uv.position + Vector2(0, normalized_uv.size.y),
        .color = color, 
    };
    const Vertex v2 =
    {
        .position = _transform_around_point(transform, vp2, pivot),
        .uv = normalized_uv.position + Vector2(normalized_uv.size.x, 0),
        .color = color,
    };
    const Vertex v3 =
    {
        .position = _transform_around_point(transform, vp3, pivot),
        .uv = normalized_uv.position + normalized_uv.size,
        .color = color,
    };
    const Vertex v4 =
    {
        .position = _transform_around_point(transform, vp4, pivot),
        .uv = normalized_uv.position,
        .color = color,
    };

    draw_triangle_vertex(v1, v2, v3, texture_view, filter);
    draw_triangle_vertex(v1, v4, v2, texture_view, filter);
}

void SpriteBatch::draw_texture_transformed_pivot_centered(const Rect2D& rect, const Transform2D& transform,
    const Rect2D& uv_rect, const Color& color, GPU::TextureViewID texture_view, const Vector2& texture_size,
    SpriteFilter filter)
{
    draw_texture_transformed(rect, transform, rect.position + rect.size/2.F, uv_rect, color, texture_view,
        texture_size, filter);
}

Slice<SpriteBatch::Batch> SpriteBatch::get_batches() const
{
    return batches.slice();
}

Slice<SpriteBatch::Vertex> SpriteBatch::get_vertices() const
{
    return vertices.slice();
}

void SpriteBatch::submit_renderpass(const TransientAllocation sprite_transient, PassResources& resources)
{
    for(const SpriteBatch::Batch& batch : get_batches())
    {
        const GPU::DescriptorSetID set = resources.context.allocate_descriptor_set(batch.set_layout);
        
        const GPU::WriteDescriptorInfo write_infos[] =
        {
            GPU::WriteDescriptorInfo::combined_texture_sampler(set, 0, 0, Slice(&batch.texture, 1)),
        };

        GPU::descriptor_set_update_descriptors(resources.context.data.render_device.get_device(),
            { .write_infos = write_infos });
        
        GPU::command_buffer_bind_pipeline(resources.command_buffer,
            GPU::PipelineBindPoint::Graphics, batch.pipeline);

        GPU::command_buffer_bind_descriptor_sets(resources.command_buffer,
            GPU::PipelineBindPoint::Graphics, batch.pipeline_layout, 0, Slice(&set, 1));

        GPU::command_buffer_constant_block(resources.command_buffer,
            batch.pipeline_layout, GPU::ShaderStage::Vertex, 0, sizeof(BatchBlock),
            reinterpret_cast<MemoryAddress>(&batch.block));

        const GPU::BufferID vbs[] =
        {
            resources.global_device_vertex_buffer,
        };

        GPU::command_buffer_bind_vertex_buffers(resources.command_buffer,
            0, vbs,
            Slice(&sprite_transient.offset, 1));

        GPU::command_buffer_draw(resources.command_buffer,
            batch.vertex_count, 1, batch.vb_offset, 0);
    }
}

void SpriteBatch::_bind_to_batch(GPU::TextureViewID texture_view, SpriteFilter filter)
{
    DebugAssert(texture_view != GPU::TextureViewID::invalid(), "invalid texture view");
    DebugAssert(filter != SpriteFilter::MaxCount, "invalid filter");

    if(current_texture_view == texture_view && current_filter == filter)
    {
        return;
    }

    Batch new_batch =
    {
        .pipeline = pipeline,
        .set_layout = set_layout,
        .pipeline_layout = pipeline_layout,
        .block = block,
        .vb_offset = vertices.count,
        .vertex_count = 0,
        .texture =
        {
            .texture_view = texture_view,
            .layout = GPU::TextureLayout::ShaderReadOnly,
            .sampler = samplers[u32(filter)],
        },
    };

    current_texture_view = texture_view;
    current_filter = filter;

    (void)batches.add(new_batch);
}

}