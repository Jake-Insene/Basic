#pragma once
#include <collections/array.h>
#include <gpu/gpu.h>
#include <math/color.h>
#include <math/vec2.h>
#include <math/mat4.h>
#include <math/rect_2d.h>
#include <math/transform_2d.h>


namespace Basic
{

/*
* Batches primitives per type.
* Currently supports: Line, Triangles.
*/
struct GeometryBatch
{
    DisableCopy(GeometryBatch);
    DisableMove(GeometryBatch);

    struct alignas(16) Vertex
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
        BatchBlock block;

        usize vb_offset;
        u32 vertex_count;
    };

    enum class RecordingState
    {
        // The batcher has begin recording.
        Begin,
        // The batcher has end recording. [Default]
        End,
    };

    struct InternalData
    {
        Mem::Allocator* allocator;

        GPU::PipelineLayoutID line_pipeline_layout;
        GPU::PipelineID line_pipeline;

        GPU::PipelineLayoutID triangle_pipeline_layout;
        GPU::PipelineID triangle_pipeline;

        Array<Vertex> vertices;
        Array<Batch> batches;
        GPU::PrimitiveTopology current_topology;

        BatchBlock block;
        RecordingState state;
    } data;

    /**
    * @param allocator Batcher allocator.
    * @param render_attachment_format Use to create the pipelines. This usually doesn't change often.
    */
    GeometryBatch(Mem::Allocator* allocator, GPU::TextureFormat render_attachment_format);
    ~GeometryBatch();

    void begin(Mat4 projection);
    void end();

    void draw_line_vertex(const Vertex& begin, const Vertex& end);
    void draw_triangle_vertex(const Vertex& v1, const Vertex& v2, const Vertex& v3);

    void draw_line(const Vector2& begin, const Vector2& end, const Color& color);
    
    void draw_triangle(const Vector2& v1, const Vector2& v2, const Vector2& v3, const Color& color);
    void draw_fill_triangle(const Vector2& v1, const Vector2& v2, const Vector2& v3, const Color& color);

    void draw_rectangle(const Rect2D& rect, const Color& color);
    void draw_fill_rectangle(const Rect2D& rect, const Color& color);
    void draw_fill_rectangle_transformed(const Rect2D& rect, const Transform2D& transform, const Color& color);

    Slice<const Batch> get_batches() const;
    Slice<const Vertex> get_vertices() const;

    void _try_begin_new_batch(GPU::PrimitiveTopology new_topology);
    void _set_topology(GPU::PrimitiveTopology new_topology);
};

}