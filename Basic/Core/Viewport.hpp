#pragma once
#include "Core/RenderTarget.hpp"


namespace Basic
{

struct Viewport
{
    DisableCopy(Viewport);
    DisableMove(Viewport);

    struct InternalData
    {
        RenderTarget render_target;

        Vector2I viewport_size;
    } data;

    Viewport(GPU::TextureFormat viewport_format, const Vector2I& viewport_size);
    ~Viewport();

    RenderTarget& get_render_target() { return data.render_target; }
    Vector2I get_size() const { return data.viewport_size; }
};

}
