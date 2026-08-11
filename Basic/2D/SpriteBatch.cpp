#include "Basic/2D/SpriteBatch.hpp"

#include <engine/engine.h>
#include <graphics/shader.h>


namespace Basic
{

SpriteBatch::SpriteBatch(Mem::Allocator& allocator, GPU::TextureFormat render_attachment_format)
: allocator(allocator), vertices(allocator, 4, {}), batches(allocator, 4, {}),
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

    set_layout = GPU::descriptor_set_layout_create(Engine::get_render_device()->get_device(),
        GPU::DescriptorSetLayoutCreateInfo::create(set_bindings));

    Graphics::Shader sprite_shader{
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
        pipeline_layout = GPU::pipeline_layout_create(Engine::get_render_device()->get_device(),
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

        pipeline = GPU::pipeline_create(Engine::get_render_device()->get_device(),
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
        samplers[i] = GPU::sampler_create(Engine::get_render_device()->get_device(),
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

void SpriteBatch::draw_texture_gpu_transformed(const Rect2D& rect, const Transform2D& transform, const Rect2D& uv_rect,
    const Color& color, GPU::TextureViewID texture_view, const Vector2& texture_size, SpriteFilter filter)
{
    Rect2D normalized_uv = Rect2D(
        uv_rect.position / texture_size,
        uv_rect.size / texture_size
    );

    const Vector2 v1 = rect.position;
    const Vector2 v2 = rect.position + rect.size;
    const Vector2 v3 = rect.position + Vector2(rect.size.x, 0);
    const Vector2 v4 = rect.position + Vector2(0, rect.size.y);

    draw_triangle_vertex(
        {.position = transform * v1, .uv = normalized_uv.position + Vector2(0, normalized_uv.size.y), .color = color, },
        {.position = transform * v2, .uv = normalized_uv.position + Vector2(normalized_uv.size.x, 0), .color = color, },
        {.position = transform * v3, .uv = normalized_uv.position + normalized_uv.size, .color = color, },
        texture_view, filter
    );

    draw_triangle_vertex(
        {.position = transform * v1, .uv = normalized_uv.position + Vector2(0, normalized_uv.size.y), .color = color, },
        {.position = transform * v4, .uv = normalized_uv.position, .color = color, },
        {.position = transform * v2, .uv = normalized_uv.position + Vector2(normalized_uv.size.x, 0), .color = color, },
        texture_view, filter
    );
}

void SpriteBatch::draw_texture(const Rect2D& rect, const Rect2D& uv_rect, const Color& color,
    Texture2D* texture, SpriteFilter filter)
{
    if(texture == nullptr)
    {
        texture = Resource::load<Texture2D>("default:white_texture");
    }

    GPU::TextureViewID texture_view = Engine::get_gpu_resource_manager()->texture_get_texture_view(texture->texture_ref);
    draw_texture_gpu_transformed(rect, Transform2D(), uv_rect, color, texture_view, Vector2(texture->get_size()), filter);
}

void SpriteBatch::draw_texture_transformed(const Rect2D& rect, const Transform2D& transform, const Rect2D& uv_rect,
    const Color& color, Texture2D* texture, SpriteFilter filter)
{
        if(texture == nullptr)
    {
        texture = Resource::load<Texture2D>("default:white_texture");
    }

    GPU::TextureViewID texture_view = Engine::get_gpu_resource_manager()->texture_get_texture_view(texture->texture_ref);
    draw_texture_gpu_transformed(rect, transform, uv_rect, color, texture_view, Vector2(texture->get_size()), filter);
}

Slice<SpriteBatch::Batch> SpriteBatch::get_batches() const
{
    return batches.slice();
}

Slice<SpriteBatch::Vertex> SpriteBatch::get_vertices() const
{
    return vertices.slice();
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