// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "Engine/Core/RandomStream.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Collections/BitArray.h"
#include "Engine/Core/Collections/HashSet.h"
#include "Engine/Core/Collections/Dictionary.h"
#include <ThirdParty/catch2/catch.hpp>

const bool TestBits[] = { true, false, true, false };

template<typename AllocationType = HeapAllocation>
void InitBitArray(BitArray<AllocationType>& array)
{
    array.Add(TestBits, ARRAY_COUNT(TestBits));
}

template<typename AllocationType = HeapAllocation>
void CheckBitArray(const BitArray<AllocationType>& array)
{
    CHECK(array.Count() == ARRAY_COUNT(TestBits));
    for (int32 i = 0; i < ARRAY_COUNT(TestBits); i++)
    {
        CHECK(array[i] == TestBits[i]);
    }
}

TEST_CASE("Array")
{
    SECTION("Test Allocators")
    {
        Array<int32> a1;
        Array<int32, InlinedAllocation<8>> a2;
        Array<int32, FixedAllocation<8>> a3;
        for (int32 i = 0; i < 7; i++)
        {
            a1.Add(i);
            a2.Add(i);
            a3.Add(i);
        }
        CHECK(a1.Count() == 7);
        CHECK(a2.Count() == 7);
        CHECK(a3.Count() == 7);
        for (int32 i = 0; i < 7; i++)
        {
            CHECK(a1[i] == i);
            CHECK(a2[i] == i);
            CHECK(a3[i] == i);
        }
    }

    // Generate some random data for testing
    Array<uint32> testData;
    testData.Resize(32);
    RandomStream rand(101);
    for (int32 i = 0; i < testData.Count(); i++)
        testData[i] = rand.GetUnsignedInt();
    
    SECTION("Test Copy Constructor")
    {
        const Array<uint32> a1(testData);
        const Array<uint32, InlinedAllocation<8>> a2(testData);
        const Array<uint32, InlinedAllocation<64>> a3(testData);
        const Array<uint32, FixedAllocation<64>> a4(testData);
        CHECK(a1 == testData);
        CHECK(a2 == testData);
        CHECK(a3 == testData);
        CHECK(a4 == testData);
    }
    
    SECTION("Test Copy Operator")
    {
        Array<uint32> a1;
        a1 = testData;
        CHECK(a1 == testData);
    }
}

TEST_CASE("BitArray")
{
    SECTION("Test Allocators")
    {
        BitArray<> a1;
        BitArray<InlinedAllocation<8>> a2;
        BitArray<FixedAllocation<8>> a3;
        for (int32 i = 0; i < 7; i++)
        {
            const bool v = i & 2;
            a1.Add(v);
            a2.Add(v);
            a3.Add(v);
        }
        CHECK(a1.Count() == 7);
        CHECK(a2.Count() == 7);
        CHECK(a3.Count() == 7);
        for (int32 i = 0; i < 7; i++)
        {
            const bool v = i & 2;
            CHECK(a1.Get(i) == v);
            CHECK(a2.Get(i) == v);
            CHECK(a3.Get(i) == v);
        }
    }

    SECTION("Test Move/Copy")
    {
        BitArray<> array1;
        BitArray<FixedAllocation<4>> array2;
        BitArray<InlinedAllocation<4>> array3;
        BitArray<InlinedAllocation<2>> array4;
        
        InitBitArray(array1);
        InitBitArray(array2);
        InitBitArray(array3);
        InitBitArray(array4);

        CheckBitArray(array1);
        CheckBitArray(array2);
        CheckBitArray(array3);
        CheckBitArray(array4);

        BitArray<> arrayClone1 = array1;
        BitArray<FixedAllocation<4>> arrayClone2(array1);
        BitArray<FixedAllocation<4>> arrayClone3(MoveTemp(array1));
        BitArray<> arrayClone4(MoveTemp(array1));
        BitArray<FixedAllocation<4>> arrayClone5 = MoveTemp(array2);
        BitArray<InlinedAllocation<4>> arrayClone6 = MoveTemp(array3);
        BitArray<InlinedAllocation<2>> arrayClone7 = MoveTemp(array4);

        CheckBitArray(arrayClone1);
        CheckBitArray(arrayClone2);
        CheckBitArray(arrayClone4);
        CheckBitArray(arrayClone5);
        CheckBitArray(arrayClone6);
        CheckBitArray(arrayClone7);

        CHECK(array1.Count() == 0);
        CHECK(array2.Count() == 0);
        CHECK(array3.Count() == 0);
        CHECK(array4.Count() == 0);
    }

    // Generate some random data for testing
    BitArray<> testData;
    testData.Resize(32);
    RandomStream rand(101);
    for (int32 i = 0; i < testData.Count(); i++)
        testData.Set(i, rand.GetBool());
    
    SECTION("Test Copy Constructor")
    {
        const BitArray<> a1(testData);
        const BitArray<InlinedAllocation<8>> a2(testData);
        const BitArray<InlinedAllocation<64>> a3(testData);
        const BitArray<FixedAllocation<64>> a4(testData);
        CHECK(a1 == testData);
        CHECK(a2 == testData);
        CHECK(a3 == testData);
        CHECK(a4 == testData);
    }
    
    SECTION("Test Copy Operator")
    {
        BitArray<> a1;
        a1 = testData;
        CHECK(a1 == testData);
    }

    SECTION("Test Set All")
    {
        BitArray<> a1;
        a1.Resize(9);
        CHECK(a1.Count() == 9);
        a1.SetAll(true);
        for (int32 i = 0; i < a1.Count(); i++)
            CHECK(a1[i] == true);
        a1.SetAll(false);
        for (int32 i = 0; i < a1.Count(); i++)
            CHECK(a1[i] == false);
    }
}

