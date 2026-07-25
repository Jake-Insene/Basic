#include "Basic/2D/GeometryBatch.hpp"

#include <engine/engine.h>
#include <graphics/shader.h>


namespace Basic
{

GeometryBatch::GeometryBatch(Mem::Allocator* allocator, GPU::TextureFormat render_attachment_format) : data{}
{
    data.allocator = allocator;

    const GPU::ConstantBlock shared_blocks[] =
    {
        GPU::ConstantBlock::create(GPU::ShaderStage::Vertex, 0, sizeof(BatchBlock)),
    };

    Graphics::Shader primitive_shader = {};
    primitive_shader.init(allocator,
        {
            .file_path = "shaders/packages/2D/GeometryBatch.slang.spirv",
            .vertex_name = "VertexMain",
            .fragment_name = "FragmentMain",
        }
    );

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

        data.line_pipeline_layout = GPU::pipeline_layout_create(Engine::get_render_device()->get_device(),
            GPU::PipelineLayoutCreateInfo::create(shared_blocks, {})
        );

        data.line_pipeline = GPU::pipeline_create(Engine::get_render_device()->get_device(),
            GPU::PipelineCreateInfo::create(
                GPU::PipelineBindPoint::Graphics,
                primitive_shader.get_stages(),
                GPU::VertexInput::create(vertex_bindings, vertex_attributes),
                GPU::InputAssembly::create(GPU::PrimitiveTopology::LineList),
                GPU::RasterizerState::state(GPU::PolygonMode::Fill, GPU::CullMode::Front, GPU::FrontFace::ClockWise),
                GPU::MultisampleState::disable(),
                GPU::DepthStencilState::depth_stencil_disable(),
                GPU::ColorBlendState::create(false, GPU::LogicOp::Copy, color_blend_attachments, Vector4()),
                data.line_pipeline_layout, GPU::RenderingInfo::render_attachments(Slice(&render_attachment_format, 1))
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

        data.triangle_pipeline_layout = GPU::pipeline_layout_create(Engine::get_render_device()->get_device(),
            GPU::PipelineLayoutCreateInfo::create(shared_blocks, {})
        );

        data.triangle_pipeline = GPU::pipeline_create(Engine::get_render_device()->get_device(),
            GPU::PipelineCreateInfo::create(
                GPU::PipelineBindPoint::Graphics,
                primitive_shader.get_stages(),
                GPU::VertexInput::create(vertex_bindings, vertex_attributes),
                GPU::InputAssembly::create(GPU::PrimitiveTopology::TriangleList),
                GPU::RasterizerState::state(GPU::PolygonMode::Fill, GPU::CullMode::Front, GPU::FrontFace::ClockWise),
                GPU::MultisampleState::disable(),
                GPU::DepthStencilState::depth_stencil_disable(),
                GPU::ColorBlendState::create(false, GPU::LogicOp::Copy, color_blend_attachments, Vector4()),
                data.triangle_pipeline_layout, GPU::RenderingInfo::render_attachments(Slice(&render_attachment_format, 1))
            )
        );
    }

    primitive_shader.destroy();

    data.vertices = Array<Vertex>::with_size(allocator, 4);
    data.batches = Array<Batch>::with_size(allocator, 4);
    data.current_topology = GPU::PrimitiveTopology::Unknown;
    data.state = RecordingState::End;
}

GeometryBatch::~GeometryBatch()
{
    GPU::pipeline_destroy(data.line_pipeline);
    GPU::pipeline_layout_destroy(data.line_pipeline_layout);
    GPU::pipeline_destroy(data.triangle_pipeline);
    GPU::pipeline_layout_destroy(data.triangle_pipeline_layout);

    data.vertices.destroy();
    data.batches.destroy();
}

void GeometryBatch::begin(Mat4 projection)
{
    DebugAssert(data.state == RecordingState::End, "batcher is still open");

    data.vertices.clear();
    data.batches.clear();
    data.current_topology = GPU::PrimitiveTopology::Unknown;
    data.state = RecordingState::Begin;

    data.block =
    {
        .projection = projection,
    };
}

void GeometryBatch::end()
{
    DebugAssert(data.state == RecordingState::Begin, "batcher is already end");
    data.state = RecordingState::End;
}

void GeometryBatch::draw_line_vertex(const Vertex& begin, const Vertex& end)
{
    _try_begin_new_batch(GPU::PrimitiveTopology::LineList);

    (void)data.vertices.add(begin);
    (void)data.vertices.add(end);
    data.batches.last().vertex_count += 2;
}

void GeometryBatch::draw_triangle_vertex(const Vertex& v1, const Vertex& v2, const Vertex& v3)
{
    _try_begin_new_batch(GPU::PrimitiveTopology::TriangleList);

    (void)data.vertices.add(v1);
    (void)data.vertices.add(v2);
    (void)data.vertices.add(v3);
    data.batches.last().vertex_count += 3;
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

Slice<const GeometryBatch::Batch> GeometryBatch::get_batches() const
{
    return data.batches.slice();
}

Slice<const GeometryBatch::Vertex> GeometryBatch::get_vertices() const
{
    return data.vertices.slice();
}

void GeometryBatch::_try_begin_new_batch(GPU::PrimitiveTopology topology)
{
    DebugAssert(data.state == RecordingState::Begin, "batcher is not open");
    if(data.current_topology != topology || data.batches.is_empty())
    {
        _set_topology(topology);
    }
}

void GeometryBatch::_set_topology(GPU::PrimitiveTopology new_topology)
{
    DebugAssert(new_topology != GPU::PrimitiveTopology::Unknown, "invalid topology");
    data.current_topology = new_topology;

    GPU::PipelineID pipeline = new_topology == GPU::PrimitiveTopology::LineList ?
        data.line_pipeline : data.triangle_pipeline;

    GPU::PipelineLayoutID pipeline_layout = new_topology == GPU::PrimitiveTopology::LineList ?
        data.line_pipeline_layout : data.triangle_pipeline_layout;

    Batch new_batch =
    {
        .pipeline = pipeline,
        .pipeline_layout = pipeline_layout,
        .block = data.block,
        .vb_offset = data.vertices.count,
        .vertex_count = 0,
    };

    (void)data.batches.add(new_batch);
}

}