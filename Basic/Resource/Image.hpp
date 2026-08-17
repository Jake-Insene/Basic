#pragma once
#include <Collections/Error.hpp>
#include <math/vec2.h>

#include "Basic/Resource/Resource.hpp"


/*
* Represents a collection of pixels in memory.
*/
struct Image : Resource
{
    RESOURCE(RESOURCE_IMAGE, ResourceFlags::LoadFromAssets, ResourceExtensions("png"))

    enum class ImageFormat
    {
        Unknown = 0,
        RGB8,
        RGBA8,
    };

    struct InternalData
    {
        Slice<u8> pixels;
        Vector2I size;
        ImageFormat format;
    } data;
    
    Image(const ResourceCreateInfo& info);
    virtual ~Image() override;
    
    Error load_from_path(Collections::StringView path);
    void unload();

    Slice<u8> get_raw_pixels() const { return data.pixels; }
    Vector2I get_size() const { return data.size; }
    ImageFormat get_format() const { return data.format; }
};

