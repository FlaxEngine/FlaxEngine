// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Memory/BumpFastAllocation.h"
// #include "Engine/Core/UniquePtr.h"
#include <catch2/catch.hpp>

// TEST_CASE("HeapAllocation/UPtr")
// {
//     const UPtr<int32> ptr0;
//     ASSERT(ptr0.IsEmpty());
// 
//     const UPtr<int32> ptr1{ 5 };
//     ASSERT(!ptr1.IsEmpty());
// }

TEST_CASE("BumpFastAllocation/Array")
{
    constexpr uintptr capacity = 1024; // 1 KiB
    constexpr uintptr alignment = sizeof(byte*);

    BumpFastAllocation::Context context{ capacity, alignment };
    CHECK(context.GetUsed() == 0);

    {
        Array<int32, BumpFastAllocation> array{ 8, context };
        array.Add(1); //TODO(mtszkarbowiak) <<ASAP>> Fix zero-size non-zero capacity allocation.
        const uintptr used1 = context.GetUsed();
        CHECK(used1 > 0);

        array.Resize(16);
        const uintptr used2 = context.GetUsed();
        CHECK(used2 > used1);

        // Array goes out of scope.
    }

    context.Reset();
    CHECK(context.GetUsed() == 0);
}

// TEST_CASE("BumpFastAllocation/UPtr")
// {
//     using ThePtr = UPtr<int32, BumpFastAllocation>;
// 
//     constexpr uintptr capacity = 1024; // 1 KiB
//     constexpr uintptr alignment = sizeof(byte*);
// 
//     BumpFastAllocation::Context context{ capacity, alignment };
//     ASSERT(context.GetUsed() == 0);
// 
//     {
//         ThePtr ptr{ 69, context };
//         const uintptr used1 = context.GetUsed();
//         ASSERT(used1 > 0);
// 
//         ptr = ThePtr{ 420, context };
//         const uintptr used2 = context.GetUsed();
//         ASSERT(used2 > used1);
//     }
// 
//     context.Reset();
//     ASSERT(context.GetUsed() == 0);
// }
