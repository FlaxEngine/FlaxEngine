// Copyright (c) Wojciech Figat. All rights reserved.

#if COMPILE_WITH_PROFILER

#include "ProfilerMemory.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Utilities.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Core/Types/StringBuilder.h"
#include "Engine/Core/Collections/Sorting.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Platform/MemoryStats.h"
#include "Engine/Platform/File.h"
#include "Engine/Scripting/Enums.h"
#include "Engine/Threading/ThreadLocal.h"
#include "Engine/Utilities/StringConverter.h"

#define GROUPS_COUNT (int32)ProfilerMemory::Groups::MAX

static_assert(GROUPS_COUNT <= MAX_uint8, "Fix memory profiler groups to fit a single byte.");

// Compact name storage.
struct GroupNameBuffer
{
    Char Buffer[30];

    template<typename T>
    void Set(const T* str, bool autoFormat = false)
    {
        int32 max = StringUtils::Length(str), dst = 0;
        char prev = 0;
        for (int32 i = 0; i < max && dst < ARRAY_COUNT(Buffer) - 2; i++)
        {
            char cur = str[i];
            if (autoFormat && StringUtils::IsUpper(cur) && StringUtils::IsLower(prev))
                Buffer[dst++] = '/';
            Buffer[dst++] = cur;
            prev = cur;
        }
        Buffer[dst] = 0;
    }
};

// Compact groups stack container.
struct GroupStackData
{
    uint8 Count : 7;
    uint8 SkipRecursion : 1;
    uint8 Stack[15];

    FORCE_INLINE void Push(ProfilerMemory::Groups group)
    {
        if (Count < ARRAY_COUNT(Stack))
            Count++;
        Stack[Count - 1] = (uint8)group;
    }

    FORCE_INLINE void Pop()
    {
        if (Count > 0)
            Count--;
    }

    FORCE_INLINE ProfilerMemory::Groups Peek() const
    {
        return Count > 0 ? (ProfilerMemory::Groups)Stack[Count - 1] : ProfilerMemory::Groups::Unknown;
    }
};

template<>
struct TIsPODType<GroupStackData>
{
    enum { Value = true };
};

// Memory allocation data for a specific pointer.
struct PointerData
{
    uint32 Size;
    uint8 Group;
};

template<>
struct TIsPODType<PointerData>
{
    enum { Value = true };
};

#define UPDATE_PEEK(group) Platform::AtomicStore(&GroupMemoryPeek[(int32)group], Math::Max(Platform::AtomicRead(&GroupMemory[(int32)group]), Platform::AtomicRead(&GroupMemoryPeek[(int32)group])))

namespace
{
    alignas(16) volatile int64 GroupMemory[GROUPS_COUNT] = {};
    alignas(16) volatile int64 GroupMemoryPeek[GROUPS_COUNT] = {};
    alignas(16) volatile int64 GroupMemoryCount[GROUPS_COUNT] = {};
    uint8 GroupParents[GROUPS_COUNT] = {};
    ThreadLocal<GroupStackData> GroupStack;
    GroupNameBuffer GroupNames[GROUPS_COUNT];
    bool InitedNames = false;
    CriticalSection PointersLocker;
    Dictionary<void*, PointerData> Pointers;

    void InitNames()
    {
        if (InitedNames)
            return;
        InitedNames = true;
        for (int32 i = 0; i < GROUPS_COUNT; i++)
        {
            const char* name = ScriptingEnum::GetName((ProfilerMemory::Groups)i);
            GroupNames[i].Set(name, true);
        }
#define RENAME_GROUP(group, name) GroupNames[(int32)ProfilerMemory::Groups::group].Set(name)
        RENAME_GROUP(GraphicsRenderTargets, "Graphics/RenderTargets");
        RENAME_GROUP(GraphicsCubeMaps, "Graphics/CubeMaps");
        RENAME_GROUP(GraphicsVolumeTextures, "Graphics/VolumeTextures");
        RENAME_GROUP(GraphicsVertexBuffers, "Graphics/VertexBuffers");
        RENAME_GROUP(GraphicsIndexBuffers, "Graphics/IndexBuffers");
#undef RENAME_GROUP

        // Init constant memory
        PROFILE_MEM_INC(ProgramSize, Platform::GetMemoryStats().ProgramSizeMemory);
        UPDATE_PEEK(ProfilerMemory::Groups::ProgramSize);
    }

