// Copyright (c) Wojciech Figat. All rights reserved.

#include "Engine/Content/Content.h"
#include "Engine/Content/Assets/Model.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Streaming/StreamingHandlers.h"
#include <ThirdParty/catch2/catch.hpp>

TEST_CASE("ModelsStreamingHandler")
{
    SECTION("Test StreamLODs")
    {
        // A model with 3 LODs (one empty mesh each)
        Model* model = Content::CreateVirtualAsset<Model>();
        REQUIRE(model);
        int32 meshesPerLod[] = { 1, 1, 1 };
        REQUIRE(!model->SetupLODs(ToSpan(meshesPerLod, 3)));
        REQUIRE(model->GetLODsCount() == 3);
        ModelsStreamingHandler handler;
        const auto residency = [&](double time)
        {
            return handler.CalculateResidency(model, handler.CalculateTargetQuality(model, time));
        };

        // Not enabled: all LODs
        CHECK(residency(100.0) == 3);

        // Enabled, nothing drawn: the last LOD only
        model->SetStreamLODs(true);
        CHECK(residency(100.0) == 1);

        // LOD 2 drawn: LODs 1 and 2 (one finer than the finest drawn)
        Engine::FrameCount = 5;
        model->OnLODDrawn(2);
        CHECK(residency(100.0) == 2);

        // LOD 1 drawn: all LODs
        Engine::FrameCount = 6;
        model->OnLODDrawn(1);
        CHECK(residency(101.0) == 3);

        // LOD 2 drawn again later, LOD 1 not drawn for over 20 seconds: LODs 1 and 2
        Engine::FrameCount = 7;
        model->OnLODDrawn(2);
        CHECK(residency(110.0) == 3);
        CHECK(residency(121.5) == 2);

        // Nothing drawn for over 20 seconds: the last LOD only
        CHECK(residency(131.0) == 1);

        model->DeleteObject();
    }
}
