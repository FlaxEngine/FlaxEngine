// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#if PLATFORM_UNIX

#include "Engine/Platform/Platform.h"
#include <sys/types.h>
#include <unistd.h>
#include <cstdint>
#include <stdlib.h>

typedef uint16_t offset_t;
#define align_mem_up(num, align) (((num) + ((align) - 1)) & ~((align) - 1))

void* UnixPlatform::Allocate(uint64 size, uint64 alignment)
{
    void* ptr = nullptr;

    // Alignment always has to be power of two
    ASSERT_LOW_LAYER((alignment & (alignment - 1)) == 0);

    if (alignment && size)
    {
        uint32_t pad = sizeof(offset_t) + (alignment - 1);
        void* p = malloc(size + pad);
        if (p)
        {
            // Add the offset size to malloc's pointer
            ptr = (void*)align_mem_up(((uintptr_t)p + sizeof(offset_t)), alignment);

            // Calculate the offset and store it behind aligned pointer
            *((offset_t*)ptr - 1) = (offset_t)((uintptr_t)ptr - (uintptr_t)p);
        }
#if COMPILE_WITH_PROFILER
        OnMemoryAlloc(ptr, size);
#endif
    }
    return ptr;
}

void UnixPlatform::Free(void* ptr)
{
    if (ptr)
    {
#if COMPILE_WITH_PROFILER
        OnMemoryFree(ptr);
#endif
        // Walk backwards from the passed-in pointer to get the pointer offset
        offset_t offset = *((offset_t*)ptr - 1);

        // Get original pointer
        void* p = (void*)((uint8_t*)ptr - offset);

        // Free memory
        free(p);
    }
}

uint64 UnixPlatform::GetCurrentProcessId()
{
    return getpid();
}

#endif
