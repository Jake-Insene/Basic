#include "Basic/2D/SpriteBatch.hpp"

#include <engine/engine.h>
#include <graphics/shader.h>


namespace Basic
{

SpriteBatch::SpriteBatch(Mem::Allocator* allocator, GPU::TextureFormat render_attachment_format)
{
    data.allocator = allocator;

    const GPU::ConstantBlock blocks[] =
    {
        GPU::ConstantBlock::create(GPU::ShaderStage::Vertex, 0, sizeof(BatchBlock)),
    };

    const GPU::DescriptorBinding set_bindings[] =
    {
        GPU::DescriptorBinding::combined_texture_sampler(0, 1, GPU::ShaderStage::Fragment),
    };

    data.set_layout = GPU::descriptor_set_layout_create(Engine::get_render_device()->get_device(),
        GPU::DescriptorSetLayoutCreateInfo::create(set_bindings));

    Graphics::Shader sprite_shader = {};
    sprite_shader.init(allocator,
        {
            .file_path = "shaders/packages/2D/SpriteBatch.slang.spirv",
            .vertex_name = "VertexMain",
            .fragment_name = "FragmentMain",
        }
    );

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

        data.pipeline_layout = GPU::pipeline_layout_create(Engine::get_render_device()->get_device(),
            GPU::PipelineLayoutCreateInfo::create(blocks, Slice(&data.set_layout, 1))
        );

        const GPU::ColorBlendAttachmentState color_blend_attachments[] =
        {
            GPU::ColorBlendAttachmentState::create(
                true, GPU::BlendFactor::SrcAlpha, GPU::BlendFactor::OneMinusSrcAlpha, GPU::BlendOp::Add,
                GPU::BlendFactor::One, GPU::BlendFactor::OneMinusSrcAlpha, GPU::BlendOp::Add,
                GPU::ColorComponentFlags(0xFF)
            ),
        };

        data.pipeline = GPU::pipeline_create(Engine::get_render_device()->get_device(),
            GPU::PipelineCreateInfo::create(
                GPU::PipelineBindPoint::Graphics,
                sprite_shader.get_stages(),
                GPU::VertexInput::create(vertex_bindings, vertex_attributes),
                GPU::InputAssembly::create(GPU::PrimitiveTopology::TriangleList),
                GPU::RasterizerState::state(GPU::PolygonMode::Fill, GPU::CullMode::Front, GPU::FrontFace::ClockWise),
                GPU::MultisampleState::disable(),
                GPU::DepthStencilState::depth_stencil_disable(),
                GPU::ColorBlendState::create(false, GPU::LogicOp::Copy, color_blend_attachments, Vector4()),
                data.pipeline_layout, GPU::RenderingInfo::render_attachments(Slice(&render_attachment_format, 1))
            )
        );
    }

    sprite_shader.destroy();

    data.vertices = Array<Vertex>::with_size(allocator, 4);
    data.batches = Array<Batch>::with_size(allocator, 4);
    data.current_texture_view = GPU::TextureViewID::invalid();
    data.current_filter = SpriteFilter::MaxCount;
    data.state = RecordingState::End;

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
        data.samplers[i] = GPU::sampler_create(Engine::get_render_device()->get_device(),
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
        GPU::sampler_destroy(data.samplers[i]);
    }

    GPU::pipeline_destroy(data.pipeline);
    GPU::pipeline_layout_destroy(data.pipeline_layout);
    GPU::descriptor_set_layout_destroy(data.set_layout);

    data.vertices.destroy();
    data.batches.destroy();
}

void SpriteBatch::begin(Mat4 projection)
{
    DebugAssert(data.state == RecordingState::End, "batcher is still open");

    data.vertices.clear();
    data.batches.clear();
    data.current_texture_view = GPU::TextureViewID::invalid();
    data.current_filter = SpriteFilter::MaxCount;
    data.state = RecordingState::Begin;

    data.block =
    {
        .projection = projection,
    };
}

void SpriteBatch::end()
{
    DebugAssert(data.state == RecordingState::Begin, "batcher is already end");
    data.state = RecordingState::End;
}

void SpriteBatch::draw_texture(const Rect2D& rect, const Rect2D& uv_rect, const Color& color,
    Texture2D* texture, SpriteFilter filter)
{
    DebugAssert(data.state == RecordingState::Begin, "batcher is not open");
    GPU::TextureViewID texture_view = Engine::get_render_device()->get_gpu_resource_manager()->texture_get_texture_view(texture->texture_ref);

    Rect2D normalized_uv = Rect2D(
        uv_rect.position / Vector2(texture->get_size()),
        uv_rect.size / Vector2(texture->get_size())
    );

    _bind_to_batch(texture_view, filter);

    const Vector2 uvs[] =
    {
        normalized_uv.position + Vector2(0, normalized_uv.size.y), // bottom-left
        normalized_uv.position + Vector2(normalized_uv.size.x, 0), // top-right
        normalized_uv.position + normalized_uv.size, // bottom-right
        normalized_uv.position + Vector2(0, normalized_uv.size.y), // bottom-left
        normalized_uv.position, // top-left
        normalized_uv.position + Vector2(normalized_uv.size.x, 0), // top-right
    };

    const Vector2 positions[] =
    {
        rect.position, // bottom-left
        rect.position + rect.size, // top-right
        rect.position + Vector2(rect.size.x, 0), // bottom-right
        rect.position, // bottom-left
        rect.position + Vector2(0, rect.size.y), // top-left
        rect.position + rect.size, // top-right
    };

    (void)data.vertices.add(Vertex{.position = positions[0], .uv = uvs[0], .color = color});
    (void)data.vertices.add(Vertex{.position = positions[1], .uv = uvs[1], .color = color});
    (void)data.vertices.add(Vertex{.position = positions[2], .uv = uvs[2], .color = color});
    (void)data.vertices.add(Vertex{.position = positions[3], .uv = uvs[3], .color = color});
    (void)data.vertices.add(Vertex{.position = positions[4], .uv = uvs[4], .color = color});
    (void)data.vertices.add(Vertex{.position = positions[5], .uv = uvs[5], .color = color});
    data.batches.last().vertex_count += 6;
}

Slice<SpriteBatch::Batch> SpriteBatch::get_batches() const
{
    return data.batches.slice();
}

Slice<SpriteBatch::Vertex> SpriteBatch::get_vertices() const
{
    return data.vertices.slice();
}

void SpriteBatch::_bind_to_batch(GPU::TextureViewID texture_view, SpriteFilter filter)
{
    DebugAssert(texture_view != GPU::TextureViewID::invalid(), "invalid texture view");
    DebugAssert(filter != SpriteFilter::MaxCount, "invalid filter");

    if(data.current_texture_view == texture_view && data.current_filter == filter)
    {
        return;
    }

    Batch new_batch =
    {
        .pipeline = data.pipeline,
        .set_layout = data.set_layout,
        .pipeline_layout = data.pipeline_layout,
        .block = data.block,
        .vb_offset = data.vertices.count,
        .vertex_count = 0,
        .texture =
        {
            .texture_view = texture_view,
            .layout = GPU::TextureLayout::ShaderReadOnly,
            .sampler = data.samplers[u32(filter)],
        },
    };

    data.current_texture_view = texture_view;
    data.current_filter = filter;

    (void)data.batches.add(new_batch);
}

}