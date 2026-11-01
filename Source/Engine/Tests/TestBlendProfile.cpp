// Copyright (c) Wojciech Figat. All rights reserved.

#include "Engine/Animations/BlendProfile.h"
#include "Engine/Core/Collections/BitArray.h"
#include <ThirdParty/catch2/catch.hpp>

namespace
{
    // Skeleton: 0 root, 1 pelvis, 2 spine, 3 thigh, 4 foot
    enum
    {
        Root,
        Pelvis,
        Spine,
        Thigh,
        Foot,
        Count
    };

    // Feet profile: the leg nodes are in the mask
    void SetupMask(BitArray<>& mask)
    {
        mask.Resize(Count);
        mask.SetAll(false);
        mask.Set(Thigh, true);
        mask.Set(Foot, true);
    }

    // Weights of a transition of the given duration, at the given time since it started
    void Weights(float duration, float time, AlphaBlendMode mode, const BitArray<>* mask, float scale, float* weights)
    {
        BlendProfile::ComputeWeights(time / duration, mode, mask, scale, Count, weights);
    }
}

TEST_CASE("BlendProfile")
{
    BitArray<> mask;
    SetupMask(mask);

    SECTION("Scale Alpha")
    {
        CHECK(BlendProfile::ScaleAlpha(0.0f, 0.5f) == 0.0f);
        CHECK(BlendProfile::ScaleAlpha(0.25f, 0.5f) == Approx(0.5f));
        CHECK(BlendProfile::ScaleAlpha(0.5f, 0.5f) == 1.0f);
        CHECK(BlendProfile::ScaleAlpha(0.75f, 0.5f) == 1.0f);
        CHECK(BlendProfile::ScaleAlpha(0.3f, 0.25f) == Approx(1.0f));

        // Scale 1 (or more) blends with the rest of the skeleton
        CHECK(BlendProfile::ScaleAlpha(0.4f, 1.0f) == Approx(0.4f));
        CHECK(BlendProfile::ScaleAlpha(0.4f, 3.0f) == Approx(0.4f));

        // Scale 0 switches as soon as the blend starts
        CHECK(BlendProfile::ScaleAlpha(0.0f, 0.0f) == 0.0f);
        CHECK(BlendProfile::ScaleAlpha(0.01f, 0.0f) == 1.0f);

        // Input alpha is clamped
        CHECK(BlendProfile::ScaleAlpha(-1.0f, 0.5f) == 0.0f);
        CHECK(BlendProfile::ScaleAlpha(2.0f, 0.5f) == 1.0f);
    }

    SECTION("Half Time Profile")
    {
        // A 0.2s transition with a 0.5 profile on the legs: the legs are on the new pose at 0.1s, the rest at 0.2s
        float weights[Count];
        const AlphaBlendMode modes[] = { AlphaBlendMode::Linear, AlphaBlendMode::HermiteCubic };
        for (const AlphaBlendMode mode : modes)
        {
            Weights(0.2f, 0.0f, mode, &mask, 0.5f, weights);
            for (int32 i = 0; i < Count; i++)
                CHECK(weights[i] == Approx(0.0f).margin(1e-6f));

            Weights(0.2f, 0.1f, mode, &mask, 0.5f, weights);
            CHECK(weights[Thigh] == Approx(1.0f));
            CHECK(weights[Foot] == Approx(1.0f));
            CHECK(weights[Root] == Approx(0.5f));
            CHECK(weights[Pelvis] == Approx(0.5f));
            CHECK(weights[Spine] == Approx(0.5f));

            Weights(0.2f, 0.2f, mode, &mask, 0.5f, weights);
            for (int32 i = 0; i < Count; i++)
                CHECK(weights[i] == Approx(1.0f));
        }

        // Linear mode midway through the legs' blend
        Weights(0.2f, 0.05f, AlphaBlendMode::Linear, &mask, 0.5f, weights);
        CHECK(weights[Foot] == Approx(0.5f));
        CHECK(weights[Spine] == Approx(0.25f));
    }

    SECTION("Blend Mode Applies Per Node")
    {
        // The curve runs over each node's own progress
        float weights[Count];
        Weights(0.2f, 0.05f, AlphaBlendMode::HermiteCubic, &mask, 0.5f, weights);
        CHECK(weights[Foot] == Approx(AlphaBlend::Process(0.5f, AlphaBlendMode::HermiteCubic)));
        CHECK(weights[Spine] == Approx(AlphaBlend::Process(0.25f, AlphaBlendMode::HermiteCubic)));
    }

    SECTION("No Profile")
    {
        // Without a mask, or with a mask too small for the skeleton, every node blends over the whole duration
        float weights[Count];
        Weights(0.2f, 0.1f, AlphaBlendMode::Linear, nullptr, 0.5f, weights);
        for (int32 i = 0; i < Count; i++)
            CHECK(weights[i] == Approx(0.5f));

        BitArray<> smallMask;
        smallMask.Resize(2);
        smallMask.SetAll(true);
        Weights(0.2f, 0.1f, AlphaBlendMode::Linear, &smallMask, 0.5f, weights);
        for (int32 i = 0; i < Count; i++)
            CHECK(weights[i] == Approx(0.5f));
    }
}
