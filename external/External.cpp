#include "Basic/Resource/ResourceManager.hpp"


// TODO: At applicacion can only use one resource manager now.
extern Basic::ResourceManager* current_rm;

static void* _alloc(usize size)
{
    Mem::Allocator& allocator = current_rm->get_allocator();
    return allocator.alloc(size, alignof(usize)).items;
}

static void* _realloc(void* ptr, usize old_size, usize new_size)
{
    Mem::Allocator& allocator = current_rm->get_allocator();

    Slice old_mem = Slice(reinterpret_cast<u8*>(ptr), old_size);
    if(ptr != nullptr && allocator.realloc(old_mem, new_size, alignof(usize)))
    {
        return ptr;
    }
    
    Slice new_mem = allocator.alloc(new_size, alignof(usize));
    if (!new_mem.null() && ptr != nullptr && old_size != 0)
    {
        Mem::copy(new_mem, old_mem);
        allocator.free(old_mem);
    }
    
    return new_mem.ptr();
}

static void _free(void* ptr)
{
    Mem::Allocator& allocator = current_rm->get_allocator();
    
    if(ptr != nullptr)
    {
        allocator.free(
            Slice(reinterpret_cast<u8*>(ptr), 1)
        );
    }
}

static void* __bread_memcpy(void* dest, const void* src, size_t len)
{
    Slice dest_items = Slice(reinterpret_cast<u8*>(dest), len);
    Slice src_items = Slice(reinterpret_cast<const u8*>(src), len);
    Mem::copy(dest_items, src_items);
    return dest;
}

static void* __bread_memset(void* dest, int value, size_t len)
{
    Slice dest_items = Slice(reinterpret_cast<u8*>(dest), len);
    Mem::set(dest_items, u8(value));
    return dest;
}


/*
* Here start external inclusion
* The external libraries are C compatible so they need c casting style.
*/

#if defined(BREAD_MSVC)
#pragma warning(push)
#pragma warning(disable : 4245)
#elif defined(BREAD_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#endif

#if !defined(BREAD_ANDROID)
static u32 _lrotl(u32 x, int y)
{
    return (((x) << (y)) | ((x) >> (-(y) & 31)));
}
#endif

#define stbi_err(x) DebugAssert(x, "stb_image assertion fail");

#define STBI_MALLOC(size) _alloc(size)
#define STBI_REALLOC(ptr, new_size) _realloc(ptr, 0, new_size)
#define STBI_FREE(ptr) _free(ptr)

#define STBI_REALLOC_SIZED(ptr, old_size, new_size)  _realloc(ptr, old_size, new_size)

#define STBI_NO_THREAD_LOCALS
#define STBI_NO_STDIO
#define STBI_NO_LINEAR
#define STBI_NO_HDR
#define STBI_ASSERT(x) DebugAssert(x, "stb_image assertion fail")

#define abs(x) Math::abs(x)
#define fabs(x) abs(x)

#define STB_IMAGE_IMPLEMENTATION
#include "external/stb_image.h"

#undef abs
#undef fabs

#define STBTT_ifloor(x) i32(Math::floor<f32>(x))
#define STBTT_iceil(x) i32(Math::ceil<f32>(x))
#define STBTT_sqrt(x) Math::sqrt(x)
#define STBTT_pow(x, y) Math::pow(x, y)
#define STBTT_fmod(x, y) Math::mod(x, y)
#define STBTT_cos(x) Math::cos(x)
#define STBTT_acos(x) Math::acos(x)
#define STBTT_fabs(x) Math::abs(x)

#define STBTT_malloc(size, u) ((void)u, _alloc(size))
#define STBTT_free(ptr, u) ((void)u, _free(ptr))

#define STBTT_assert(x)
#define STBTT_strlen(x) Core::NullTerminatedLen(x)

#define STBTT_memcpy(dest, src, size) __bread_memcpy(dest, src, size)
#define STBTT_memset(dest, value, size) __bread_memset(dest, value, size)

#define STB_TRUETYPE_IMPLEMENTATION
#include "external/stb_truetype.h"

#define DRWAV_ASSERT(...)
#define DRWAV_MALLOC(size) _alloc(size)
#define DRWAV_REALLOC(ptr, size) _realloc(ptr, 0, size)
#define DRWAV_FREE(ptr) _free(ptr)
#define DRWAV_COPY_MEMORY(dest, src, len) __bread_memcpy(reinterpret_cast<void*>(dest), reinterpret_cast<const void*>(src), len)
#define DRWAV_ZERO_MEMORY(dest, len) __bread_memset(reinterpret_cast<void*>(dest), 0, len)

#define DR_WAV_NO_STDIO
#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

#if defined(BREAD_MSVC)
#pragma warning(pop)
#elif defined(BREAD_CLANG)
#pragma clang diagnostic pop
#endif
