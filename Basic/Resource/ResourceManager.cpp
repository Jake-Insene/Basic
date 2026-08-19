#include "Basic/Resource/ResourceManager.hpp"

#include <IO/File.hpp>

#include "Basic/Core/RenderDevice.hpp"
#include "Basic/Resource/ResourceManagerInternal.hpp"
#include "Basic/Resource/Font.hpp"
#include "Basic/Resource/Image.hpp"
#include "Basic/Resource/Texture.hpp"
#include "Basic/Resource/Sound.hpp"

#include <external/stb_image.h>


Basic::ResourceManager* current_rm;

namespace Basic
{

ResourceManager::ResourceManager(Mem::Allocator& allocator, Basic::RenderDevice& render_device)
: allocator(allocator), resources(allocator, 4),
gpu_resource_manager(
    {
        .allocator = allocator,
        .device = render_device.get_device(),
        .graphics_queue = render_device.get_graphics_queue(),
        .copy_queue = render_device.get_copy_queue(),
        .gpu_memory_allocator = render_device.get_gpu_memory_allocator(),
    }
)
{
    current_rm = this;

    // default resources
    u32 white = 0xFFFFFFFF;
    Texture2D* white_texture = _create_resource<Texture2D>();
    white_texture->path.set("default:white_texture");
    (void)white_texture->load_from_raw(Image::ImageFormat::RGBA8, Vector2I(1, 1), Mem::to_bytes(Slice(&white, 1)));

    (void)place_resource(
        "default:white_texture",
        white_texture
    );
}

ResourceManager::~ResourceManager()
{
    for(auto& it : resources.iter())
    {
        RMDebugInfo("Destroying the resource '{}'", it.first);
        Core::Mem::Destruct(*it.second.resource);
        allocator.free(
            Mem::to_bytes(Slice(it.second.resource, 1))
        );
    }
}

Collections::Result<Resource*, Error> ResourceManager::load_resource(ResourceType type,
    ResourceTypeSpecification, Collections::StringView path)
{
    switch (type)
    {
    case RESOURCE_IMAGE:
        return _load_image(path);
    case RESOURCE_TEXTURE:
        break;
    case RESOURCE_TEXTURE_2D:
        return _load_texture_2d(path);
        break;
    case RESOURCE_SOUND:
        return _load_sound(path);
        break;
    case RESOURCE_FONT:
        return _load_font(path);
        break;
    default:
        break;
    }

    return MakeError(ErrorCode::InvalidResourceType);
}


bool ResourceManager::place_resource(Collections::StringView resource_name, Resource* resource)
{
    if (resources.has(resource_name))
    {
        return false;
    }

    resources.insert(
        resource_name,
        {
            .resource = resource,
        }
    );
    return true;
}

Collections::Result<Resource*, Error> ResourceManager::_load_image(Collections::StringView path)
{
    Image* image = nullptr;
    if (resources.has(path))
    {
        image = reinterpret_cast<Image*>(resources.get(path).resource);
    }
    else
    {
        image = _create_resource<Image>();

        Error load_result = image->load_from_path(path);
        if (!load_result)
        {
            get_allocator().free(Mem::to_bytes(Slice(image, 1)));
            return load_result;
        }

        image->path.set(path);
        (void)place_resource(
            path,
            image
        );
    }

    return image;
}

Collections::Result<Resource*, Error> ResourceManager::_load_texture_2d(Collections::StringView path)
{
    Texture2D* tex = nullptr;
    if(resources.has(path))
    {
        tex = reinterpret_cast<Texture2D*>(resources.get(path).resource);
    }
    else
    {
        RMDebugInfo("Loading texture '{}'...", path);
        tex = _create_resource<Texture2D>();
        
        Error load_result = tex->load_from_path(path);
        if (!load_result)
        {
            get_allocator().free(Mem::to_bytes(Slice(tex, 1)));
            return load_result;
        }

        tex->path.set(path);
        (void)place_resource(
            path,
            tex
        );
    }
    
    return tex;
}

Collections::Result<Resource*, Error> ResourceManager::_load_sound(Collections::StringView path)
{
    if (resources.has(path))
    {
        return reinterpret_cast<Sound*>(resources.get(path).resource);
    }

    Sound* new_sound = _create_resource<Sound>();
    Error load_result = new_sound->load(path);
    if (!load_result)
    {
        get_allocator().free(Mem::to_bytes(Slice(new_sound, 1)));
        return load_result;
    }

    (void)place_resource(
        path,
        new_sound
    );
    return new_sound;
}

Collections::Result<Resource*, Error> ResourceManager::_load_font(Collections::StringView path)
{
    if (resources.has(path))
    {
        return reinterpret_cast<Font*>(resources.get(path).resource);
    }

    Font* new_font = _create_resource<Font>();
    Error load_result = new_font->load(path);
    if (!load_result)
    {
        get_allocator().free(Mem::to_bytes(Slice(new_font, 1)));
        return load_result;
    }

    (void)place_resource(
        path,
        new_font
    );
    return new_font;
}

}
