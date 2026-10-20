// Copyright (c) Wojciech Figat. All rights reserved.

#include "Engine/Renderer/RenderList.h"
#include "Engine/Renderer/DrawCall.h"
#include "Engine/Graphics/RenderContext.h"
#include <ThirdParty/catch2/catch.hpp>

TEST_CASE("RenderList")
{
    SECTION("Test more than 65,535 draw calls")
    {
        // A dense view can put more than 65,535 draw calls or objects into one list: the batch indices must not wrap
        constexpr int32 count = 70000;
        RenderList* list = RenderList::GetFromPool();
        auto& drawCallsList = list->DrawCallsLists[(int32)DrawCallsListType::GBuffer];

        // Draw calls without instances: each one is its own batch (and no material is needed)
        DrawCall drawCall;
        for (int32 i = 0; i < count; i++)
        {
            drawCall.SortKey = (uint64)i;
            drawCallsList.Indices.Add(list->DrawCalls.Add(drawCall));
        }
        RenderContext renderContext;
        list->SortDrawCalls(renderContext, false, DrawCallsListType::GBuffer);
        REQUIRE(drawCallsList.Batches.Count() == count);
        CHECK(drawCallsList.Batches[count - 1].StartIndex == count - 1);
        CHECK(drawCallsList.Batches[count - 1].BatchSize == 1);

        // A batched draw call (eg. from foliage): its instances start after the draw calls in the objects buffer
        BatchedDrawCall batch(list);
        batch.Instances.AddOne().Store(drawCall);
        list->BatchedDrawCalls.Add(MoveTemp(batch));
        list->BuildObjectsBuffer();
        CHECK(list->BatchedDrawCalls.Get()[0].ObjectsStartIndex == count);

        RenderList::ReturnToPool(list);
    }
}