    void Dump(StringBuilder& output, const int32 maxCount)
    {
        InitNames();

        // Sort groups
        struct GroupInfo
        {
            ProfilerMemory::Groups Group;
            int64 Size;
            int64 Peek;
            uint32 Count;

            bool operator<(const GroupInfo& other) const
            {
                return Size > other.Size;
            }
        };
        GroupInfo groups[GROUPS_COUNT];
        for (int32 i = 0; i < GROUPS_COUNT; i++)
        {
            GroupInfo& group = groups[i];
            group.Group = (ProfilerMemory::Groups)i;
            group.Size = Platform::AtomicRead(&GroupMemory[i]);
            group.Peek = Platform::AtomicRead(&GroupMemoryPeek[i]);
            group.Count = (uint32)Platform::AtomicRead(&GroupMemoryCount[i]);
        }
        Sorting::QuickSort(groups, GROUPS_COUNT);

        // Print groups
        output.Append(TEXT("Memory profiler summary:")).AppendLine();
        for (int32 i = 0; i < maxCount; i++)
        {
            const GroupInfo& group = groups[i];
            if (group.Size == 0)
                break;
            const Char* name = GroupNames[(int32)group.Group].Buffer;
            const String size = Utilities::BytesToText(group.Size);
            const String peek = Utilities::BytesToText(group.Peek);
            output.AppendFormat(TEXT("{:>30}: {:>11} (peek: {}, count: {})"), name, size.Get(), peek.Get(), group.Count);
            output.AppendLine();
        }

        // Warn that data might be missing due to inactive profiler
        if (!ProfilerMemory::Enabled)
            output.AppendLine(TEXT("Detailed memory profiling is disabled. Run with command line: -mem"));
    }

    FORCE_INLINE void AddGroupMemory(ProfilerMemory::Groups group, int64 add)
    {
        // Group itself
        Platform::InterlockedAdd(&GroupMemory[(int32)group], add);
        Platform::InterlockedIncrement(&GroupMemoryCount[(int32)group]);
        UPDATE_PEEK(group);

        // Total memory
        Platform::InterlockedAdd(&GroupMemory[(int32)ProfilerMemory::Groups::TotalTracked], add);
        Platform::InterlockedIncrement(&GroupMemoryCount[(int32)ProfilerMemory::Groups::TotalTracked]);
        UPDATE_PEEK(ProfilerMemory::Groups::TotalTracked);

        // Group hierarchy parents
        uint8 parent = GroupParents[(int32)group];
        while (parent != 0)
        {
            Platform::InterlockedAdd(&GroupMemory[parent], add);
            Platform::InterlockedIncrement(&GroupMemoryCount[parent]);
            UPDATE_PEEK(parent);
            parent = GroupParents[parent];
        }
    }

    FORCE_INLINE void SubGroupMemory(ProfilerMemory::Groups group, int64 add)
    {
        // Group itself
        int64 value = Platform::InterlockedAdd(&GroupMemory[(int32)group], add);
        Platform::InterlockedDecrement(&GroupMemoryCount[(int32)group]);

        // Total memory
        value = Platform::InterlockedAdd(&GroupMemory[(int32)ProfilerMemory::Groups::TotalTracked], add);
        Platform::InterlockedDecrement(&GroupMemoryCount[(int32)ProfilerMemory::Groups::TotalTracked]);

        // Group hierarchy parents
        uint8 parent = GroupParents[(int32)group];
        while (parent != 0)
        {
            value = Platform::InterlockedAdd(&GroupMemory[parent], add);
            Platform::InterlockedDecrement(&GroupMemoryCount[parent]);
            parent = GroupParents[parent];
        }
    }
}

