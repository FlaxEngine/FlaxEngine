// Copyright (c) Wojciech Figat. All rights reserved.

#include "Allocation.h"

/// <summary>
/// Allocator that uses pages for stack-based allocs without freeing memory during it's lifetime.
/// </summary>
class ArenaAllocator
{
private:
    struct Page
    {
        void* Memory;
        Page* Next;
        uint32 Offset, Size;
    };

    int32 _pageSize;
    Page* _first = nullptr;

public:
    ArenaAllocator(int32 pageSizeBytes = 1024 * 1024) // 1 MB by default
        : _pageSize(pageSizeBytes)
    {
    }

    ~ArenaAllocator()
    {
        Free();
    }

    // Allocates a chunk of unitialized memory.
    void* Allocate(uint64 size, uint64 alignment = 1);

    // Frees all memory allocations within allocator.
    void Free();

    // Creates a new object within the arena allocator.
    template<class T, class... Args>
    inline T* New(Args&&...args)
    {
        T* ptr = (T*)Allocate(sizeof(T));
        new(ptr) T(Forward<Args>(args)...);
        return ptr;
    }

    // Invokes destructor on values in a dictionary and clears it.
    template<typename Key, typename Value, typename Allocator>
    void ClearDelete(Dictionary<Key, Value, Allocator>& collection)
    {
        for (auto it = collection.Begin(); it.IsNotEnd(); ++it)
            Memory::DestructItem(it->Value);
        collection.Clear();
    }
};
