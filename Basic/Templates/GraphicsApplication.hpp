#pragma once
#include <Runtime/Application.hpp>

#include "Basic/Core/SwapChain.hpp"
#include "Basic/Core/RenderDevice.hpp"
#include "Basic/Memory/GenericGPUMemoryAllocator.hpp"


namespace Basic
{

struct GraphicsApplication : Application
{
    struct Device
    {
        GPU::DeviceID device;
        Device() : device(Basic::RenderDevice::create_device(Basic::RenderDevice::select_physical_device()))
        {}
        ~Device() { GPU::device_destroy(device); }
    };

    struct InternalData
    {
        Device device;
        Basic::GenericGPUMemoryAllocator gpu_memory_allocator;
        Basic::RenderDevice render_device;
        Basic::SwapChain swap_chain;

        InternalData(Mem::Allocator& allocator, Window& window)
        : device(),
        gpu_memory_allocator({.allocator = allocator, .device = device.device}),
        render_device(allocator, device.device, gpu_memory_allocator),
        swap_chain(
            Basic::SwapChainInfo{
                .allocator = allocator,
                .device = render_device.get_device(),
                .present_queue = render_device.get_present_queue(),
                .window = window.window_id,
                .surface_format = Basic::SwapChain::DefaultSurfaceFormat,
            }
        )
        {}
    };

    InternalData data;    

    GraphicsApplication(const ApplicationAllocateInfo& alloc_info);
    virtual ~GraphicsApplication();

    SwapChain& get_swap_chain() { return data.swap_chain; }
    RenderDevice& get_render_device() { return data.render_device; }
};

}
