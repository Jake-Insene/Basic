#pragma once
#include <Collections/Array.hpp>
#include <display/display.h>
#include <gpu/gpu.h>


namespace Basic
{

struct SwapChainInfo
{
    // Swap Chain allocator.
    Mem::Allocator& allocator;
    // Prefered to allocate in.
    GPU::DeviceID device;
    // The prefered queue to present.
    GPU::QueueID present_queue;
    // The target window.
    Display::WindowID window;
    // The target image format.
    GPU::TextureFormat surface_format;
};

/**
* Manages swap chain creation and recreation. It doesn't handle window deletion.
*
* The swap chain can be invalidated, this means it will require a reconstruction making
* its images to no longer be valid. This will trigger a GPU::queue_wait() when rebuilding to
* make sure no image is being in use.
*/
struct SwapChain
{
    DisableCopy(SwapChain);
    DisableMove(SwapChain);

    // TODO: Investigate about android prefered swap chain format
    static constexpr GPU::TextureFormat DefaultSurfaceFormat = GPU::TextureFormat::RGBA8Srgb;
    static constexpr usize DefaultMinImageCount = 3;

    struct ImageInfo
    {
        // Image GPU handle.
        GPU::TextureID image;
        // Image View GPU handle of surface_format.
        GPU::TextureViewID image_view;
    };

    Mem::Allocator& allocator;
    GPU::DeviceID device;
    GPU::QueueID present_queue;
    Display::WindowID window;
    GPU::TextureFormat surface_format;
    GPU::PresentMode present_mode;

    Vector2I image_size;
        
    GPU::SwapChainID swap_chain;
    Collections::Array<ImageInfo> images;

    bool is_valid_swap_chain;
    bool pending_rebuild;

    SwapChain(const SwapChainInfo& info);
    ~SwapChain();

    /**
    * It tries to acquire an image from the swap chain.
    * 
    * @param image_index A pointer to place the index to the image acqured for presenting.
    * @param present_complete A semaphore to signal when the image becomes available.
    * @return true If there is an image to acquire.
    * @return false If there is no image available for presenting.
    */
    bool acquire_image(u32* image_index, GPU::SemaphoreID present_complete);

    /**
    * @param image_index The index of the image to present
    * @param wait_semaphores A list of semaphores to wait before presenting.
    * @return If the image can be presented.
    */
    bool present(const u32 image_index, const Slice<const GPU::SemaphoreID>& wait_semaphores);

    /**
    * This triggers a invalidation.
    * 
    * @param new_present_mode The new present mode.
    */
    void set_present_mode(GPU::PresentMode new_present_mode);
    [[nodiscard]] GPU::PresentMode get_present_mode() const { return present_mode; }

    usize get_image_count() const { return images.count; }
    ImageInfo& get_image(u32 image_index) const { return images.get(image_index); }
    Vector2I get_image_size() const { return image_size; }

    GPU::TextureFormat get_surface_format() const { return surface_format; }

    void _init_images();
    void _free_images();
    void _rebuild();
    bool _try_rebuild();
};

}
