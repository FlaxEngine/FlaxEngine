// Copyright (c) Wojciech Figat. All rights reserved.

#include "Engine/Scripting/Scripting.h"
#include "Engine/Scripting/ManagedCLR/MClass.h"
#include "Engine/Scripting/ManagedCLR/MMethod.h"
#include "Engine/Scripting/ManagedCLR/MUtils.h"
#include <ThirdParty/catch2/catch.hpp>

TEST_CASE("ManagedAllocations")
{
#if COMPILE_WITH_PROFILER
    SECTION("Test TickProfilerMemory")
    {
        // The main loop calls it on every iteration, so it must not allocate managed memory on each call
        MClass* klass = Scripting::FindClass("FlaxEngine.Tests.TestManagedAllocations");
        REQUIRE(klass);
        MMethod* method = klass->GetMethod("GetAllocatedBytes");
        REQUIRE(method);
        auto getAllocatedBytes = [method]() { return MUtils::Unbox<int64>(method->Invoke(nullptr, nullptr, nullptr)); };
        extern void TickProfilerMemory();
        TickProfilerMemory();

        // The call of the managed method allocates too: measure it without TickProfilerMemory first
        const int64 start = getAllocatedBytes();
        const int64 before = getAllocatedBytes();
        for (int32 i = 0; i < 100; i++)
            TickProfilerMemory();
        const int64 allocated = getAllocatedBytes() - before - (before - start);
        CHECK(allocated < 1000);
    }
#endif
}
