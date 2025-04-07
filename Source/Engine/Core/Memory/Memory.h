// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Templates.h"
#include "Engine/Platform/Platform.h"
#include <new>

#include "CrtAllocator.h"
typedef CrtAllocator Allocator;

namespace AllocatorExt
{
    /// <summary>
    /// Reallocates block of the memory.
    /// </summary>
    /// </summary>
    /// <param name="ptr">A pointer to the memory block to reallocate.</param>
    /// <param name="newSize">The size of the new allocation (in bytes).</param>
    /// <returns>The pointer to the allocated chunk of the memory. The pointer is a multiple of alignment.</returns>
    inline void* Realloc(void* ptr, uint64 newSize)
    {
        if (newSize == 0)
        {
            Allocator::Free(ptr);
            return nullptr;
        }
        if (!ptr)
            return Allocator::Allocate(newSize);
        void* result = Allocator::Allocate(newSize);
        if (result)
        {
            Platform::MemoryCopy(result, ptr, newSize);
            Allocator::Free(ptr);
        }
        return result;
    }

    /// <summary>
    /// Reallocates block of the memory.
    /// </summary>
    /// </summary>
    /// <param name="ptr">A pointer to the memory block to reallocate.</param>
    /// <param name="newSize">The size of the new allocation (in bytes).</param>
    /// <param name="alignment">The memory alignment (in bytes). Must be an integer power of 2.</param>
    /// <returns>The pointer to the allocated chunk of the memory. The pointer is a multiple of alignment.</returns>
    inline void* ReallocAligned(void* ptr, uint64 newSize, uint64 alignment)
    {
        if (newSize == 0)
        {
            Allocator::Free(ptr);
            return nullptr;
        }
        if (!ptr)
            return Allocator::Allocate(newSize, alignment);
        void* result = Allocator::Allocate(newSize, alignment);
        if (result)
        {
            Platform::MemoryCopy(result, ptr, newSize);
            Allocator::Free(ptr);
        }
        return result;
    }

    /// <summary>
    /// Reallocates block of the memory.
    /// </summary>
    /// </summary>
    /// <param name="ptr">A pointer to the memory block to reallocate.</param>
    /// <param name="oldSize">The size of the old allocation (in bytes).</param>
    /// <param name="newSize">The size of the new allocation (in bytes).</param>
    /// <returns>The pointer to the allocated chunk of the memory. The pointer is a multiple of alignment.</returns>
    inline void* Realloc(void* ptr, uint64 oldSize, uint64 newSize)
    {
        if (newSize == 0)
        {
            Allocator::Free(ptr);
            return nullptr;
        }
        if (!ptr)
            return Allocator::Allocate(newSize);
        if (newSize <= oldSize)
            return ptr;
        void* result = Allocator::Allocate(newSize);
        if (result)
        {
            Platform::MemoryCopy(result, ptr, oldSize);
            Allocator::Free(ptr);
        }
        return result;
    }
}

class Memory
{
public:
    /// <summary>
    /// Constructs the item in the memory.
    /// </summary>
    /// <remarks>The optimized version is noop.</remarks>
    /// <param name="dst">The address of the memory location to construct.</param>
    template<typename T>
    FORCE_INLINE static typename TEnableIf<!TIsTriviallyConstructible<T>::Value>::Type ConstructItem(T* dst)
    {
        new(dst) T();
    }

    /// <summary>
    /// Constructs the item in the memory.
    /// </summary>
    /// <remarks>The optimized version is noop.</remarks>
    /// <param name="dst">The address of the memory location to construct.</param>
    template<typename T>
    FORCE_INLINE static typename TEnableIf<TIsTriviallyConstructible<T>::Value>::Type ConstructItem(T* dst)
    {
        // More undefined behavior! No more clean memory! More performance! Yay!
        //Platform::MemoryClear(dst, sizeof(T));
    }

    /// <summary>
    /// Constructs the range of items in the memory.
    /// </summary>
    /// <remarks>The optimized version is noop.</remarks>
    /// <param name="dst">The address of the first memory location to construct.</param>
    /// <param name="count">The number of element to construct. Can be equal 0.</param>
    template<typename T>
    FORCE_INLINE static typename TEnableIf<!TIsTriviallyConstructible<T>::Value>::Type ConstructItems(T* dst, int32 count)
    {
        while (count--)
        {
            new(dst) T();
            ++(T*&)dst;
        }
    }

