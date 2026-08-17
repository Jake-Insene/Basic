#pragma once
#include <Collections/ID.hpp>
#include <gpu/gpu.h>


namespace Basic
{
    enum class AllocationTag
    {
        Staging,
        Texture,
        Buffer,
    };

    using GPUMemoryAllocationID = Collections::ID<u32, struct _AllocationTag>;

    struct GPUMemoryAllocator
    {
        /**
        * Memory Allocation API
        */
        virtual GPUMemoryAllocationID allocate(AllocationTag tag, const GPU::MemoryRequirements& requirements) = 0;
        virtual void free(GPUMemoryAllocationID allocation) = 0;

        /**
        * Staging API
        */
        virtual GPU::BufferID begin_staging(usize size) = 0;
        virtual void end_staging(GPU::BufferID staging_buffer) = 0;

        virtual Slice<u8> map_staging(GPU::BufferID staging_buffer) = 0;
        virtual void unmap_staging(GPU::BufferID staging_buffer, const Slice<u8>& memory) = 0;

        /**
        * Allocation API
        */

        virtual GPU::MemoryHeapID allocation_get_heap(GPUMemoryAllocationID allocation) = 0;
        [[nodiscard]] virtual usize allocation_get_offset(GPUMemoryAllocationID allocation) = 0;
        virtual Slice<u8> allocation_map(GPUMemoryAllocationID allocation) = 0;
        virtual void allocation_unmap(GPUMemoryAllocationID allocation, const Slice<u8>& mapped) = 0;
    };
}