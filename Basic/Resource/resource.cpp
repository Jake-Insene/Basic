#include "Basic/Resource/resource.h"

#include "Basic/Resource/resource_manager.h"


Resource::Resource(const ResourceCreateInfo& info)
: allocator(info.allocator), type(info.resource_type), path(allocator, 0, {}),
resource_manager(info.resource_manager)
{
    allocator = info.allocator;
    type = info.resource_type;
}

Resource::~Resource()
{}