void InitProfilerMemory(const Char* cmdLine)
{
    // Check for command line option (memory profiling affects performance thus not active by default)
    ProfilerMemory::Enabled = StringUtils::FindIgnoreCase(cmdLine, TEXT("-mem"));

    // Init hierarchy
#define INIT_PARENT(parent, child) GroupParents[(int32)ProfilerMemory::Groups::child] = (uint8)ProfilerMemory::Groups::parent
    INIT_PARENT(Malloc, MallocArena);
    INIT_PARENT(Graphics, GraphicsTextures);
    INIT_PARENT(Graphics, GraphicsRenderTargets);
    INIT_PARENT(Graphics, GraphicsCubeMaps);
    INIT_PARENT(Graphics, GraphicsVolumeTextures);
    INIT_PARENT(Graphics, GraphicsBuffers);
    INIT_PARENT(Graphics, GraphicsVertexBuffers);
    INIT_PARENT(Graphics, GraphicsIndexBuffers);
    INIT_PARENT(Graphics, GraphicsMeshes);
    INIT_PARENT(Graphics, GraphicsShaders);
    INIT_PARENT(Graphics, GraphicsMaterials);
    INIT_PARENT(Graphics, GraphicsCommands);
    INIT_PARENT(Animations, AnimationsData);
    INIT_PARENT(Content, ContentAssets);
    INIT_PARENT(Content, ContentFiles);
#undef INIT_PARENT
}

void TickProfilerMemory()
{
    // Update profiler memory
    PointersLocker.Lock();
    GroupMemory[(int32)ProfilerMemory::Groups::Profiler] = 
        sizeof(GroupMemory) + sizeof(GroupNames) + sizeof(GroupStack) + 
        Pointers.Capacity() * sizeof(Dictionary<void*, PointerData>::Bucket);
    PointersLocker.Unlock();

    // Get total system memory and update untracked amount
    auto memory = Platform::GetProcessMemoryStats();
    memory.UsedPhysicalMemory -= GroupMemory[(int32)ProfilerMemory::Groups::Profiler];
    GroupMemory[(int32)ProfilerMemory::Groups::Total] = memory.UsedPhysicalMemory;
    GroupMemory[(int32)ProfilerMemory::Groups::TotalUntracked] = Math::Max<int64>(memory.UsedPhysicalMemory - GroupMemory[(int32)ProfilerMemory::Groups::TotalTracked], 0);

    // Update peeks
    UPDATE_PEEK(ProfilerMemory::Groups::Profiler);
    UPDATE_PEEK(ProfilerMemory::Groups::Total);
    UPDATE_PEEK(ProfilerMemory::Groups::TotalUntracked);
    GroupMemoryPeek[(int32)ProfilerMemory::Groups::Total] = Math::Max(GroupMemoryPeek[(int32)ProfilerMemory::Groups::Total], GroupMemoryPeek[(int32)ProfilerMemory::Groups::TotalTracked]);
}

bool ProfilerMemory::Enabled = false;

void ProfilerMemory::IncrementGroup(Groups group, uint64 size)
{
    AddGroupMemory(group, (int64)size);
}

void ProfilerMemory::DecrementGroup(Groups group, uint64 size)
{
    SubGroupMemory(group, -(int64)size);
}

void ProfilerMemory::BeginGroup(Groups group)
{
    auto& stack = GroupStack.Get();
    stack.Push(group);
}

void ProfilerMemory::EndGroup()
{
    auto& stack = GroupStack.Get();
    stack.Pop();
}

void ProfilerMemory::RenameGroup(Groups group, const StringView& name)
{
    GroupNames[(int32)group].Set(name.Get());
}

Array<String> ProfilerMemory::GetGroupNames()
{
    Array<String> result;
    result.Resize((int32)Groups::MAX);
    InitNames();
    for (int32 i = 0; i < (int32)Groups::MAX; i++)
        result[i] = GroupNames[i].Buffer;
    return result;
}

