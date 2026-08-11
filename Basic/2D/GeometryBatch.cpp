#include "Basic/2D/GeometryBatch.hpp"

#include <engine/engine.h>
#include <graphics/shader.h>


namespace Basic
{

GeometryBatch::GeometryBatch(Mem::Allocator& allocator, GPU::TextureFormat render_attachment_format)
: allocator(allocator), vertices(allocator, 4, {}), batches(allocator, 4, {}),
current_topology(GPU::PrimitiveTopology::Unknown), state(RecordingState::End)
{
    const GPU::ConstantBlock shared_blocks[] =
    {
        GPU::ConstantBlock::create(GPU::ShaderStage::Vertex, 0, sizeof(BatchBlock)),
    };

    Graphics::Shader primitive_shader{allocator,
        {
            .path = "shaders/packages/2D/GeometryBatch.slang.spirv",
            .vertex_name = "VertexMain",
            .fragment_name = "FragmentMain",
        }
    };

    const GPU::ColorBlendAttachmentState color_blend_attachments[] =
    {
        GPU::ColorBlendAttachmentState::create(
            true, GPU::BlendFactor::SrcAlpha, GPU::BlendFactor::OneMinusSrcAlpha, GPU::BlendOp::Add,
            GPU::BlendFactor::One, GPU::BlendFactor::OneMinusSrcAlpha, GPU::BlendOp::Add,
            GPU::ColorComponentFlags(0xFF)
        ),
    };

    { // Line
        const GPU::VertexBinding vertex_bindings[] =
        {
            GPU::VertexBinding::create(0, sizeof(Vertex), GPU::InputRate::Vertex),
        };

        const GPU::VertexAttribute vertex_attributes[] =
        {
            GPU::VertexAttribute::create(0, 0, GPU::VertexFormat::RGBA32Float, 0),
        };

        line_pipeline_layout = GPU::pipeline_layout_create(Engine::get_render_device()->get_device(),
            GPU::PipelineLayoutCreateInfo::create(shared_blocks, {})
        );

        const GPU::TextureFormat pipeline_render_attachments[] =
        {
            render_attachment_format,
        };

        line_pipeline = GPU::pipeline_create(Engine::get_render_device()->get_device(),
            GPU::PipelineCreateInfo::create(
                GPU::PipelineBindPoint::Graphics,
                primitive_shader.get_stages(),
                GPU::VertexInput::create(vertex_bindings, vertex_attributes),
                GPU::InputAssembly::create(GPU::PrimitiveTopology::LineList),
                GPU::RasterizerState::state(GPU::PolygonMode::Fill, GPU::CullMode::Front, GPU::FrontFace::ClockWise),
                GPU::MultisampleState::disable(),
                GPU::DepthStencilState::depth_stencil_disable(),
                GPU::ColorBlendState::create(false, GPU::LogicOp::Copy, color_blend_attachments, Vector4()),
                line_pipeline_layout, GPU::RenderingInfo::render_attachments(pipeline_render_attachments)
            )
        );
    }

    { // Triangle
        const GPU::VertexBinding vertex_bindings[] =
        {
            GPU::VertexBinding::create(0, sizeof(Vertex), GPU::InputRate::Vertex),
        };

        const GPU::VertexAttribute vertex_attributes[] =
        {
            GPU::VertexAttribute::create(0, 0, GPU::VertexFormat::RGBA32Float, 0),
        };

        triangle_pipeline_layout = GPU::pipeline_layout_create(Engine::get_render_device()->get_device(),
            GPU::PipelineLayoutCreateInfo::create(shared_blocks, {})
        );

        const GPU::TextureFormat pipeline_render_attachments[] =
        {
            render_attachment_format,
        };

        triangle_pipeline = GPU::pipeline_create(Engine::get_render_device()->get_device(),
            GPU::PipelineCreateInfo::create(
                GPU::PipelineBindPoint::Graphics,
                primitive_shader.get_stages(),
                GPU::VertexInput::create(vertex_bindings, vertex_attributes),
                GPU::InputAssembly::create(GPU::PrimitiveTopology::TriangleList),
                GPU::RasterizerState::state(GPU::PolygonMode::Fill, GPU::CullMode::Front, GPU::FrontFace::ClockWise),
                GPU::MultisampleState::disable(),
                GPU::DepthStencilState::depth_stencil_disable(),
                GPU::ColorBlendState::create(false, GPU::LogicOp::Copy, color_blend_attachments, Vector4()),
                triangle_pipeline_layout, GPU::RenderingInfo::render_attachments(pipeline_render_attachments)
            )
        );
    }
}

GeometryBatch::~GeometryBatch()
{
    GPU::pipeline_destroy(line_pipeline);
    GPU::pipeline_layout_destroy(line_pipeline_layout);
    GPU::pipeline_destroy(triangle_pipeline);
    GPU::pipeline_layout_destroy(triangle_pipeline_layout);
}

void GeometryBatch::begin(Mat4 projection)
{
    DebugAssert(state == RecordingState::End, "batcher is still open");

    vertices.clear();
    batches.clear();
    current_topology = GPU::PrimitiveTopology::Unknown;
    state = RecordingState::Begin;

    block =
    {
        .projection = projection,
    };
}

void GeometryBatch::end()
{
    DebugAssert(state == RecordingState::Begin, "batcher is already end");
    state = RecordingState::End;
}

void GeometryBatch::draw_line_vertex(const Vertex& begin, const Vertex& end)
{
    _try_begin_new_batch(GPU::PrimitiveTopology::LineList);

    (void)vertices.add(begin);
    (void)vertices.add(end);
    batches.last().vertex_count += 2;
}

void GeometryBatch::draw_triangle_vertex(const Vertex& v1, const Vertex& v2, const Vertex& v3)
{
    _try_begin_new_batch(GPU::PrimitiveTopology::TriangleList);

    (void)vertices.add(v1);
    (void)vertices.add(v2);
    (void)vertices.add(v3);
    batches.last().vertex_count += 3;
}

void GeometryBatch::draw_line(const Vector2& begin, const Vector2& end, const Color& color)
{
    draw_line_vertex({.position = begin, .color = color}, {.position = end, .color = color});
}

void GeometryBatch::draw_triangle(const Vector2& v1, const Vector2& v2, const Vector2& v3, const Color& color)
{
    draw_line(v1, v2, color);
    draw_line(v2, v3, color);
    draw_line(v3, v1, color);
}

void GeometryBatch::draw_fill_triangle(const Vector2& v1, const Vector2& v2, const Vector2& v3, const Color& color)
{
    draw_triangle_vertex({.position = v1, .color = color},
        {.position = v2, .color = color}, {.position = v3, .color = color});
}

void GeometryBatch::draw_rectangle(const Rect2D& rect, const Color& color)
{
    draw_line(rect.position, rect.position + Vector2(rect.size.x, 0), color);
    draw_line(rect.position + Vector2(rect.size.x, 0), rect.position + rect.size, color);
    draw_line(rect.position + rect.size, rect.position + Vector2(0, rect.size.y), color);
    draw_line(rect.position + Vector2(0, rect.size.y), rect.position, color);
}

void GeometryBatch::draw_fill_rectangle(const Rect2D& rect, const Color& color)
{
    draw_fill_triangle(rect.position, rect.position + rect.size,
        rect.position + Vector2(rect.size.x, 0), color);

    draw_fill_triangle(rect.position, rect.position + Vector2(0, rect.size.y),
        rect.position + rect.size, color);
}

void GeometryBatch::draw_fill_rectangle_transformed(const Rect2D& rect, const Transform2D& transform,
    const Color& color)
{
    const Vector2 v1 = rect.position;
    const Vector2 v2 = rect.position + rect.size;
    const Vector2 v3 = rect.position + Vector2(rect.size.x, 0);
    const Vector2 v4 = rect.position + Vector2(0, rect.size.y);

    draw_fill_triangle(transform * v1, transform * v2, transform * v3, color);
    draw_fill_triangle(transform * v1, transform * v4, transform * v2, color);
}

Slice<const GeometryBatch::Batch> GeometryBatch::get_batches() const
{
    return batches.slice().as_const();
}

Slice<const GeometryBatch::Vertex> GeometryBatch::get_vertices() const
{
    return vertices.slice().as_const();
}

void GeometryBatch::_try_begin_new_batch(GPU::PrimitiveTopology topology)
{
    DebugAssert(state == RecordingState::Begin, "batcher is not open");
    if(current_topology != topology || batches.is_empty())
    {
        _set_topology(topology);
    }
}

void GeometryBatch::_set_topology(GPU::PrimitiveTopology new_topology)
{
    DebugAssert(new_topology != GPU::PrimitiveTopology::Unknown, "invalid topology");
    current_topology = new_topology;

    GPU::PipelineID pipeline = new_topology == GPU::PrimitiveTopology::LineList ?
        line_pipeline : triangle_pipeline;

    GPU::PipelineLayoutID pipeline_layout = new_topology == GPU::PrimitiveTopology::LineList ?
        line_pipeline_layout : triangle_pipeline_layout;

    Batch new_batch =
    {
        .pipeline = pipeline,
        .pipeline_layout = pipeline_layout,
        .block = block,
        .vb_offset = vertices.count,
        .vertex_count = 0,
    };

    (void)batches.add(new_batch);
}

}