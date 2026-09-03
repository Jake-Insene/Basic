#pragma once
#include <Collections/Array.hpp>
#include <Math/color.h>
#include <Math/vec2.h>
#include <Math/mat4.h>
#include <Math/rect_2d.h>
#include <Math/transform_2d.h>

#include "Basic/Core/RenderCore.hpp"
#include "Basic/Core/RenderDevice.hpp"


namespace Basic
{

struct PassResources;

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
    
    Mem::Allocator& allocator;

    GPU::DescriptorSetLayoutID set_layout;

    GPU::PipelineLayoutID pipeline_layout;
    GPU::PipelineID pipeline;

    Collections::Array<Vertex> vertices;
    Collections::Array<Batch> batches;
    GPU::TextureViewID current_texture_view;
    SpriteFilter current_filter;

    BatchBlock block;
    RecordingState state;

    GPU::SamplerID samplers[u32(SpriteFilter::MaxCount)];
    
    SpriteBatch(Mem::Allocator& allocator, RenderDevice& render_device, GPU::TextureFormat render_attachment_format);
    ~SpriteBatch();

    void begin(Mat4 projection);
    void end();

    void draw_triangle_vertex(const Vertex& v1, const Vertex& v2, const Vertex& v3,
        GPU::TextureViewID texture_view, SpriteFilter filter);

    void draw_texture_transformed(const Rect2D& rect, const Transform2D& transform, const Vector2& pivot,
        const Rect2D& uv_rect, const Color& color, GPU::TextureViewID texture_view, const Vector2& texture_size, SpriteFilter filter);

    void draw_texture_transformed_pivot_centered(const Rect2D& rect, const Transform2D& transform,
        const Rect2D& uv_rect, const Color& color, GPU::TextureViewID texture_view, const Vector2& texture_size, SpriteFilter filter);

    Slice<Batch> get_batches() const;
    Slice<Vertex> get_vertices() const;

    void submit_renderpass(TransientAllocation sprite_transient, PassResources& resources);

    void _bind_to_batch(GPU::TextureViewID texture_view, SpriteFilter filter);
};

}