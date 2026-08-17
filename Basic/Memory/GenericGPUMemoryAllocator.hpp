#pragma once
#include "Collections/Array.hpp"
#include "Collections/FreeList.hpp"
#include "gpu/gpu.h"
#include "Mem/Allocator.hpp"

#include "Basic/Core/GPUMemoryAllocator.hpp"

namespace Basic
{

struct GPUMemoryAllocatorCreateInfo
{
    Mem::Allocator& allocator;
    GPU::DeviceID device;
};

struct GenericGPUMemoryAllocator : GPUMemoryAllocator
{
    struct Allocation
    {
        usize heap_index;
        usize heap_offset;

        AllocationTag tag;
        usize size;

        bool free;

        GPUMemoryAllocationID prev;
        GPUMemoryAllocationID next;
    };

    struct Heap
    {
        GPU::MemoryHeapID heap;
        usize heap_size;
        GPU::HeapUsage heap_usage;
        AllocationTag tag;
        usize heap_index;

        usize map_count;
        Slice<u8> mapped;

        GPUMemoryAllocationID first_allocation;
    };

    enum class StagingFlags
    {
        Mapped = Bit(0),
        Allocated = Bit(1),
    };

    struct StagingHeap
    {
        GPU::MemoryHeapID heap;
        StagingFlags flags;
        usize heap_size;
        GPU::BufferID buffer;
        Slice<u8> mapped_buffer;
    };

    Mem::Allocator& allocator;
    GPU::DeviceID device;

    Collections::Array<Heap> heaps;
    Collections::FreeList<Allocation, GPUMemoryAllocationID> allocations;
    Collections::Array<StagingHeap> staging_heaps;

    GenericGPUMemoryAllocator(const GPUMemoryAllocatorCreateInfo& info);
    ~GenericGPUMemoryAllocator();

    virtual GPUMemoryAllocationID allocate(AllocationTag tag, const GPU::MemoryRequirements& requirements) override;
    virtual void free(GPUMemoryAllocationID allocation) override;

    virtual GPU::BufferID begin_staging(usize size) override;
    virtual void end_staging(GPU::BufferID staging_buffer) override;

    virtual Slice<u8> map_staging(GPU::BufferID staging_buffer) override;
    virtual void unmap_staging(GPU::BufferID staging_buffer, const Slice<u8>& memory) override;

    virtual GPU::MemoryHeapID allocation_get_heap(GPUMemoryAllocationID allocation) override;
    [[nodiscard]] virtual usize allocation_get_offset(GPUMemoryAllocationID allocation) override;
    virtual Slice<u8> allocation_map(GPUMemoryAllocationID allocation) override;
    virtual void allocation_unmap(GPUMemoryAllocationID allocation, const Slice<u8>& mapped) override;

    Heap& _request_heap_for(AllocationTag tag, usize size, GPU::HeapUsage heap_usage);
    Heap& _create_heap(AllocationTag tag, usize size, GPU::HeapUsage heap_usage);
};

}

EnableBitOp(Basic::GenericGPUMemoryAllocator::StagingFlags);
