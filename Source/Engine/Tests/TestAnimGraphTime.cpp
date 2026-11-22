// Copyright (c) Wojciech Figat. All rights reserved.

#include "Engine/Animations/Graph/AnimGraphTime.h"
#include "Engine/Core/Math/Math.h"
#include <ThirdParty/catch2/catch.hpp>

TEST_CASE("AnimGraphTime")
{
    SECTION("Played by its start position past its end, it samples the times it's given after")
    {
        // An animation node at a speed of 0, its time given each update as its start position (explicit time playback)
        const float length = 1.4f;
        float timePos = 0.0f;
        CHECK(Math::NearEqual(AnimGraphTime::GetAnimPos(timePos, 0.5f, 1.0f, false, length), 0.5f));
        CHECK(Math::NearEqual(AnimGraphTime::GetAnimPos(timePos, 1.5f, 1.0f, false, length), length));
        CHECK(Math::NearEqual(AnimGraphTime::GetAnimPos(timePos, 0.2f, 1.0f, false, length), 0.2f));
        CHECK(Math::NearEqual(AnimGraphTime::GetAnimPos(timePos, 1.0f, 1.0f, false, length), 1.0f));
    }

    SECTION("Before its start, the same")
    {
        const float length = 1.4f;
        float timePos = 0.0f;
        CHECK(Math::NearEqual(AnimGraphTime::GetAnimPos(timePos, -0.5f, 1.0f, false, length), 0.0f));
        CHECK(Math::NearEqual(AnimGraphTime::GetAnimPos(timePos, 0.3f, 1.0f, false, length), 0.3f));
    }

    SECTION("Played on its own, it ends at its end and stays there")
    {
        const float length = 1.0f;
        float timePos = 0.0f;
        timePos += 0.8f;
        CHECK(Math::NearEqual(AnimGraphTime::GetAnimPos(timePos, 0.0f, 1.0f, false, length), 0.8f));
        timePos += 0.4f;
        CHECK(Math::NearEqual(AnimGraphTime::GetAnimPos(timePos, 0.0f, 1.0f, false, length), length));
        timePos += 0.4f;
        CHECK(Math::NearEqual(AnimGraphTime::GetAnimPos(timePos, 0.0f, 1.0f, false, length), length));
    }

    SECTION("Looping, it wraps")
    {
        const float length = 1.0f;
        float timePos = 1.25f;
        CHECK(Math::NearEqual(AnimGraphTime::GetAnimPos(timePos, 0.0f, 1.0f, true, length), 0.25f));
    }

    SECTION("At its end it shows its last frame, not its first")
    {
        // A 1.5 s animation at 30 frames a second: 45 frames
        CHECK(Math::NearEqual(AnimGraphTime::GetAnimSamplePos(1.5f, 1.5f, 30.0f, 1.5f), 45.0f));
        CHECK(Math::NearEqual(AnimGraphTime::GetAnimSamplePos(1.5f, 1.5f, 30.0f, 0.0f), 0.0f));
        CHECK(Math::NearEqual(AnimGraphTime::GetAnimSamplePos(1.5f, 1.5f, 30.0f, 0.75f), 22.5f));
        // Fitted twice into a node twice its length: three quarters through is halfway into its second cycle
        CHECK(Math::NearEqual(AnimGraphTime::GetAnimSamplePos(3.0f, 1.5f, 30.0f, 2.25f), 22.5f));
        CHECK(Math::NearEqual(AnimGraphTime::GetAnimSamplePos(3.0f, 1.5f, 30.0f, 3.0f), 45.0f));
    }
}
