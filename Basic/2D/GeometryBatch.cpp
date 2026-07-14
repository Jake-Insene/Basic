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
            .file_path = "shaders/bread/2D/PrimitiveBatch.slang.spirv",
            .vertex_name = "VertexMain",
            .fragment_name = "FragmentMain",
        }
    );

    { // Line
        const GPU::VertexBinding vertex_bindings[] =
        {
            GPU::VertexBinding::create(0, sizeof(Primitive), GPU::InputRate::Vertex),
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
                data.line_pipeline_layout, GPU::RenderingInfo::render_attachments(Slice(&render_attachment_format, 1))
            )
        );
    }

    { // Triangle
        const GPU::VertexBinding vertex_bindings[] =
        {
            GPU::VertexBinding::create(0, sizeof(Primitive), GPU::InputRate::Vertex),
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
                data.triangle_pipeline_layout, GPU::RenderingInfo::render_attachments(Slice(&render_attachment_format, 1))
            )
        );
    }

    primitive_shader.destroy();

    data.primitives = Array<Primitive>::with_size(allocator, 4);
    data.batches = Array<Batch>::with_size(allocator, 4);
    data.current_topology = GPU::PrimitiveTopology::Unknown;
}

GeometryBatch::~GeometryBatch()
{
    GPU::pipeline_layout_destroy(data.line_pipeline_layout);
    GPU::pipeline_destroy(data.line_pipeline);
    GPU::pipeline_layout_destroy(data.triangle_pipeline_layout);
    GPU::pipeline_destroy(data.triangle_pipeline);

    data.primitives.destroy();
    data.batches.destroy();
}

void GeometryBatch::reset()
{
    data.primitives.clear();
    data.batches.clear();
    data.current_topology = GPU::PrimitiveTopology::Unknown;
}

void GeometryBatch::draw_line(const Vector2& begin, const Vector2& end, const Color& color)
{
    if(data.current_topology != GPU::PrimitiveTopology::LineList || data.batches.is_empty())
    {
        _set_topology(GPU::PrimitiveTopology::LineList);
    }

    (void)data.primitives.add(Primitive{.position = begin, .color = color});
    (void)data.primitives.add(Primitive{.position = end, .color = color});
    data.batches.last().vertex_count += 2;
}

void GeometryBatch::draw_triangle(const Vector2& v1, const Vector2& v2, const Vector2& v3, const Color& color)
{
    if(data.current_topology != GPU::PrimitiveTopology::TriangleList || data.batches.is_empty())
    {
        _set_topology(GPU::PrimitiveTopology::TriangleList);
    }

    (void)data.primitives.add(Primitive{.position = v1, .color = color});
    (void)data.primitives.add(Primitive{.position = v2, .color = color});
    (void)data.primitives.add(Primitive{.position = v3, .color = color});
    data.batches.last().vertex_count += 3;
}

Slice<GeometryBatch::Batch> GeometryBatch::build_batches()
{
    return data.batches.slice();
}

void GeometryBatch::_set_topology(GPU::PrimitiveTopology new_topology)
{
    data.current_topology = new_topology;

    Batch new_batch =
    {
        .pipeline = data.line_pipeline,
        .pipeline_layout = data.line_pipeline_layout,
        .vb_offset = data.primitives.count,
        .vertex_count = 0,
    };

    (void)data.batches.add(new_batch);
}

}