#include "Basic/Core/SwapChain.hpp"


namespace Basic
{

SwapChain::SwapChain(const SwapChainInfo& info)
: allocator(info.allocator), device(info.device), present_queue(info.present_queue),
window(info.window), surface_format(info.surface_format), present_mode(GPU::PresentMode::VSync),
swap_chain(), images(info.allocator, 3, {}), is_valid_swap_chain(false), pending_rebuild(true)
{
    _rebuild();
}

SwapChain::~SwapChain()
{
    _free_images();

    if(swap_chain.is_valid())
    {
        GPU::swap_chain_destroy(swap_chain);
    }
}

bool SwapChain::acquire_image(u32* image_index, GPU::SemaphoreID present_complete)
{
    if(pending_rebuild)
    {
        _rebuild();
    }

    if(!is_valid_swap_chain && !_try_rebuild())
    {
        return false;
    }

    GPU::AcquireResult result = GPU::swap_chain_acquire_next_image(
        swap_chain,
        {
            .timeout = Core::MaxValue<u64>,
            .semaphore = present_complete,
            .fence = GPU::FenceID::invalid(),
        }, 
        image_index
    );

    if(result == GPU::AcquireResult::Suboptimal)
    {
        pending_rebuild = true;
        return true;
    }
    
    if(result == GPU::AcquireResult::OutOfDate)
    {
        return false;
    }

    return true;
}

bool SwapChain::present(const u32 image_index, const Slice<const GPU::SemaphoreID>& wait_semaphores)
{
    const GPU::SwapChainID swap_chain_id = swap_chain;

    GPU::AcquireResult result = GPU::queue_present(
        present_queue,
        {
            .wait_semaphores = wait_semaphores,
            .swapchains = Slice(&swap_chain_id, 1),
            .image_indices = Slice(&image_index, 1),
        }
    );

    if(result == GPU::AcquireResult::Suboptimal || result == GPU::AcquireResult::OutOfDate)
    {
        return _try_rebuild();
    }

    return true;
}

void SwapChain::set_present_mode(GPU::PresentMode new_present_mode)
{
    if(present_mode != new_present_mode)
    {
        present_mode = new_present_mode;
        pending_rebuild = true;
    }
}

void SwapChain::_init_images()
{
    images.resize(GPU::swap_chain_get_image_count(swap_chain));
    for(usize i = 0; i < images.count; i++)
    {
        images.get(i) = 
        {
            .image = GPU::swap_chain_get_image(swap_chain, i),
            .image_view = GPU::swap_chain_get_image_view(swap_chain, i),
        };
    }
}

void SwapChain::_free_images()
{
    images.clear();
}

void SwapChain::_rebuild()
{
    GPU::queue_wait_idle(present_queue);

    _free_images();
    
    if(swap_chain.is_valid())
    {
        is_valid_swap_chain = false;
        pending_rebuild = true;
        GPU::swap_chain_destroy(swap_chain);
        swap_chain = GPU::SwapChainID::invalid();
    }

    image_size = Display::window_get_size(window);
    if(image_size.x == 0 || image_size.y == 0
        || image_size.x < 0 || image_size.y < 0)
    {
        return;
    }

    swap_chain = GPU::swap_chain_create(
        device,
        {
            .surface = Display::window_get_surface(window),
            .present_mode = present_mode,
            .format = surface_format,
            .min_image_count = DefaultMinImageCount,
            .size = Vector2U(image_size),
        }
    );
    is_valid_swap_chain = true;
    pending_rebuild = false;

    _init_images();
}

bool SwapChain::_try_rebuild()
{
    _rebuild();
    return is_valid_swap_chain;
}

}
