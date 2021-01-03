// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Templates.h"
#include "Engine/Core/Memory/Memory.h"
#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Platform/Platform.h"

/// <summary>
/// Helper utility used for sorting data collections.
/// </summary>
class FLAXENGINE_API Sorting
{
public:

    /// <summary>
    /// Helper collection used by the sorting algorithms. Implements stack using single linear allocation with variable capacity.
    /// </summary>
    class SortingStack
    {
    public:

        static SortingStack& Get();

    public:

        int32 _count;
        int32 _capacity;
        int32* _data;

    public:

        /// <summary>
        /// Initializes a new instance of the <see cref="SortingStack"/> class.
        /// </summary>
        SortingStack()
            : _count(0)
            , _capacity(0)
            , _data(nullptr)
        {
        }

        /// <summary>
        /// Finalizes an instance of the <see cref="SortingStack"/> class.
        /// </summary>
        ~SortingStack()
        {
            Allocator::Free(_data);
        }

    public:

        FORCE_INLINE int32 Count() const
        {
            return _count;
        }

        FORCE_INLINE int32 Capacity() const
        {
            return _capacity;
        }

        FORCE_INLINE bool HasItems() const
        {
            return _count > 0;
        }

    public:

        FORCE_INLINE void Clear()
        {
            _count = 0;
        }

        void Push(const int32 item)
        {
            EnsureCapacity(_count + 1);
            _data[_count++] = item;
        }

        int32 Pop()
        {
            ASSERT(_count > 0);
            const int32 item = _data[_count - 1];
            _count--;
            return item;
        }

    public:

        void SetCapacity(const int32 capacity)
        {
            ASSERT(capacity >= 0);

            if (capacity == _capacity)
                return;

            int32* newData = nullptr;
            if (capacity > 0)
            {
                newData = (int32*)Allocator::Allocate(capacity * sizeof(int32));
            }

            if (_data)
            {
                if (newData && _count > 0)
                {
                    for (int32 i = 0; i < _count && i < capacity; i++)
                        newData[i] = _data[i];
                }
                Allocator::Free(_data);
            }

            _data = newData;
            _capacity = capacity;
            _count = _count < _capacity ? _count : _capacity;
        }

        void EnsureCapacity(int32 minCapacity)
        {
            if (_capacity >= minCapacity)
                return;

            int32 num = _capacity == 0 ? 64 : _capacity * 2;
            if (num < minCapacity)
                num = minCapacity;

            SetCapacity(num);
        }
    };

public:

    /// <summary>
    /// Sorts the linear data array using Quick Sort algorithm (non recursive version, uses temporary stack collection).
    /// </summary>
    /// <param name="data">The data pointer.</param>
    /// <param name="count">The elements count.</param>
    template<typename T>
    static void QuickSort(T* data, int32 count)
    {
        if (count < 2)
            return;

        auto& stack = SortingStack::Get();

        // Push left and right
        stack.Push(0);
        stack.Push(count - 1);

        // Keep sorting from stack while is not empty
        while (stack.HasItems())
        {
            // Pop right and left
            int32 right = stack.Pop();
            const int32 left = stack.Pop();

            // Partition
            T* x = &data[right];
            int32 i = left - 1;
            for (int32 j = left; j <= right - 1; j++)
            {
                if (data[j] < *x)
                {
                    i++;
                    Swap(data[i], data[j]);
                }
            }
            Swap(data[i + 1], data[right]);
            const int32 pivot = i + 1;

            // If there are elements on left side of pivot, then push left side to stack
            if (pivot - 1 > left)
            {
                stack.Push(left);
                stack.Push(pivot - 1);
            }

            // If there are elements on right side of pivot, then push right side to stack
            if (pivot + 1 < right)
            {
                stack.Push(pivot + 1);
                stack.Push(right);
            }
        }
    }

