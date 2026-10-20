// Copyright (c) Wojciech Figat. All rights reserved.

#include "Engine/Foliage/Foliage.h"
#include "Engine/Graphics/RenderContext.h"
#include <ThirdParty/catch2/catch.hpp>

TEST_CASE("Foliage")
{
    SECTION("Test GetBatchDrawModes")
    {
        // A frame with shadows: the main view draws GBuffer and Forward, a shadow view draws Depth
        RenderContextBatch renderContextBatch;
        renderContextBatch.Contexts.AddOne().View.Pass = DrawPass::GBuffer | DrawPass::Forward;
        renderContextBatch.Contexts.AddOne().View.Pass = DrawPass::Depth;

        // A type that only casts shadows is drawn by the shadow view (the main view alone has no Depth pass)
        CHECK(Foliage::GetBatchDrawModes(DrawPass::Depth, ShadowsCastingMode::All, renderContextBatch) == DrawPass::Depth);

        // A visible type that casts shadows is drawn by both views
        CHECK(Foliage::GetBatchDrawModes(DrawPass::Default, ShadowsCastingMode::All, renderContextBatch) == (DrawPass::Depth | DrawPass::GBuffer | DrawPass::Forward));

        // A type without shadows is not drawn by the shadow view
        CHECK(Foliage::GetBatchDrawModes(DrawPass::Depth, ShadowsCastingMode::None, renderContextBatch) == DrawPass::None);
    }
}
