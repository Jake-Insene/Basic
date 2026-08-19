#include "Basic/Resource/Resource.hpp"

#include "Basic/Resource/ResourceManager.hpp"


namespace Basic
{

Resource::Resource(const ResourceCreateInfo& info)
: allocator(info.allocator), type(info.resource_type), path(allocator, 0, {}),
resource_manager(info.resource_manager)
{
    allocator = info.allocator;
    type = info.resource_type;
}

Resource::~Resource()
{}

}
