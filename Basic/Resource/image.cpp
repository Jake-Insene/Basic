#include "Basic/Resource/Image.hpp"

#include <IO/File.hpp>
#include <external/stb_image.h>

#include "Basic/Resource/ResourceManagerInternal.hpp"


Image::Image(const ResourceCreateInfo& info)
: Resource(info)
{
    data.pixels = {};
    data.size = {};
    data.format = {};
}

Image::~Image()
{
    if(!data.pixels.null())
    {
        allocator.free(data.pixels);
    }
}

Error Image::load_from_path(Collections::StringView path)
{
    if (!IO::File::exists(allocator, path))
    {
        RMDebugInfo("Couldn't load the font '{}'", path);
        return MakeError(ErrorCode::FileNotFound);
    }

    Slice buffer = IO::File::read_all(allocator, path);
    
    i32 channels = 0;
    data.pixels.items = reinterpret_cast<u8*>(stbi_load_from_memory(
        buffer.ptr(), static_cast<int>(buffer.len), &data.size.width, &data.size.height, &channels, 0
    ));

    if(data.pixels.null())
    {
        return MakeError(ErrorCode::ImageCorrupted);
    }
    
    if(channels == 3)
    {
        data.format = ImageFormat::RGB8;
    }
    else if(channels == 4)
    {
        data.format = ImageFormat::RGBA8;
    }
    else
    {
        RMFatal("invalid channel count {}", channels);
    }
    
    data.pixels.len = isize(data.size.width * data.size.height * channels);
    allocator.free(buffer);
    
    return ErrorCode::Ok;
}

void Image::unload()
{
    if (!data.pixels.null())
    {
        allocator.free(data.pixels);
        data.pixels = {};
        data.size = {};
        data.format = {};
    }
}