    /// <summary>
    /// Constructs the range of items in the memory.
    /// </summary>
    /// <remarks>The optimized version is noop.</remarks>
    /// <param name="dst">The address of the first memory location to construct.</param>
    /// <param name="count">The number of element to construct. Can be equal 0.</param>
    template<typename T>
    FORCE_INLINE static typename TEnableIf<TIsTriviallyConstructible<T>::Value>::Type ConstructItems(T* dst, int32 count)
    {
        // More undefined behavior! No more clean memory! More performance! Yay!
        //Platform::MemoryClear(dst, count * sizeof(T));
    }

    /// <summary>
    /// Constructs the range of items in the memory from the set of arguments.
    /// </summary>
    /// <remarks>The optimized version uses low-level memory copy.</remarks>
    /// <param name="dst">The address of the first memory location to construct.</param>
    /// <param name="src">The address of the first memory location to pass to the constructor.</param>
    /// <param name="count">The number of element to construct. Can be equal 0.</param>
    template<typename T, typename U>
    FORCE_INLINE static typename TEnableIf<!TIsBitwiseConstructible<T, U>::Value>::Type ConstructItems(T* dst, const U* src, int32 count)
    {
        while (count--)
        {
            new(dst) T(*src);
            ++(T*&)dst;
            ++src;
        }
    }

    /// <summary>
    /// Constructs the range of items in the memory from the set of arguments.
    /// </summary>
    /// <remarks>The optimized version uses low-level memory copy.</remarks>
    /// <param name="dst">The address of the first memory location to construct.</param>
    /// <param name="src">The address of the first memory location to pass to the constructor.</param>
    /// <param name="count">The number of element to construct. Can be equal 0.</param>
    template<typename T, typename U>
    FORCE_INLINE static typename TEnableIf<TIsBitwiseConstructible<T, U>::Value>::Type ConstructItems(T* dst, const U* src, int32 count)
    {
        Platform::MemoryCopy(dst, src, count * sizeof(U));
    }

    /// <summary>
    /// Destructs the item in the memory.
    /// </summary>
    /// <remarks>The optimized version is noop.</remarks>
    /// <param name="dst">The address of the memory location to destruct.</param>
    template<typename T>
    FORCE_INLINE static typename TEnableIf<!TIsTriviallyDestructible<T>::Value>::Type DestructItem(T* dst)
    {
        dst->~T();
    }

    /// <summary>
    /// Destructs the item in the memory.
    /// </summary>
    /// <remarks>The optimized version is noop.</remarks>
    /// <param name="dst">The address of the memory location to destruct.</param>
    template<typename T>
    FORCE_INLINE static typename TEnableIf<TIsTriviallyDestructible<T>::Value>::Type DestructItem(T* dst)
    {
    }

    /// <summary>
    /// Destructs the range of items in the memory.
    /// </summary>
    /// <remarks>The optimized version is noop.</remarks>
    /// <param name="dst">The address of the first memory location to destruct.</param>
    /// <param name="count">The number of element to destruct. Can be equal 0.</param>
    template<typename T>
    FORCE_INLINE static typename TEnableIf<!TIsTriviallyDestructible<T>::Value>::Type DestructItems(T* dst, int32 count)
    {
        while (count--)
        {
            dst->~T();
            ++dst;
        }
    }

    /// <summary>
    /// Destructs the range of items in the memory.
    /// </summary>
    /// <remarks>The optimized version is noop.</remarks>
    /// <param name="dst">The address of the first memory location to destruct.</param>
    /// <param name="count">The number of element to destruct. Can be equal 0.</param>
    template<typename T>
    FORCE_INLINE static typename TEnableIf<TIsTriviallyDestructible<T>::Value>::Type DestructItems(T* dst, int32 count)
    {
    }

    /// <summary>
    /// Copies the range of items using the assignment operator.
    /// </summary>
    /// <remarks>The optimized version is low-level memory copy.</remarks>
    /// <param name="dst">The address of the first memory location to start assigning to.</param>
    /// <param name="src">The address of the first memory location to assign from.</param>
    /// <param name="count">The number of element to assign. Can be equal 0.</param>
    template<typename T>
    FORCE_INLINE static typename TEnableIf<!TIsTriviallyCopyAssignable<T>::Value>::Type CopyItems(T* dst, const T* src, int32 count)
    {
        while (count--)
        {
            *dst = *src;
            ++dst;
            ++src;
        }
    }

