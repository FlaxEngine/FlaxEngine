// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Memory/Memory.h"
#include "Engine/Core/Types/BaseTypes.h"

class RendererAllocation
{
public:
    static FLAXENGINE_API void* Allocate(uintptr size);
    static FLAXENGINE_API void Free(void* ptr, uintptr size);

    enum { HasSwap = true };

    template<typename T>
    class Data
    {
        T* _data = nullptr;
        uintptr _size;

    public:
        FORCE_INLINE Data()
        {
        }

        FORCE_INLINE ~Data()
        {
            if (_data)
                RendererAllocation::Free(_data, _size);
        }

        FORCE_INLINE T* Get()
        {
            return _data;
        }

        FORCE_INLINE const T* Get() const
        {
            return _data;
        }

        FORCE_INLINE int32 CalculateCapacityGrow(int32 capacity, int32 minCapacity) const
        {
            capacity = capacity ? capacity * 2 : 64;
            if (capacity < minCapacity)
                capacity = minCapacity;
            return capacity;
        }

        FORCE_INLINE void Allocate(uint64 capacity)
        {
            _size = capacity * sizeof(T);
            _data = (T*)RendererAllocation::Allocate(_size);
        }

        FORCE_INLINE void Relocate(uint64 capacity, int32 oldCount, int32 newCount)
        {
            T* newData = capacity != 0 ? (T*)RendererAllocation::Allocate(capacity * sizeof(T)) : nullptr;
            if (oldCount)
            {
                if (newCount > 0)
                    Memory::MoveItems(newData, _data, newCount);
                Memory::DestructItems(_data, oldCount);
            }
            if (_data)
                RendererAllocation::Free(_data, _size);
            _data = newData;
            _size = capacity * sizeof(T);
        }

        FORCE_INLINE void Free()
        {
            if (_data)
            {
                RendererAllocation::Free(_data, _size);
                _data = nullptr;
            }
        }

        FORCE_INLINE void Swap(Data& other)
        {
            ::Swap(_data, other._data);
            ::Swap(_size, other._size);
        }
    };
};
