#pragma once
#include <Collections/Array.hpp>
#include <Collections/Stack.hpp>
#include <gpu/gpu.h>


namespace Basic
{

struct CommandPoolInfo
{
    Mem::Allocator& allocator;
    GPU::DeviceID device;
    GPU::QueueUsage queue_usage;
};

struct CommandPoolExecuteInfo
{
    Slice<const GPU::SemaphoreID> wait_semaphores;
    Slice<const GPU::PipelineStages> wait_stages;
    GPU::CommandBufferID command_buffer;
    Slice<const GPU::SemaphoreID> signal_semaphores;
};

struct CommandPoolExecuteEmptyInfo
{
    Slice<const GPU::SemaphoreID> wait_semaphores;
    Slice<const GPU::PipelineStages> wait_stages;
    Slice<const GPU::SemaphoreID> signal_semaphores;
};

struct CommandPool
{
    struct WorkSubmit
    {
        GPU::FenceID fence;
        GPU::CommandBufferID command_buffer;
        bool empty;
    };

    Mem::Allocator& allocator;
    GPU::DeviceID device;
    GPU::QueueUsage queue_usage;
    GPU::CommandPoolID gpu_command_pool;
    Collections::Array<GPU::CommandBufferID> command_buffers;
    Collections::Array<GPU::FenceID> gpu_work_fences;
    Collections::Array<WorkSubmit> work_submited;
        
    Collections::Stack<GPU::FenceID> gpu_free_fences;
    Collections::Stack<GPU::CommandBufferID> free_command_buffers;

    CommandPool(const CommandPoolInfo& info);
    ~CommandPool();

    GPU::CommandBufferID acquire_command_buffer();
    GPU::FenceID execute(GPU::QueueID queue, const CommandPoolExecuteInfo& info);
    GPU::FenceID execute_empty(GPU::QueueID queue, const CommandPoolExecuteEmptyInfo& info);

    void wait_for_all();

    void release_fence(GPU::FenceID fence);

    GPU::FenceID _alloc_new_fence();
    void _remove_finished_work();
};

}
