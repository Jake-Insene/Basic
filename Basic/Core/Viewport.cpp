#include "Core/Viewport.hpp"


namespace Basic
{

Viewport::Viewport(GPU::TextureFormat viewport_format, const Vector2I& viewport_size)
    : data{.render_target = RenderTarget(viewport_format, viewport_size), .viewport_size = viewport_size}
{
};

Viewport::~Viewport()
{
}

}
