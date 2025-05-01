// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

// Floating-point values tolerance error for blending animations
#define ANIM_GRAPH_BLEND_THRESHOLD 1e-5f
#define ANIM_GRAPH_BLEND_THRESHOLD2 (ANIM_GRAPH_BLEND_THRESHOLD * ANIM_GRAPH_BLEND_THRESHOLD)

// Enables/disables detailed animation graph profiling (via CPU profiler events)
#define ANIM_GRAPH_PROFILE 1

#if ANIM_GRAPH_PROFILE
#include "Engine/Profiler/ProfilerCPU.h"
#define ANIM_GRAPH_PROFILE_EVENT(name) PROFILE_CPU_NAMED(name)
#else
#define ANIM_GRAPH_PROFILE_EVENT(name)
#endif
