// Copyright (c) Wojciech Figat. All rights reserved.

#include "ArenaAllocation.h"
#include "../Math/Math.h"
#include "Engine/Profiler/ProfilerMemory.h"

void ArenaAllocator::Free()
{
    // Free all pages
    Page* page = _first;
    while (page)
    {
#if COMPILE_WITH_PROFILER
        ProfilerMemory::OnGroupUpdate(ProfilerMemory::Groups::MallocArena, -page->Size, -1);
#endif
        Allocator::Free(page->Memory);
        Page* next = page->Next;
        Allocator::Free(page);
        page = next;
    }

    // Unlink
    _first = nullptr;
}

void* ArenaAllocator::Allocate(uint64 size, uint64 alignment)
{
    // Find the first page that has some space left
    Page* page = _first;
    while (page && page->Offset + size + alignment > page->Size)
        page = page->Next;

    // Create a new page if need to
    if (!page)
    {
        uint64 pageSize = Math::Max<uint64>(_pageSize, size);
#if COMPILE_WITH_PROFILER
        ProfilerMemory::OnGroupUpdate(ProfilerMemory::Groups::MallocArena, pageSize, 1);
#endif
        page = (Page*)Allocator::Allocate(sizeof(Page));
        page->Memory = Allocator::Allocate(pageSize);
        page->Next = _first;
        page->Offset = 0;
        page->Size = (uint32)pageSize;
        _first = page;
    }

    // Allocate within a page
    page->Offset = Math::AlignUp(page->Offset, (uint32)alignment);
    void* mem = (byte*)page->Memory + page->Offset;
    page->Offset += (uint32)size;

    return mem;
}