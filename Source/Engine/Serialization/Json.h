// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/String.h"
#include "Engine/Core/Types/StringView.h"

#if PLATFORM_SIMD_SSE4_2
#define RAPIDJSON_SSE42
#elif PLATFORM_SIMD_SSE2
#define RAPIDJSON_SSE2
#elif PLATFORM_SIMD_NEON
#define RAPIDJSON_NEON
#endif
#define RAPIDJSON_ERROR_CHARTYPE Char
#define RAPIDJSON_ERROR_STRING(x) TEXT(x)
#define RAPIDJSON_ASSERT(x) ASSERT(x)
#define RAPIDJSON_NEW(x) New<x>
#define RAPIDJSON_DELETE(x) Delete(x)
#define RAPIDJSON_NOMEMBERITERATORCLASS
//#define RAPIDJSON_MALLOC(size) ::malloc(size)
//#define RAPIDJSON_REALLOC(ptr, new_size) ::realloc(ptr, new_size)
//#define RAPIDJSON_FREE(ptr) ::free(ptr)

#include <ThirdParty/rapidjson/writer.h>
#include <ThirdParty/rapidjson/prettywriter.h>
#include <ThirdParty/rapidjson/document.h>

namespace rapidjson_flax
{
    // The memory allocator implementation for rapidjson library that uses default engine Allocator.
    class FlaxAllocator
    {
    public:
        static const bool kNeedFree = true;

        void* Malloc(size_t size)
        {
            // Behavior of malloc(0) is implementation defined so for size=0 return nullptr.
            // By default Flax doesn't use Allocate(0) so it's not important for the engine itself.
            if (size)
                return Allocator::Allocate((uint64)size);
            return nullptr;
        }

        void* Realloc(void* originalPtr, size_t originalSize, size_t newSize)
        {
            return AllocatorExt::Realloc(originalPtr, (uint64)originalSize, (uint64)newSize);
        }

        static void Free(void* ptr)
        {
            Allocator::Free(ptr);
        }
    };

    // String buffer with UTF8 encoding
    typedef rapidjson::GenericStringBuffer<rapidjson::UTF8<>, FlaxAllocator> StringBuffer;

    // GenericDocument with UTF8 encoding
    typedef rapidjson::GenericDocument<rapidjson::UTF8<>, rapidjson::MemoryPoolAllocator<FlaxAllocator>, FlaxAllocator> Document;

    // GenericValue with UTF8 encoding
    typedef rapidjson::GenericValue<rapidjson::UTF8<>, rapidjson::MemoryPoolAllocator<FlaxAllocator>> Value;

    // JSON writer to the stream
    template<typename OutputStream, typename SourceEncoding = rapidjson::UTF8<>, typename TargetEncoding = rapidjson::UTF8<>, unsigned writeFlags = rapidjson::kWriteDefaultFlags>
    using Writer = rapidjson::Writer<OutputStream, SourceEncoding, TargetEncoding, FlaxAllocator, writeFlags>;

    // Pretty JSON writer to the stream
    template<typename OutputStream, typename SourceEncoding = rapidjson::UTF8<>, typename TargetEncoding = rapidjson::UTF8<>, unsigned writeFlags = rapidjson::kWriteDefaultFlags>
    using PrettyWriter = rapidjson::PrettyWriter<OutputStream, SourceEncoding, TargetEncoding, FlaxAllocator, writeFlags>;
}
