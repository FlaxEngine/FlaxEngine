// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#include "Engine/Core/RandomStream.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Collections/BitArray.h"
#include <ThirdParty/catch2/catch.hpp>

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
}
