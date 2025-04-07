// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

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
    class FLAXENGINE_API SortingStack
    {
    public:
        static SortingStack& Get();

        int32 Count = 0;
        int32 Capacity = 0;
        int32* Data = nullptr;

        SortingStack();
        ~SortingStack();

        void SetCapacity(int32 capacity);
        void EnsureCapacity(int32 minCapacity);

        void Push(const int32 item)
        {
            EnsureCapacity(Count + 1);
            Data[Count++] = item;
        }

        int32 Pop()
        {
            ASSERT(Count > 0);
            const int32 item = Data[Count - 1];
            Count--;
            return item;
        }
    };

private:
    template<typename T>
    static void Merge(T* data, T* tmp, int32 start, int32 mid, int32 end)
    {
        int32 h = start;
        int32 i = start;
        int32 j = mid + 1;

        while (h <= mid && j <= end)
        {
            if (data[h] < data[j])
                tmp[i] = data[h++];
            else
                tmp[i] = data[j++];
            i++;
        }

        if (h > mid)
        {
            for (int32 k = j; k <= end; k++)
                tmp[i++] = data[k];
        }
        else
        {
            for (int32 k = h; k <= mid; k++)
                tmp[i++] = data[k];
        }

        for (int32 k = start; k <= end; k++)
            data[k] = tmp[k];
    }

    template<typename T>
    static void MergeSort(T* data, T* tmp, int32 start, int32 end)
    {
        if (start >= end)
            return;
        const int32 mid = (start + end) / 2;
        MergeSort(data, tmp, start, mid);
        MergeSort(data, tmp, mid + 1, end);
        Merge(data, tmp, start, mid, end);
    }

public:
    /// <summary>
    /// Sorts the linear data array using Quick Sort algorithm (non recursive version, uses temporary stack collection).
    /// </summary>
    /// <param name="data">The data container.</param>
    template<typename T, typename AllocationType = HeapAllocation>
    FORCE_INLINE static void QuickSort(Array<T, AllocationType>& data)
    {
        QuickSort(data.Get(), data.Count());
    }

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
        while (stack.Count)
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
        while (stack.Count)
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
        while (stack.Count != 0)
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
        while (stack.Count)
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

    /// <summary>
    /// Sorts the linear data array using Merge Sort algorithm (recursive version, uses temporary memory).
    /// </summary>
    /// <param name="data">The data pointer.</param>
    /// <param name="count">The elements count.</param>
    /// <param name="tmp">The additional temporary memory buffer for sorting data. If null then will be automatically allocated within this function call.</param>
    template<typename T>
    static void MergeSort(T* data, int32 count, T* tmp = nullptr)
    {
        if (count < 2)
            return;
        const bool alloc = tmp == nullptr;
        if (alloc)
            tmp = (T*)Platform::Allocate(sizeof(T) * count, 16);
        MergeSort(data, tmp, 0, count - 1);
        if (alloc)
            Platform::Free(tmp);
    }

    template<typename T, typename AllocationType = HeapAllocation, typename TempAllocationType = HeapAllocation>
    FORCE_INLINE static void MergeSort(Array<T, AllocationType>& data, Array<T, TempAllocationType>* tmp = nullptr)
    {
        if (tmp)
            tmp->Resize(data.Count());
        MergeSort(data.Get(), data.Count(), tmp ? tmp->Get() : nullptr);
    }

    /// <summary>
    /// Sorts the linear data array using Radix Sort algorithm (uses temporary keys collection).
    /// </summary>
    /// <param name="inputKeys">The data pointer to the input sorting keys array. When this method completes it contains a pointer to the original data or the temporary depending on the algorithm passes count. Use it as a results container.</param>
    /// <param name="inputValues">The data pointer to the input values array. When this method completes it contains a pointer to the original data or the temporary depending on the algorithm passes count. Use it as a results container.</param>
    /// <param name="tmpKeys">The data pointer to the temporary sorting keys array.</param>
    /// <param name="tmpValues">The data pointer to the temporary values array.</param>
    /// <param name="count">The elements count.</param>
    template<typename T, typename U>
    static void RadixSort(T*& inputKeys, U*& inputValues, T* tmpKeys, U* tmpValues, int32 count)
    {
        // Based on: https://github.com/bkaradzic/bx/blob/master/include/bx/inline/sort.inl
        enum
        {
            RADIXSORT_BITS = 11,
            RADIXSORT_HISTOGRAM_SIZE = 1 << RADIXSORT_BITS,
            RADIXSORT_BIT_MASK = RADIXSORT_HISTOGRAM_SIZE - 1
        };
        if (count < 2)
            return;

        T* keys = inputKeys;
        T* tempKeys = tmpKeys;
        U* values = inputValues;
        U* tempValues = tmpValues;

        uint32 histogram[RADIXSORT_HISTOGRAM_SIZE];
        uint16 shift = 0;
        int32 pass = 0;
        for (; pass < 6; pass++)
        {
            Platform::MemoryClear(histogram, sizeof(uint32) * RADIXSORT_HISTOGRAM_SIZE);

            bool sorted = true;
            T key = keys[0];
            T prevKey = key;
            for (int32 i = 0; i < count; i++)
            {
                key = keys[i];
                const uint16 index = (key >> shift) & RADIXSORT_BIT_MASK;
                ++histogram[index];
                sorted &= prevKey <= key;
                prevKey = key;
            }

            if (sorted)
            {
                goto end;
            }

            uint32 offset = 0;
            for (int32 i = 0; i < RADIXSORT_HISTOGRAM_SIZE; ++i)
            {
                const uint32 cnt = histogram[i];
                histogram[i] = offset;
                offset += cnt;
            }

            for (int32 i = 0; i < count; i++)
            {
                const T k = keys[i];
                const uint16 index = (k >> shift) & RADIXSORT_BIT_MASK;
                const uint32 dest = histogram[index]++;
                tempKeys[dest] = k;
                tempValues[dest] = values[i];
            }

            T* const swapKeys = tempKeys;
            tempKeys = keys;
            keys = swapKeys;

            U* const swapValues = tempValues;
            tempValues = values;
            values = swapValues;

            shift += RADIXSORT_BITS;
        }

    end:
        if (pass & 1)
        {
            // Use temporary keys and values as a result
            inputKeys = tmpKeys;
            inputValues = tmpValues;
        }
    }
};
