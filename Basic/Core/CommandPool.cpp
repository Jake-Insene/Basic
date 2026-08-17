#include "Basic/Core/CommandPool.hpp"


namespace Basic
{

CommandPool::CommandPool(const CommandPoolInfo& info)
: allocator(info.allocator), device(info.device), queue_usage(info.queue_usage),
command_buffers(info.allocator, 4, {}),
gpu_work_fences(info.allocator, 4, {}),
work_submited(info.allocator, 4, {}),
gpu_free_fences(info.allocator, 4, {}),
free_command_buffers(info.allocator, 4, {})
{
    gpu_command_pool = GPU::command_pool_create(
        device,
        {
            .usage = queue_usage,
        }
    );
}

CommandPool::~CommandPool()
{
    for(GPU::CommandBufferID command_buffer : command_buffers.iter())
    {
        GPU::command_buffer_free(command_buffer);
    }

    for(GPU::FenceID fence : gpu_work_fences.iter())
    {
        GPU::fence_destroy(fence);
    }

    GPU::command_pool_destroy(gpu_command_pool);
}

GPU::CommandBufferID CommandPool::acquire_command_buffer()
{
    if(!free_command_buffers.is_empty())
    {
        return free_command_buffers.pop();
    }

    GPU::CommandBufferID command_buffer = GPU::command_buffer_allocate(device, {.pool = gpu_command_pool});
    return command_buffers.add(command_buffer);
}

GPU::FenceID CommandPool::execute(GPU::QueueID queue, const CommandPoolExecuteInfo& info)
{
    // check for free command buffers
    GPU::FenceID fence = GPU::FenceID::invalid();
    if(!gpu_free_fences.is_empty())
    {
        fence = gpu_free_fences.pop();
        GPU::fence_reset(Slice(&fence, 1));
    }
    else
    {
        fence = _alloc_new_fence();
    }

    // submit command buffer
    GPU::queue_execute_command_buffer(queue,
        {
            .wait_semaphores = info.wait_semaphores,
            .wait_stages = info.wait_stages,
            .command_buffers = Slice(&info.command_buffer, 1),
            .signal_semaphores = info.signal_semaphores,
            .fence = fence,
        }
    );

    (void)work_submited.add(
        WorkSubmit
        {
            .fence = fence,
            .command_buffer = info.command_buffer,
            .empty = false,
        }
    );
    
    return fence;
}

GPU::FenceID CommandPool::execute_empty(GPU::QueueID queue, const CommandPoolExecuteEmptyInfo& info)
{
    // check for free command buffers
    GPU::FenceID fence = GPU::FenceID::invalid();
    if(!gpu_free_fences.is_empty())
    {
        fence = gpu_free_fences.pop();
        GPU::fence_reset(Slice(&fence, 1));
    }
    else
    {
        fence = _alloc_new_fence();
    }

    // submit
    GPU::queue_execute_command_buffer(queue,
        {
            .wait_semaphores = info.wait_semaphores,
            .wait_stages = info.wait_stages,
            .command_buffers = {},
            .signal_semaphores = info.signal_semaphores,
            .fence = fence,
        }
    );
    
    (void)work_submited.add(
        WorkSubmit
        {
            .fence = fence,
            .command_buffer = GPU::CommandBufferID::invalid(),
            .empty = true,
        }
    );

    return fence;
}

void CommandPool::wait_for_all()
{
    if(work_submited.count == 0)
    {
        return;
    }

    Slice fences = allocator.array<GPU::FenceID>(work_submited.count);
    for(usize i = 0; i < work_submited.count; i++)
    {
        fences[i] = work_submited.get(i).fence;
    }

    GPU::fence_wait_for(fences, true, Core::MaxValue<u64>);
    allocator.free(Mem::to_bytes(fences));
    _remove_finished_work();
}

void CommandPool::release_fence(GPU::FenceID fence)
{
    for(usize i = 0; i < work_submited.count; i++)
    {
        WorkSubmit& work_data = work_submited.get(i);
        if(work_data.fence != fence)
        {
            continue;
        }

        GPU::fence_reset(Slice(&work_data.fence, 1));
        gpu_free_fences.push(work_data.fence);
        if(!work_data.empty)
        {
            free_command_buffers.push(work_data.command_buffer);
        }
        work_submited.remove_at(i);
        break;
    }
}

GPU::FenceID CommandPool::_alloc_new_fence()
{
    GPU::FenceID fence = GPU::fence_create(device, {.signaled = false});
    (void)gpu_work_fences.add(fence);
    return fence;
}

void CommandPool::_remove_finished_work()
{
    for(usize i = 0; i < work_submited.count; i++)
    {
        WorkSubmit& work_data = work_submited.get(i);
        if(!GPU::fence_get_state(work_data.fence))
        {
            continue;
        }

        gpu_free_fences.push(work_data.fence);
        if(!work_data.empty)
        {
            free_command_buffers.push(work_data.command_buffer);
        }
        work_submited.remove_at(i);
        i--;
    }
}

}
