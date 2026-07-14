#pragma once
#include <collections/array.h>
#include <math/color.h>
#include <math/vec2.h>
#include <math/mat4.h>

#include "Basic/Core/RenderCore.hpp"


namespace Basic
{

struct GeometryBatch
{
    DisableCopy(GeometryBatch);
    DisableMove(GeometryBatch);

    struct alignas(16) Primitive
    {
        Vector2 position;
        Color color;
    };

    struct BatchBlock
    {
        Mat4 projection;
    };

    struct Batch
    {
        GPU::PipelineID pipeline;
        GPU::PipelineLayoutID pipeline_layout;

        usize vb_offset;
        u32 vertex_count;
    };

    struct InternalData
    {
        Mem::Allocator* allocator;

        GPU::PipelineLayoutID line_pipeline_layout;
        GPU::PipelineID line_pipeline;

        GPU::PipelineLayoutID triangle_pipeline_layout;
        GPU::PipelineID triangle_pipeline;

        Array<Primitive> primitives;
        Array<Batch> batches;
        GPU::PrimitiveTopology current_topology;
    } data;

    GeometryBatch(Mem::Allocator* allocator, GPU::TextureFormat render_attachment_format);
    ~GeometryBatch();

    void reset();

    void draw_line(const Vector2& begin, const Vector2& end, const Color& color);
    void draw_triangle(const Vector2& v1, const Vector2& v2, const Vector2& v3, const Color& color);

    Slice<Batch> get_batches();
    Slice<Primitive> get_primitives();

    void _set_topology(GPU::PrimitiveTopology new_topology);
};

}