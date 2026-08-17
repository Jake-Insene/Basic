#pragma once
#include <gpu/gpu.h>

#include "Basic/Core/GPUMemoryAllocator.hpp"

namespace Basic
{

/**
* A Render Device is a collection of GPU resources and utilities.
* * A GPU Device.
* * Queues.
* * GPU Memory Allocator
* *
*/
struct RenderDevice
{
    DisableCopy(RenderDevice);
    DisableMove(RenderDevice);

    Mem::Allocator& allocator;

    GPU::DeviceID device;

    struct QueueList
    {
        GPU::QueueID graphics;
        GPU::QueueID compute;
        GPU::QueueID copy;
        GPU::QueueID present;
    } queues;

    GPUMemoryAllocator& memory_allocator;

    RenderDevice(Mem::Allocator& allocator, GPU::DeviceID device, GPUMemoryAllocator& memory_allocator);
    ~RenderDevice();

    GPU::DeviceID get_device() const { return device; }

    GPU::QueueID get_graphics_queue() const { return queues.graphics; }
    GPU::QueueID get_compute_queue() const { return queues.compute; }
    GPU::QueueID get_copy_queue() const { return queues.copy; }
    GPU::QueueID get_present_queue() const { return queues.present; }

    GPUMemoryAllocator& get_gpu_memory_allocator() const { return memory_allocator; }

    static GPU::DeviceID create_device();
};

}