TEST_CASE("HashSet")
{
    SECTION("Test Allocators")
    {
        HashSet<int32> a1;
        HashSet<int32, InlinedAllocation<DICTIONARY_DEFAULT_CAPACITY>> a2;
        HashSet<int32, FixedAllocation<DICTIONARY_DEFAULT_CAPACITY>> a3;
        for (int32 i = 0; i < 7; i++)
        {
            a1.Add(i);
            a2.Add(i);
            a3.Add(i);
        }
        CHECK(a1.Count() == 7);
        CHECK(a2.Count() == 7);
        CHECK(a3.Count() == 7);
        for (int32 i = 0; i < 7; i++)
        {
            CHECK(a1.Contains(i));
            CHECK(a2.Contains(i));
            CHECK(a3.Contains(i));
        }
    }

    SECTION("Test Resizing")
    {
        HashSet<int32> a1;
        for (int32 i = 0; i < 4000; i++)
            a1.Add(i);
        CHECK(a1.Count() == 4000);
        int32 capacity = a1.Capacity();
        for (int32 i = 0; i < 4000; i++)
        {
            CHECK(a1.Contains(i));
        }
        a1.Clear();
        CHECK(a1.Count() == 0);
        CHECK(a1.Capacity() == capacity);
        for (int32 i = 0; i < 4000; i++)
            a1.Add(i);
        CHECK(a1.Count() == 4000);
        CHECK(a1.Capacity() == capacity);
        for (int32 i = 0; i < 4000; i++)
            a1.Remove(i);
        CHECK(a1.Count() == 0);
        CHECK(a1.Capacity() == capacity);
        for (int32 i = 0; i < 4000; i++)
            a1.Add(i);
        CHECK(a1.Count() == 4000);
        CHECK(a1.Capacity() == capacity);
    }
    
    SECTION("Test Default Capacity")
    {
        HashSet<int32> a1;
        a1.Add(1);
        CHECK(a1.Capacity() <= DICTIONARY_DEFAULT_CAPACITY);
    }

    SECTION("Test Add/Remove")
    {
        HashSet<int32> a1;
        for (int32 i = 0; i < 4000; i++)
        {
            a1.Add(i);
            a1.Remove(i);
        }
        CHECK(a1.Count() == 0);
        CHECK(a1.Capacity() <= DICTIONARY_DEFAULT_CAPACITY);
        a1.Clear();
        for (int32 i = 1; i <= 10; i++)
            a1.Add(-i);
        for (int32 i = 0; i < 4000; i++)
        {
            a1.Add(i);
            a1.Remove(i);
        }
        CHECK(a1.Count() == 10);
        CHECK(a1.Capacity() <= DICTIONARY_DEFAULT_CAPACITY);
    }
}

TEST_CASE("Dictionary")
{
    SECTION("Test Allocators")
    {
        Dictionary<int32, int32> a1;
        Dictionary<int32, int32, InlinedAllocation<DICTIONARY_DEFAULT_CAPACITY>> a2;
        Dictionary<int32, int32, FixedAllocation<DICTIONARY_DEFAULT_CAPACITY>> a3;
        for (int32 i = 0; i < 7; i++)
        {
            a1.Add(i, i);
            a2.Add(i, i);
            a3.Add(i, i);
        }
        CHECK(a1.Count() == 7);
        CHECK(a2.Count() == 7);
        CHECK(a3.Count() == 7);
        for (int32 i = 0; i < 7; i++)
        {
            CHECK(a1.ContainsKey(i));
            CHECK(a2.ContainsKey(i));
            CHECK(a3.ContainsKey(i));
            CHECK(a1.ContainsValue(i));
            CHECK(a2.ContainsValue(i));
            CHECK(a3.ContainsValue(i));
        }
    }

    SECTION("Test Resizing")
    {
        Dictionary<int32, int32> a1;
        for (int32 i = 0; i < 4000; i++)
            a1.Add(i, i);
        CHECK(a1.Count() == 4000);
        int32 capacity = a1.Capacity();
        for (int32 i = 0; i < 4000; i++)
        {
            CHECK(a1.ContainsKey(i));
            CHECK(a1.ContainsValue(i));
        }
        a1.Clear();
        CHECK(a1.Count() == 0);
        CHECK(a1.Capacity() == capacity);
        for (int32 i = 0; i < 4000; i++)
            a1.Add(i, i);
        CHECK(a1.Count() == 4000);
        CHECK(a1.Capacity() == capacity);
        for (int32 i = 0; i < 4000; i++)
            a1.Remove(i);
        CHECK(a1.Count() == 0);
        CHECK(a1.Capacity() == capacity);
        for (int32 i = 0; i < 4000; i++)
            a1.Add(i, i);
        CHECK(a1.Count() == 4000);
        CHECK(a1.Capacity() == capacity);
    }

    SECTION("Test Default Capacity")
    {
        Dictionary<int32, int32> a1;
        a1.Add(1, 1);
        CHECK(a1.Capacity() <= DICTIONARY_DEFAULT_CAPACITY);
    }

    SECTION("Test Add/Remove")
    {
        Dictionary<int32, int32> a1;
        for (int32 i = 0; i < 4000; i++)
        {
            a1.Add(i, i);
            a1.Remove(i);
        }
        CHECK(a1.Count() == 0);
        CHECK(a1.Capacity() <= DICTIONARY_DEFAULT_CAPACITY);
        a1.Clear();
        for (int32 i = 1; i <= 10; i++)
            a1.Add(-i, -i);
        for (int32 i = 0; i < 4000; i++)
        {
            a1.Add(i, i);
            a1.Remove(i);
        }
        CHECK(a1.Count() == 10);
        CHECK(a1.Capacity() <= DICTIONARY_DEFAULT_CAPACITY);
    }
}