ProfilerMemory::GroupsArray ProfilerMemory::GetGroups(int32 mode)
{
    GroupsArray result;
    Platform::MemoryClear(&result, sizeof(result));
    static_assert(ARRAY_COUNT(result.Values) >= (int32)Groups::MAX, "Update group array size.");
    InitNames();
    if (mode == 0)
    {
        for (int32 i = 0; i < (int32)Groups::MAX; i++)
            result.Values[i] = Platform::AtomicRead(&GroupMemory[i]);
    }
    else if (mode == 1)
    {
        for (int32 i = 0; i < (int32)Groups::MAX; i++)
            result.Values[i] = Platform::AtomicRead(&GroupMemoryPeek[i]);
    }
    else if (mode == 2)
    {
        for (int32 i = 0; i < (int32)Groups::MAX; i++)
            result.Values[i] = Platform::AtomicRead(&GroupMemoryCount[i]);
    }
    return result;
}

void ProfilerMemory::Dump(const StringView& options)
{
    bool file = options.Contains(TEXT("file"));
    StringBuilder output;
    int32 maxCount = 20;
    if (file || options.Contains(TEXT("all")))
        maxCount = MAX_int32;
    ::Dump(output, maxCount);
    if (file)
    {
        String path = String(StringUtils::GetDirectoryName(Log::Logger::LogFilePath)) / TEXT("Memory_") + DateTime::Now().ToFileNameString() + TEXT(".txt");
        File::WriteAllText(path, output, Encoding::ANSI);
        LOG(Info, "Saved to {}", path);
        return;
    }
    LOG_STR(Info, output.ToStringView());
}

void ProfilerMemory::OnMemoryAlloc(void* ptr, uint64 size)
{
    ASSERT_LOW_LAYER(Enabled && ptr);
    auto& stack = GroupStack.Get();
    if (stack.SkipRecursion)
        return;
    stack.SkipRecursion = true;

    // Register pointer
    PointerData ptrData;
    ptrData.Size = size;
    ptrData.Group = (uint8)stack.Peek();
    PointersLocker.Lock();
    Pointers[ptr] = ptrData;
    PointersLocker.Unlock();

    // Update group memory
    const int64 add = (int64)size;
    AddGroupMemory((Groups)ptrData.Group, add);
    Platform::InterlockedAdd(&GroupMemory[(int32)ProfilerMemory::Groups::Malloc], add);
    Platform::InterlockedIncrement(&GroupMemoryCount[(int32)ProfilerMemory::Groups::Malloc]);
    UPDATE_PEEK(ProfilerMemory::Groups::Malloc);

    stack.SkipRecursion = false;
}

void ProfilerMemory::OnMemoryFree(void* ptr)
{
    ASSERT_LOW_LAYER(Enabled && ptr);
    auto& stack = GroupStack.Get();
    if (stack.SkipRecursion)
        return;
    stack.SkipRecursion = true;

    // Find and remove pointer
    PointerData ptrData;
    PointersLocker.Lock();
    auto it = Pointers.Find(ptr);
    bool found = it.IsNotEnd();
    if (found)
        ptrData = it->Value;
    Pointers.Remove(it); 
    PointersLocker.Unlock(); 

    if (found)
    {
        // Update group memory
        const int64 add = -(int64)ptrData.Size;
        SubGroupMemory((Groups)ptrData.Group, add);
        Platform::InterlockedAdd(&GroupMemory[(int32)ProfilerMemory::Groups::Malloc], add);
        Platform::InterlockedDecrement(&GroupMemoryCount[(int32)ProfilerMemory::Groups::Malloc]);
    }

    stack.SkipRecursion = false;
}

void ProfilerMemory::OnGroupUpdate(Groups group, int64 sizeDelta, int64 countDetla)
{
    Platform::InterlockedAdd(&GroupMemory[(int32)group], sizeDelta);
    Platform::InterlockedAdd(&GroupMemoryCount[(int32)group], countDetla);
    UPDATE_PEEK(group);
}

#endif
