// Copyright (c) Wojciech Figat. All rights reserved.

#include "Engine/XR/OpenXR/OpenXRConvert.h"
#include <ThirdParty/catch2/catch.hpp>

static XrQuaternionf RotationAxis(float x, float y, float z, float angle)
{
    const float halfAngle = angle * 0.5f;
    const float sin = Math::Sin(halfAngle);
    return { x * sin, y * sin, z * sin, Math::Cos(halfAngle) };
}

TEST_CASE("OpenXR")
{
    SECTION("Test Position")
    {
        CHECK(Float3::NearEqual(OpenXRConvert::ToPosition({ 0.0f, 0.0f, 0.0f }), Float3::Zero));
        CHECK(Float3::NearEqual(OpenXRConvert::ToPosition({ 1.0f, 2.0f, 3.0f }), Float3(100.0f, 200.0f, -300.0f)));

        // Forward, up and right in meters
        CHECK(Float3::NearEqual(OpenXRConvert::ToPosition({ 0.0f, 0.0f, -1.0f }), Float3::Forward * 100.0f));
        CHECK(Float3::NearEqual(OpenXRConvert::ToPosition({ 0.0f, 1.0f, 0.0f }), Float3::Up * 100.0f));
        CHECK(Float3::NearEqual(OpenXRConvert::ToPosition({ 1.0f, 0.0f, 0.0f }), Float3::Right * 100.0f));
    }
    SECTION("Test Orientation")
    {
        // Identity looks forward
        const Quaternion identity = OpenXRConvert::ToOrientation({ 0.0f, 0.0f, 0.0f, 1.0f });
        CHECK(Float3::NearEqual(identity * Float3::Forward, Float3::Forward));
        CHECK(Float3::NearEqual(identity * Float3::Up, Float3::Up));

        // Turning head right (negative rotation around Y axis in OpenXR) looks right
        const Quaternion yawRight = OpenXRConvert::ToOrientation(RotationAxis(0.0f, 1.0f, 0.0f, -PI_OVER_2));
        CHECK(Float3::NearEqual(yawRight * Float3::Forward, Float3::Right));
        CHECK(Float3::NearEqual(yawRight * Float3::Up, Float3::Up));

        // Looking up (positive rotation around X axis in OpenXR) looks up
        const Quaternion pitchUp = OpenXRConvert::ToOrientation(RotationAxis(1.0f, 0.0f, 0.0f, PI_OVER_2));
        CHECK(Float3::NearEqual(pitchUp * Float3::Forward, Float3::Up));
        CHECK(Float3::NearEqual(pitchUp * Float3::Up, Float3::Backward));

        // Tilting head right (negative rotation around Z axis in OpenXR) moves up vector to the right
        const Quaternion rollRight = OpenXRConvert::ToOrientation(RotationAxis(0.0f, 0.0f, 1.0f, -PI_OVER_2));
        CHECK(Float3::NearEqual(rollRight * Float3::Forward, Float3::Forward));
        CHECK(Float3::NearEqual(rollRight * Float3::Up, Float3::Right));
    }
    SECTION("Test Projection")
    {
        const float nearPlane = 10.0f, farPlane = 1000.0f;

        // Symmetric field of view matches the regular perspective projection
        Matrix projection, expected;
        OpenXRConvert::ToProjection({ -PI_OVER_4, PI_OVER_4, PI_OVER_4, -PI_OVER_4 }, nearPlane, farPlane, projection);
        Matrix::PerspectiveFov(PI_OVER_2, 1.0f, nearPlane, farPlane, expected);
        for (int32 i = 0; i < 16; i++)
            CHECK(Math::NearEqual(projection.Raw[i], expected.Raw[i]));

        // Asymmetric field of view edges are projected onto the viewport edges
        const XrFovf fov = { -0.8f, 0.5f, 0.6f, -0.7f };
        OpenXRConvert::ToProjection(fov, nearPlane, farPlane, projection);
        const float z = 100.0f;
        const auto projectX = [&](float x) { return (x * projection.M11 + z * projection.M31) / (z * projection.M34); };
        const auto projectY = [&](float y) { return (y * projection.M22 + z * projection.M32) / (z * projection.M34); };
        CHECK(Math::NearEqual(projectX(z * Math::Tan(fov.angleLeft)), -1.0f));
        CHECK(Math::NearEqual(projectX(z * Math::Tan(fov.angleRight)), 1.0f));
        CHECK(Math::NearEqual(projectY(z * Math::Tan(fov.angleDown)), -1.0f));
        CHECK(Math::NearEqual(projectY(z * Math::Tan(fov.angleUp)), 1.0f));
    }
}
