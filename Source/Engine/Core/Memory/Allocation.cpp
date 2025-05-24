// Copyright (c) Wojciech Figat. All rights reserved.

#include "ArenaAllocation.h"
#include "../Math/Math.h"

void ArenaAllocator::Free()
{
    // Free all pages
    Page* page = _first;
    while (page)
    {
        Allocator::Free(page->Memory);
        Page* next = page->Next;
        Allocator::Free(page);
        page = next;
    }
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
        page = (Page*)Allocator::Allocate(sizeof(Page));
        page->Memory = Allocator::Allocate(pageSize);
        page->Next = _first;
        page->Offset = 0;
        page->Size = pageSize;
        _first = page;
    }

    // Allocate within a page
    page->Offset = Math::AlignUp(page->Offset, (uint32)alignment);
    void* mem = (byte*)page->Memory + page->Offset;
    page->Offset += size;

    return mem;
}