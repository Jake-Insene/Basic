#include "Basic/Resource/Texture.hpp"

#include "Basic/Core/RenderDevice.hpp"
#include "Basic/Resource/ResourceManagerInternal.hpp"
#include "Basic/Resource/ResourceManager.hpp"

#include <external/stb_image.h>

namespace Basic
{

Texture::Texture(const ResourceCreateInfo& info)
: Resource(info)
{
    texture_ref = Basic::GPUTextureID::invalid();
    texture_view = GPU::TextureViewID::invalid();
    size = Vector2I();
}

Texture::~Texture()
{
    resource_manager.get_gpu_resource_manager().destroy_texture(texture_ref);
}

Vector2I Texture::get_size() const
{
    return size;
}

Error Texture2D::load_from_path(Collections::StringView path)
{
    if (!IO::File::exists(allocator, path))
    {
        RMDebugInfo("Couldn't load the font '{}'", path);
        return MakeError(ErrorCode::FileNotFound);
    }

    Slice buffer = IO::File::read_all(allocator, path);
    
    i32 channels = 0;
    Slice<u8> pixels;
    pixels.items = reinterpret_cast<u8*>(stbi_load_from_memory(
        buffer.ptr(), static_cast<int>(buffer.len), &size.width, &size.height, &channels, 0
    ));

    if(pixels.null())
    {
        return MakeError(ErrorCode::ImageCorrupted);
    }
    
    Image::ImageFormat image_format;
    if(channels == 3)
    {
        image_format = Image::ImageFormat::RGB8;
    }
    else if(channels == 4)
    {
        image_format = Image::ImageFormat::RGBA8;
    }
    else
    {
        RMFatal("invalid channel count {}", channels);
    }
    
    pixels.len = isize(size.width * size.height * channels);
    allocator.free(buffer);

    ErrorCode result = load_from_raw(image_format, size, pixels);
    allocator.free(pixels);
    
    return result;
}

Error Texture2D::load_from_image(Image* image)
{
    return load_from_raw(image->get_format(), image->get_size(), image->get_raw_pixels());
}

Error Texture2D::load_from_raw(Image::ImageFormat image_format, const Vector2I& image_size, const Slice<u8>& pixels)
{
    Basic::GPUResourceManager::TextureAllocateInfo create_info =
    {
        .type = GPU::TextureType::Texture2D,
        .format = image_format == Image::ImageFormat::RGB8 ? GPU::TextureFormat::RGB8Srgb : GPU::TextureFormat::RGBA8Srgb,
        .extent = Vector3U(image_size.width, image_size.height, 1),
        .pixels = pixels,
        .flags = Basic::TextureAllocateFlags(),
    };

    if(texture_ref != Basic::GPUTextureID::invalid())
    {
        resource_manager.get_gpu_resource_manager().destroy_texture(texture_ref);
    }
    
    texture_ref = resource_manager.get_gpu_resource_manager().create_texture(create_info);
    texture_view = resource_manager.get_gpu_resource_manager().texture_get_texture_view(texture_ref);
    size = image_size;

    return ErrorCode::Ok;
}

}
