#pragma once
#include "collections/array.h"
#include "math/color.h"
#include "math/vec2.h"
#include "math/mat4.h"

#include "Core/RenderCore.hpp"


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

    struct BatchBlockInfo
    {
        Vector2 viewport_size;
    };

    struct InternalData
    {
        Mem::Allocator* allocator;

        GPU::PipelineLayoutID line_pipeline_layout;
        GPU::PipelineID line_pipeline;

        GPU::PipelineLayoutID triangle_pipeline_layout;
        GPU::PipelineID triangle_pipeline;

        Array<Batch> batches;
        usize current_batch;
    } data;

    GeometryBatch(Mem::Allocator* allocator, GPU::TextureFormat render_attachment_format);
    ~GeometryBatch();

    void begin(const BatchBlockInfo& info);
    void end();

    void draw_line(const Vector2& begin, const Vector2& end, const Color& color);

    Slice<Batch> build_batches();
};

}