#include "Basic/Templates/GraphicsApplication.hpp"


namespace Basic
{

GraphicsApplication::GraphicsApplication(const ApplicationAllocateInfo& alloc_info)
: Application(alloc_info),
data(alloc_info.allocator, window)
{
}

GraphicsApplication::~GraphicsApplication()
{
}

}