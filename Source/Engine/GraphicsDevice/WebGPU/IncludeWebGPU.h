// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if GRAPHICS_API_WEBGPU

// Fix Intellisense errors when Emsicpten SDK is not visible
#ifndef UINT32_C
#define UINT32_C(x) (x ## U)
#endif
#ifndef __SIZE_MAX__
#define __SIZE_MAX__ (static_cast<intptr>(-1))
#endif
#ifndef SIZE_MAX
#define SIZE_MAX __SIZE_MAX__
#endif

#include <webgpu/webgpu.h>

// Utiltiy macro to convert WGPUStringView into UTF-16 string (on stack)
#define WEBGPU_TO_STR(strView) StringAsUTF16<>(strView.data, strView.data ? strView.length : 0).Get()

// Utiltiy macro to get WGPUStringView for a text constant
#define WEBGPU_STR(str) { str, ARRAY_COUNT(str) - 1 }

#define WEBGPU_MAX_QUERY_SETS 8

#endif