    /// <summary>
    /// Copies the range of items using the assignment operator.
    /// </summary>
    /// <remarks>The optimized version is low-level memory copy.</remarks>
    /// <param name="dst">The address of the first memory location to start assigning to.</param>
    /// <param name="src">The address of the first memory location to assign from.</param>
    /// <param name="count">The number of element to assign. Can be equal 0.</param>
    template<typename T>
    FORCE_INLINE static typename TEnableIf<TIsTriviallyCopyAssignable<T>::Value>::Type CopyItems(T* dst, const T* src, int32 count)
    {
        Platform::MemoryCopy(dst, src, count * sizeof(T));
    }

    /// <summary>
    /// Moves the range of items in the memory from the set of arguments.
    /// </summary>
    /// <remarks>The optimized version uses low-level memory copy.</remarks>
    /// <param name="dst">The address of the first memory location to move.</param>
    /// <param name="src">The address of the first memory location to pass to the move constructor.</param>
    /// <param name="count">The number of element to move. Can be equal 0.</param>
    template<typename T, typename U>
    FORCE_INLINE static typename TEnableIf<!TIsBitwiseConstructible<T, U>::Value>::Type MoveItems(T* dst, U* src, int32 count)
    {
        while (count--)
        {
            new(dst) T((U&&)*src);
            ++(T*&)dst;
            ++src;
        }
    }

    /// <summary>
    /// Moves the range of items in the memory from the set of arguments.
    /// </summary>
    /// <remarks>The optimized version uses low-level memory copy.</remarks>
    /// <param name="dst">The address of the first memory location to move.</param>
    /// <param name="src">The address of the first memory location to pass to the move constructor.</param>
    /// <param name="count">The number of element to move. Can be equal 0.</param>
    template<typename T, typename U>
    FORCE_INLINE static typename TEnableIf<TIsBitwiseConstructible<T, U>::Value>::Type MoveItems(T* dst, U* src, int32 count)
    {
        Platform::MemoryCopy(dst, src, count * sizeof(U));
    }
};

/// <summary>
/// Creates a new object of the given type.
/// </summary>
/// <remarks>When using custom memory allocator ensure to free the data using the same allocator type.</remarks>
/// <returns>The new object instance.</returns>
template<class T, class MemoryAllocator = Allocator>
inline T* New()
{
    T* ptr = (T*)MemoryAllocator::Allocate(sizeof(T));
    Memory::ConstructItem(ptr);
    return ptr;
}

/// <summary>
/// Creates a new object of the given type with the specified parameters.
/// </summary>
/// <remarks>When using custom memory allocator ensure to free the data using the same allocator type.</remarks>
/// <returns>The new object instance.</returns>
template<class T, class MemoryAllocator = Allocator, class... Args>
inline T* New(Args&&...args)
{
    T* ptr = (T*)MemoryAllocator::Allocate(sizeof(T));
    new(ptr) T(Forward<Args>(args)...);
    return ptr;
}

/// <summary>
/// Creates and constructs an array of given elements count.
/// </summary>
/// <remarks>When using custom memory allocator ensure to free the data using the same allocator type.</remarks>
/// <param name="count">The elements count.</param>
/// <returns>The new object instance.</returns>
template<class T, class MemoryAllocator = Allocator>
inline T* NewArray(uint32 count)
{
    T* ptr = (T*)MemoryAllocator::Allocate(sizeof(T) * count);
    Memory::ConstructItems(ptr, count);
    return ptr;
}

/// <summary>
/// Destructs and frees the specified object.
/// </summary>
/// <remarks>When using custom memory allocator ensure to free the data using the same allocator type.</remarks>
/// <param name="ptr">The object pointer.</param>
template<class T, class MemoryAllocator = Allocator>
inline void Delete(T* ptr)
{
    Memory::DestructItem(ptr);
    MemoryAllocator::Free(ptr);
}

/// <summary>
/// Destructs and frees the specified array of objects.
/// </summary>
/// <remarks>When using custom memory allocator ensure to free the data using the same allocator type.</remarks>
/// <param name="ptr">The objects pointer.</param>
/// <param name="count">The objects count.</param>
template<class T, class MemoryAllocator = Allocator>
inline void DeleteArray(T* ptr, uint32 count)
{
    Memory::DestructItems(ptr, count);
    MemoryAllocator::Free(ptr);
}
