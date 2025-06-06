// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Platform/Platform.h"

#if COMPILE_WITH_PROFILER

#include "Engine/Core/Types/StringView.h"

/// <summary>
/// Provides memory allocations collecting utilities.
/// </summary>
API_CLASS(Static) class FLAXENGINE_API ProfilerMemory
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(ProfilerMemory);
public:
    /// <summary>
    /// List of different memory categories used to track and analyze memory allocations specific to a certain engine system.
    /// </summary>
    API_ENUM() enum class Groups : uint8
    {
        // Not categorized.
        Unknown,
        // Total used system memory (reported by platform).
        Total,
        // Total amount of tracked memory (by ProfilerMemory tool).
        TotalTracked,
        // Total amount of untracked memory (gap between total system memory usage and tracked memory size).
        TotalUntracked,
        // Initial memory used by program upon startup (eg. executable size, static variables).
        ProgramSize,
        // Profiling tool memory overhead.
        Profiler,

        // Total memory allocated via dynamic memory allocations.
        Malloc,
        // Total memory allocated via arena allocators (all pages).
        MallocArena,

        // General purpose engine memory.
        Engine,
        // Memory used by the threads (and relevant systems such as Job System).
        EngineThreading,
        // Memory used by Delegate (engine events system to store all references).
        EngineDelegate,

        // Total graphics memory usage.
        Graphics,
        // Total textures memory usage.
        GraphicsTextures,
        // Total render targets memory usage (textures used as target image for rendering).
        GraphicsRenderTargets,
        // Total cubemap textures memory usage (each cubemap is 6 textures).
        GraphicsCubeMaps,
        // Total volume textures memory usage (3D textures).
        GraphicsVolumeTextures,
        // Total buffers memory usage.
        GraphicsBuffers,
        // Total vertex buffers memory usage.
        GraphicsVertexBuffers,
        // Total index buffers memory usage.
        GraphicsIndexBuffers,
        // Total meshes memory usage (vertex and index buffers allocated by models).
        GraphicsMeshes,
        // Totoal shaders memory usage (shaders bytecode, PSOs data).
        GraphicsShaders,
        // Totoal materials memory usage (constant buffers, parameters data).
        GraphicsMaterials,
        // Totoal command buffers memory usage (draw lists, constants uploads, ring buffer allocators).
        GraphicsCommands,

        // Total Artificial Intelligence systems memory usage (eg. Behavior Trees).
        AI,

        // Total animations system memory usage.
        Animations,
        // Total animation data memory usage (curves, events, keyframes, graphs, etc.).
        AnimationsData,

        // Total audio system memory.
        Audio,

        // Total content system memory usage.
        Content,
        // Total general purpose memory allocated by assets.
        ContentAssets,
        // Total memory used by content files buffers (file reading and streaming buffers).
        ContentFiles,
        // Total memory used by content streaming system (internals).
        ContentStreaming,

        // Total memory allocated by scene objects.
        Level,
        // Total memory allocated by the foliage system (quad-tree, foliage instances data). Excluding foliage models data.
        LevelFoliage,
        // Total memory allocated by the terrain system (patches).
        LevelTerrain,

        // Total memory allocated by input system.
        Input,

        // Total localization system memory.
        Localization,

        // Total navigation system memory.
        Navigation,

        // Total networking system memory.
        Networking,

        // Total particles memory (loaded assets, particles buffers and instance parameters).
        Particles,

        // Total physics memory.
        Physics,

        // Total scripting memory allocated by game.
        Scripting,
        // Total Visual scripting memory allocated by game (visual script graphs, data and runtime allocations).
        ScriptingVisual,
        // Total C# scripting memory allocated by game (runtime assemblies, managed interop and runtime allocations).
        ScriptingCSharp,
        // Total amount of committed virtual memory in use by the .NET GC, as observed during the latest garbage collection.
        ScriptingCSharpGCCommitted,
        // Total managed GC heap size (including fragmentation), as observed during the latest garbage collection.
        ScriptingCSharpGCHeap,

        // Total User Interface components memory.
        UI,

        // Total video system memory (video file data, frame buffers, GPU images and any audio buffers used by video playback).
        Video,

        // Custom game-specific memory tracking.
        CustomGame0,
        // Custom game-specific memory tracking.
        CustomGame1,
        // Custom game-specific memory tracking.
        CustomGame2,
        // Custom game-specific memory tracking.
        CustomGame3,
        // Custom game-specific memory tracking.
        CustomGame4,
        // Custom game-specific memory tracking.
        CustomGame5,
        // Custom game-specific memory tracking.
        CustomGame6,
        // Custom game-specific memory tracking.
        CustomGame7,
        // Custom game-specific memory tracking.
        CustomGame8,
        // Custom game-specific memory tracking.
        CustomGame9,

        // Custom plugin-specific memory tracking.
        CustomPlugin0,
        // Custom plugin-specific memory tracking.
        CustomPlugin1,
        // Custom plugin-specific memory tracking.
        CustomPlugin2,
        // Custom plugin-specific memory tracking.
        CustomPlugin3,
        // Custom plugin-specific memory tracking.
        CustomPlugin4,
        // Custom plugin-specific memory tracking.
        CustomPlugin5,
        // Custom plugin-specific memory tracking.
        CustomPlugin6,
        // Custom plugin-specific memory tracking.
        CustomPlugin7,
        // Custom plugin-specific memory tracking.
        CustomPlugin8,
        // Custom plugin-specific memory tracking.
        CustomPlugin9,

        // Custom platform-specific memory tracking.
        CustomPlatform0,
        // Custom platform-specific memory tracking.
        CustomPlatform1,
        // Custom platform-specific memory tracking.
        CustomPlatform2,
        // Custom platform-specific memory tracking.
        CustomPlatform3,

        // Total editor-specific memory.
        Editor,

        MAX
    };

    /// <summary>
    /// The memory groups array wrapper to avoid dynamic memory allocation.
    /// </summary>
    API_STRUCT(NoDefault) struct GroupsArray
    {
        DECLARE_SCRIPTING_TYPE_MINIMAL(GroupsArray);

        // Values for each group
        API_FIELD(NoArray) int64 Values[100];
    };

