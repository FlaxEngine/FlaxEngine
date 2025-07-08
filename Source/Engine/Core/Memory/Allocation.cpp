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
        ProfilerMemory::OnGroupUpdate(ProfilerMemory::Groups::MallocArena, -(int64)page->Size, -1);
#endif
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
        uint64 pageSize = Math::Max<uint64>(_pageSize, size + alignment + sizeof(Page));
#if COMPILE_WITH_PROFILER
        ProfilerMemory::OnGroupUpdate(ProfilerMemory::Groups::MallocArena, (int64)pageSize, 1);
#endif
        page = (Page*)Allocator::Allocate(pageSize);
        page->Next = _first;
        page->Offset = sizeof(Page);
        page->Size = (uint32)pageSize;
        _first = page;
    }

    // Allocate within a page
    page->Offset = Math::AlignUp(page->Offset, (uint32)alignment);
    void* mem = (byte*)page + page->Offset;
    page->Offset += (uint32)size;

    return mem;
}

void ConcurrentArenaAllocator::Free()
{
    _locker.Lock();

    // Free all pages
    Page* page = (Page*)_first;
    while (page)
    {
#if COMPILE_WITH_PROFILER
        ProfilerMemory::OnGroupUpdate(ProfilerMemory::Groups::MallocArena, -(int64)page->Size, -1);
#endif
        Page* next = page->Next;
        if (_free1)
            _free1(page);
        else
            _free2(page, page->Size);
        page = next;
    }

    // Unlink
    _first = 0;
    _totalBytes = 0;

    _locker.Unlock();
}

void* ConcurrentArenaAllocator::Allocate(uint64 size, uint64 alignment)
{
RETRY:

    // Check if the current page has some space left
    Page* page = (Page*)Platform::AtomicRead(&_first);
    if (page)
    {
        int64 offset = Platform::AtomicRead(&page->Offset);
        int64 offsetAligned = Math::AlignUp(offset, (int64)alignment);
        int64 end = offsetAligned + size;
        if (end <= page->Size)
        {
            // Try to allocate within a page
            if (Platform::InterlockedCompareExchange(&page->Offset, end, offset) != offset)
            {
                // Someone else changed allocated so retry (new offset might mismatch alignment)
                goto RETRY;
            }
            Platform::InterlockedAdd(&_totalBytes, (int64)size);
            return (byte*)page + offsetAligned;
        }
    }

    // Page allocation is thread-synced
    _locker.Lock();

    // Check if page was unchanged by any other thread
    if ((Page*)Platform::AtomicRead(&_first) == page)
    {
        uint64 pageSize = Math::Max<uint64>(_pageSize, size + alignment + sizeof(Page));
#if COMPILE_WITH_PROFILER
        ProfilerMemory::OnGroupUpdate(ProfilerMemory::Groups::MallocArena, (int64)pageSize, 1);
#endif
        page = (Page*)(_allocate1 ? _allocate1(pageSize, 16) : _allocate2(pageSize));
        page->Next = (Page*)_first;
        page->Offset = sizeof(Page);
        page->Size = (int64)pageSize;
        Platform::AtomicStore(&_first, (intptr)page);
    }

    _locker.Unlock();

    // Use a single cde for allocation
    goto RETRY;
}
