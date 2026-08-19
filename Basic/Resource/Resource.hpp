#pragma once
#include <Collections/Error.hpp>
#include <Collections/Result.hpp>
#include <Collections/String.hpp>
#include <Collections/StringView.hpp>


namespace Basic
{

struct ResourceManager;

enum ResourceType
{
    RESOURCE_UNKNOWN = 0,
    RESOURCE_IMAGE,
    
    RESOURCE_TEXTURE,
    RESOURCE_TEXTURE_2D,
    RESOURCE_SOUND,
    RESOURCE_FONT,
 };

struct ResourceTypeSpecification
{
    bool LoadFromAssets = false;
    Collections::StringView Extensions = "";
};

enum class ResourceFlags : u32
{
    NoResourceFlags = 0,
    LoadFromAssets = 1,
};
EnableBitOp(ResourceFlags)

static constexpr ResourceTypeSpecification _construct_from_flags(ResourceFlags flags,
    Collections::StringView extensions)
{
    return ResourceTypeSpecification
    {
        .LoadFromAssets = Core::HasValue(flags & ResourceFlags::LoadFromAssets),
        .Extensions = extensions,
    };
}

#define RESOURCE(resource_type, flags, extensions) \
    static constexpr ResourceTypeSpecification Specification = _construct_from_flags(flags, extensions);\
    static constexpr ResourceType Type = resource_type;\

#define ResourceExtensions(extensions) extensions


/*
* A 'Resource' represents a collection of data that can be reused across the application.
*/
struct Resource
{
    DisableCopy(Resource);
    DisableMove(Resource);
    
    RESOURCE(
        RESOURCE_UNKNOWN,
        ResourceFlags::NoResourceFlags, 
        ResourceExtensions(""))

    struct ResourceCreateInfo
    {
        Mem::Allocator& allocator;
        ResourceType resource_type;
        ResourceManager& resource_manager;
    };
    
    Mem::Allocator& allocator;
    ResourceType type;
    Collections::String path;
    ResourceManager& resource_manager;
    
    Resource(const ResourceCreateInfo& info);
    virtual ~Resource();
};

}