public:
    /// <summary>
    /// Increments memory usage by a specific group.
    /// </summary>
    /// <param name="group">The group to update.</param>
    /// <param name="size">The amount of memory allocated (in bytes).</param>
    API_FUNCTION() static void IncrementGroup(Groups group, uint64 size);

    /// <summary>
    /// Decrements memory usage by a specific group.
    /// </summary>
    /// <param name="group">The group to update.</param>
    /// <param name="size">The amount of memory freed (in bytes).</param>
    API_FUNCTION() static void DecrementGroup(Groups group, uint64 size);

    /// <summary>
    /// Enters a new group context scope (by the current thread). Informs the profiler about context of any memory allocations within.
    /// </summary>
    /// <param name="group">The group to enter.</param>
    API_FUNCTION() static void BeginGroup(Groups group);

    /// <summary>
    /// Leaves the last group context scope (by the current thread).
    /// </summary>
    API_FUNCTION() static void EndGroup();

    /// <summary>
    /// Renames the group. Can be used for custom game/plugin groups naming.
    /// </summary>
    /// <param name="group">The group to update.</param>
    /// <param name="name">The new name to set.</param>
    API_FUNCTION() static void RenameGroup(Groups group, const StringView& name);

    /// <summary>
    /// Gets the names of all groups (array matches Groups enums).
    /// </summary>
    API_FUNCTION() static Array<String, HeapAllocation> GetGroupNames();

    /// <summary>
    /// Gets the memory stats for all groups (array matches Groups enums).
    /// </summary>
    /// <param name="mode">0 to get current memory, 1 to get peek memory, 2 to get current count.</param>
    API_FUNCTION() static GroupsArray GetGroups(int32 mode = 0);

    /// <summary>
    /// Dumps the memory allocations stats (groupped).
    /// </summary>
    /// <param name="options">'all' to dump all groups, 'file' to dump info to a file (in Logs folder)</param>
    API_FUNCTION(Attributes="DebugCommand") static void Dump(const StringView& options = StringView::Empty);

public:
    /// <summary>
    /// The profiling tools usage flag. Can be used to disable profiler. Run engine with '-mem' command line to activate it from start.
    /// </summary>
    API_FIELD(ReadOnly) static bool Enabled;

    static void OnMemoryAlloc(void* ptr, uint64 size);
    static void OnMemoryFree(void* ptr);
    static void OnGroupUpdate(Groups group, int64 sizeDelta, int64 countDelta);

public:
    /// <summary>
    /// Helper structure used to call begin/end on group within single code block.
    /// </summary>
    struct GroupScope
    {
        FORCE_INLINE GroupScope(Groups group)
        {
            if (ProfilerMemory::Enabled)
                ProfilerMemory::BeginGroup(group);
        }

        FORCE_INLINE ~GroupScope()
        {
            if (ProfilerMemory::Enabled)
                ProfilerMemory::EndGroup();
        }
    };
};

#define PROFILE_MEM_INC(group, size) ProfilerMemory::IncrementGroup(ProfilerMemory::Groups::group, size)
#define PROFILE_MEM_DEC(group, size) ProfilerMemory::DecrementGroup(ProfilerMemory::Groups::group, size)
#define PROFILE_MEM(group) ProfilerMemory::GroupScope ProfileMem(ProfilerMemory::Groups::group)
#define PROFILE_MEM_BEGIN(group) if (ProfilerMemory::Enabled) ProfilerMemory::BeginGroup(ProfilerMemory::Groups::group)
#define PROFILE_MEM_END() if (ProfilerMemory::Enabled) ProfilerMemory::EndGroup()

#else

// Empty macros for disabled profiler
#define PROFILE_MEM_INC(group, size)
#define PROFILE_MEM_DEC(group, size)
#define PROFILE_MEM(group)
#define PROFILE_MEM_BEGIN(group)
#define PROFILE_MEM_END()

#endif
