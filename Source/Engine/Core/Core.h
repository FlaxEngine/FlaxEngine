// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Compiler.h"

#define SAFE_DISPOSE(x) if(x) { (x)->Dispose(); (x) = nullptr; }
#define SAFE_RELEASE(x) if(x) { (x)->Release(); (x) = nullptr; }
#define SAFE_DELETE(x) if(x) { ::Delete(x); (x) = nullptr; }
#define INVALID_INDEX (-1)
#define _INTERNAL_CONCAT_MACROS(a, b) a ## b
#define CONCAT_MACROS(a, b) _INTERNAL_CONCAT_MACROS(a, b)
#define _INTERNAL_MACRO_TO_STR(x) #x
#define MACRO_TO_STR(x) _INTERNAL_MACRO_TO_STR(x)
#define CRASH \
    { \
        if (Platform::IsDebuggerPresent()) \
        { \
            PLATFORM_DEBUG_BREAK; \
        } \
        Platform::Crash(__LINE__, __FILE__); \
    }
#define OUT_OF_MEMORY Platform::OutOfMemory(__LINE__, __FILE__)
#define MISSING_CODE(info) Platform::MissingCode(__LINE__, __FILE__, info)
#define NON_COPYABLE(type) type(type&&) = delete; type(const type&) = delete; type& operator=(const type&) = delete; type& operator=(type&&) = delete;
#define POD_COPYABLE(type) \
    type(const type& other) { Platform::MemoryCopy(this, &other, sizeof(type)); } \
    type(type&& other) noexcept { Platform::MemoryCopy(this, &other, sizeof(type)); } \
    type& operator=(const type& other) { Platform::MemoryCopy(this, &other, sizeof(type)); return *this; } \
    type& operator=(type&& other) noexcept { Platform::MemoryCopy(this, &other, sizeof(type)); return *this; }