    /// <summary>
    /// Sorts the linear data array using Quick Sort algorithm (non recursive version, uses temporary stack collection).
    /// </summary>
    /// <param name="data">The data pointer.</param>
    /// <param name="count">The elements count.</param>
    /// <param name="compare">The custom comparision callback.</param>
    template<typename T>
    static void QuickSort(T* data, int32 count, bool (*compare)(const T& a, const T& b))
    {
        if (count < 2)
            return;

        auto& stack = SortingStack::Get();

        // Push left and right
        stack.Push(0);
        stack.Push(count - 1);

        // Keep sorting from stack while is not empty
        while (stack.HasItems())
        {
            // Pop right and left
            int32 right = stack.Pop();
            const int32 left = stack.Pop();

            // Partition
            T* x = &data[right];
            int32 i = left - 1;
            for (int32 j = left; j <= right - 1; j++)
            {
                if (compare(data[j], *x))
                {
                    i++;
                    Swap(data[i], data[j]);
                }
            }
            Swap(data[i + 1], data[right]);
            const int32 pivot = i + 1;

            // If there are elements on left side of pivot, then push left side to stack
            if (pivot - 1 > left)
            {
                stack.Push(left);
                stack.Push(pivot - 1);
            }

            // If there are elements on right side of pivot, then push right side to stack
            if (pivot + 1 < right)
            {
                stack.Push(pivot + 1);
                stack.Push(right);
            }
        }
    }

    template<typename T, typename U>
    static void SortArray(T* data, int32 count, bool (*compare)(const T& a, const T& b, U* userData), U* userData)
    {
        if (count < 2)
            return;

        auto& stack = SortingStack::Get();

        // Push left and right
        stack.Push(0);
        stack.Push(count - 1);

        // Keep sorting from stack while is not empty
        while (stack.HasItems())
        {
            // Pop right and left
            int32 right = stack.Pop();
            const int32 left = stack.Pop();

            // Partition
            T* x = &data[right];
            int32 i = left - 1;
            for (int32 j = left; j <= right - 1; j++)
            {
                if (compare(data[j], *x, userData))
                {
                    i++;
                    Swap(data[i], data[j]);
                }
            }
            Swap(data[i + 1], data[right]);
            const int32 pivot = i + 1;

            // If there are elements on left side of pivot, then push left side to stack
            if (pivot - 1 > left)
            {
                stack.Push(left);
                stack.Push(pivot - 1);
            }

            // If there are elements on right side of pivot, then push right side to stack
            if (pivot + 1 < right)
            {
                stack.Push(pivot + 1);
                stack.Push(right);
            }
        }
    }

    /// <summary>
    /// Sorts the linear data array using Quick Sort algorithm (non recursive version, uses temporary stack collection). Uses reference to values for sorting. Useful for sorting collection of pointers to objects that implement comparision operator.
    /// </summary>
    /// <param name="data">The data pointer.</param>
    /// <param name="count">The elements count.</param>
    template<typename T>
    static void QuickSortObj(T* data, int32 count)
    {
        if (count < 2)
            return;

        auto& stack = SortingStack::Get();

        // Push left and right
        stack.Push(0);
        stack.Push(count - 1);

        // Keep sorting from stack while is not empty
        while (stack.HasItems())
        {
            // Pop right and left
            int32 right = stack.Pop();
            const int32 left = stack.Pop();

            // Partition
            T x = data[right];
            int32 i = left - 1;
            for (int32 j = left; j <= right - 1; j++)
            {
                if (*data[j] < *x)
                {
                    i++;
                    Swap(data[i], data[j]);
                }
            }
            Swap(data[i + 1], data[right]);
            const int32 pivot = i + 1;

            // If there are elements on left side of pivot, then push left side to stack
            if (pivot - 1 > left)
            {
                stack.Push(left);
                stack.Push(pivot - 1);
            }

            // If there are elements on right side of pivot, then push right side to stack
            if (pivot + 1 < right)
            {
                stack.Push(pivot + 1);
                stack.Push(right);
            }
        }
    }
};
