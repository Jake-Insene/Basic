#pragma once
#include "gpu/gpu.h"
#include "Math/vec2.h"

#include "Basic/Core/GPUResourceManager.hpp"
#include "Basic/Resource/Resource.hpp"
#include "Basic/Resource/Image.hpp"


namespace Basic
{

// A group of pixels allocated in the gpu.
struct Texture : Resource
{
    RESOURCE(RESOURCE_TEXTURE, ResourceFlags::LoadFromAssets, ResourceExtensions("png"))

    Basic::GPUTextureID texture_ref;
    GPU::TextureViewID texture_view;
    Vector2I size;
    
    Texture(const ResourceCreateInfo& info);
    virtual ~Texture() override;
    
    [[nodiscard]] Vector2I get_size() const;
};

// Same as Texture but specialized for 2D.
struct Texture2D : Texture
{
    RESOURCE(RESOURCE_TEXTURE_2D, ResourceFlags::LoadFromAssets, ResourceExtensions("png"))

    Texture2D(const ResourceCreateInfo& info) : Texture(info) {}

    Error load_from_path(Collections::StringView path);
    Error load_from_image(Image* image);
    Error load_from_raw(Image::ImageFormat image_format, const Vector2I& image_size, const Slice<u8>& pixels);
};

}
