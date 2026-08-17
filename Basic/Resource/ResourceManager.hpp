#pragma once
#include <Collections/Error.hpp>
#include <Collections/HashMap.hpp>
#include <Collections/StringMap.hpp>

#include "Basic/Core/GPUResourceManager.hpp"
#include "Basic/Resource/Resource.hpp"


namespace Basic
{
struct RenderDevice;
}

struct Image;
struct SpriteAnimation;
struct TileSet;
struct Texture;


struct ResourceManager
{
    DisableCopy(ResourceManager);
    DisableMove(ResourceManager);

    static constexpr usize DefaultFontSize = 32;
    
    struct ResourceAllocation
    {
        Resource* resource;
    };

    Mem::Allocator& allocator;
    Collections::StringMap<ResourceAllocation> resources;
    Basic::GPUResourceManager gpu_resource_manager;

    [[nodiscard]] Mem::Allocator& get_allocator() const { return allocator; }
    Basic::GPUResourceManager& get_gpu_resource_manager() { return gpu_resource_manager; }

    ResourceManager(Mem::Allocator& allocator, Basic::RenderDevice& render_device);
    ~ResourceManager();

    /*
    * Try to load the resource of the given type, can return nullptr
    */
    template<typename T>
    requires(!Core::IsSame<Resource, T> && Core::IsBaseOf<Resource, T>)
    [[nodiscard]] Collections::Result<T*, Error> try_load(Collections::StringView path)
    {
        Collections::Result<Resource*, Error> resource = load_resource(T::Type, T::Specification, path);
        if (resource)
        {
            return reinterpret_cast<T*>(resource.value());
        }

        return resource.error();
    };

    /*
    * Load the resource of the given type, can return nullptr.
    */
    template<typename T>
    requires(!Core::IsSame<Resource, T> && Core::IsBaseOf<Resource, T>)
    [[nodiscard]] T* load(Collections::StringView path)
    {
        return reinterpret_cast<T*>(load_resource(T::Type, T::Specification, path).value());
    };

    [[nodiscard]] Collections::Result<Resource*, Error> load_resource(ResourceType type,
        ResourceTypeSpecification specification, Collections::StringView path);

    [[nodiscard]] bool place_resource(Collections::StringView resource_name, Resource* resource);

    // Implementation
    template<typename T>
    requires(!Core::IsSame<Resource, T>)
    [[nodiscard]] T* _create_resource()
    {
        T* resource = get_allocator().object<T>(
            Resource::ResourceCreateInfo
            {
                .allocator = get_allocator(),
                .resource_type = T::Type,
                .resource_manager = *this,
            }
        );
        return resource;
    }

    [[nodiscard]] Collections::Result<Resource*, Error> _load_image(Collections::StringView path);
    [[nodiscard]] Collections::Result<Resource*, Error> _load_texture_2d(Collections::StringView path);
    [[nodiscard]] Collections::Result<Resource*, Error> _load_sound(Collections::StringView path);
    [[nodiscard]] Collections::Result<Resource*, Error> _load_font(Collections::StringView path);
};


