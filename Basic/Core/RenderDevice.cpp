#include "Basic/Core/RenderDevice.hpp"


namespace Basic
{

static GPU::PhysicalDeviceID _select_physical_device()
{
    Slice physical_devices = GPU::physical_devices_enumerate();

    GPU::PhysicalDeviceID selected_physical_device = GPU::PhysicalDeviceID::invalid();

    bool finded = false;
    GPU::PhysicalDeviceID integrated = GPU::PhysicalDeviceID();
    GPU::PhysicalDeviceID cpu = GPU::PhysicalDeviceID();
    for(GPU::PhysicalDeviceID physical_device : physical_devices)
    {
        if(finded)
        {
            break;
        }

        GPU::PhysicalDeviceInfo pd_info = GPU::physical_device_get_info(physical_device);
        if(pd_info.device_type == GPU::DeviceType::DiscreteGPU)
        {
            selected_physical_device = physical_device;
            finded = true;
            break;
        }
        
        if(pd_info.device_type == GPU::DeviceType::IntegratedGPU)
        {
            integrated = physical_device;
        }
        else if(pd_info.device_type == GPU::DeviceType::Cpu)
        {
            cpu = physical_device;
        }
    }

    if(!finded && integrated.is_valid())
    {
        return integrated;
    }
    else if(!finded && !integrated.is_valid())
    {
        return cpu;
    }

    return selected_physical_device;
}

static RenderDevice::QueueList get_queue_list(GPU::DeviceID device)
{
    RenderDevice::QueueList queues = {};

    // Garanted
    queues.graphics = GPU::queue_get(device, {.usage = GPU::QueueUsage::Graphics, .index = 0});

    if(GPU::queue_get_count(device, {.usage = GPU::QueueUsage::Compute}) > 0)
    {
        queues.compute = GPU::queue_get(device, {.usage = GPU::QueueUsage::Compute, .index = 0});
    }
    else
    {
        queues.compute = queues.graphics;
    }

    if(GPU::queue_get_count(device, {.usage = GPU::QueueUsage::Copy}) > 0)
    {
        queues.copy = GPU::queue_get(device, {.usage = GPU::QueueUsage::Copy, .index = 0});
    }
    else
    {
        queues.copy = queues.compute;
    }

    if(GPU::queue_get_count(device, {.usage = GPU::QueueUsage::Present}) > 0)
    {
        queues.present = GPU::queue_get(device, {.usage = GPU::QueueUsage::Present, .index = 0});
    }
    else
    {
        queues.present = queues.graphics;
    }

    return queues;
}

RenderDevice::RenderDevice(Mem::Allocator& allocator, GPU::DeviceID device, GPUMemoryAllocator& memory_allocator)
: allocator(allocator),
device(device),
queues(get_queue_list(device)), memory_allocator(memory_allocator)
{}

RenderDevice::~RenderDevice()
{
    GPU::queue_wait_idle(get_graphics_queue());
    GPU::queue_wait_idle(get_compute_queue());
    GPU::queue_wait_idle(get_copy_queue());
    GPU::queue_wait_idle(get_present_queue());
}

GPU::PhysicalDeviceID RenderDevice::select_physical_device()
{
    return _select_physical_device();
}

GPU::DeviceID RenderDevice::create_device(GPU::PhysicalDeviceID selected_physical_device)
{
    return GPU::device_create(selected_physical_device, {});
}

}
