#pragma once
#include <collections/array.h>
#include <math/color.h>
#include <math/vec2.h>
#include <math/mat4.h>
#include <math/rect_2d.h>
#include <resource/texture.h>

#include "Basic/Core/RenderCore.hpp"


struct Texture2D;

namespace Basic
{

enum class SpriteFilter
{
    Linear,
    Nearest,

    MaxCount,
};

/*
* Batches quads per texture and filter.
* Currently supports: Triangle texture.
*/
struct SpriteBatch
{
    DisableCopy(SpriteBatch);
    DisableMove(SpriteBatch);

    struct alignas(16) Vertex
    {
        Vector2 position;
        Vector2 uv;
        Color color;
    };

    struct BatchBlock
    {
        Mat4 projection;
    };

    struct Batch
    {
        GPU::PipelineID pipeline;
        GPU::DescriptorSetLayoutID set_layout;
        GPU::PipelineLayoutID pipeline_layout;
        BatchBlock block;

        usize vb_offset;
        u32 vertex_count;

        GPU::DescriptorTextureInfo texture;
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

        GPU::DescriptorSetLayoutID set_layout;

        GPU::PipelineLayoutID pipeline_layout;
        GPU::PipelineID pipeline;

        Array<Vertex> vertices;
        Array<Batch> batches;
        GPU::TextureViewID current_texture_view;
        SpriteFilter current_filter;

        BatchBlock block;
        RecordingState state;

        GPU::SamplerID samplers[u32(SpriteFilter::MaxCount)];
    } data;
    
    SpriteBatch(Mem::Allocator* allocator, GPU::TextureFormat render_attachment_format);
    ~SpriteBatch();

    void begin(Mat4 projection);
    void end();

    void draw_texture(const Rect2D& rect, const Rect2D& uv_rect, const Color& color,
        Texture2D* texture, SpriteFilter filter);

    Slice<Batch> get_batches();
    Slice<Vertex> get_vertices();

    void _bind_to_batch(GPU::TextureViewID texture_view, SpriteFilter filter);
};

}