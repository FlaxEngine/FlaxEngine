// Copyright (c) Wojciech Figat. All rights reserved.

#include "Engine/Renderer/RenderList.h"
#include "Engine/Renderer/DrawCall.h"
#include <ThirdParty/catch2/catch.hpp>

TEST_CASE("RenderList")
{
    SECTION("Test more than 65,535 objects of batched draw calls")
    {
        // Batched draw calls (eg. from foliage) put their instances into the objects buffer: a few draw calls can hold
        // more than 65,535 instances, so the objects start index must not wrap
        constexpr int32 instances = 70000;
        RenderList* list = RenderList::GetFromPool();
        DrawCall drawCall;

        BatchedDrawCall first(list);
        for (int32 i = 0; i < instances; i++)
            first.Instances.AddOne().Store(drawCall);
        list->BatchedDrawCalls.Add(MoveTemp(first));
        BatchedDrawCall second(list);
        second.Instances.AddOne().Store(drawCall);
        list->BatchedDrawCalls.Add(MoveTemp(second));

        list->BuildObjectsBuffer();
        CHECK(list->BatchedDrawCalls.Get()[0].ObjectsStartIndex == 0);
        CHECK(list->BatchedDrawCalls.Get()[1].ObjectsStartIndex == instances);

        RenderList::ReturnToPool(list);
    }
}
