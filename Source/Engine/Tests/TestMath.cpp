// Copyright (c) Wojciech Figat. All rights reserved.

#include "Engine/Core/Math/Matrix.h"
#include "Engine/Core/Math/Packed.h"
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Math/Vector4.h"
#include "Engine/Core/Math/Quaternion.h"
#include "Engine/Core/Math/Transform.h"
#include "Engine/Core/RandomStream.h"
#include <ThirdParty/catch2/catch.hpp>

static Quaternion RotationX(float angle)
{
    const float halfAngle = angle * 0.5f;
    return Quaternion(Math::Sin(halfAngle), 0.0f, 0.0f, Math::Cos(halfAngle));
}

TEST_CASE("Math")
{
    SECTION("Test")
    {
        CHECK(Math::RoundUpToPowerOf2(0) == 0);
        CHECK(Math::RoundUpToPowerOf2(1) == 1);
        CHECK(Math::RoundUpToPowerOf2(2) == 2);
        CHECK(Math::RoundUpToPowerOf2(3) == 4);
        CHECK(Math::RoundUpToPowerOf2(4) == 4);
        CHECK(Math::RoundUpToPowerOf2(5) == 8);
        CHECK(Math::RoundUpToPowerOf2(6) == 8);
        CHECK(Math::RoundUpToPowerOf2(7) == 8);
        CHECK(Math::RoundUpToPowerOf2(8) == 8);
        CHECK(Math::RoundUpToPowerOf2(9) == 16);
        CHECK(Math::RoundUpToPowerOf2(10) == 16);
        CHECK(Math::RoundUpToPowerOf2(11) == 16);
        CHECK(Math::RoundUpToPowerOf2(12) == 16);
        CHECK(Math::RoundUpToPowerOf2(13) == 16);
        CHECK(Math::RoundUpToPowerOf2(14) == 16);
        CHECK(Math::RoundUpToPowerOf2(15) == 16);
        CHECK(Math::RoundUpToPowerOf2(16) == 16);
        CHECK(Math::RoundUpToPowerOf2(17) == 32);
    }
}

TEST_CASE("FloatR10G10B10A2")
{
    SECTION("Test Conversion")
    {
        CHECK(Float4::NearEqual(Float4::Zero, FloatR10G10B10A2(Float4::Zero).ToFloat4()));
        CHECK(Float4::NearEqual(Float4::One, FloatR10G10B10A2(Float4::One).ToFloat4()));
        CHECK(Float4::NearEqual(Float4(0.5004888f, 0.5004888f, 0.5004888f, 0.666667f), FloatR10G10B10A2(Vector4(0.5f)).ToFloat4()));
        CHECK(Float4::NearEqual(Float4(1, 0, 0, 0), FloatR10G10B10A2(Float4(1, 0, 0, 0)).ToFloat4()));
        CHECK(Float4::NearEqual(Float4(0, 1, 0, 0), FloatR10G10B10A2(Float4(0, 1, 0, 0)).ToFloat4()));
        CHECK(Float4::NearEqual(Float4(0, 0, 1, 0), FloatR10G10B10A2(Float4(0, 0, 1, 0)).ToFloat4()));
        CHECK(Float4::NearEqual(Float4(0, 0, 0, 1), FloatR10G10B10A2(Float4(0, 0, 0, 1)).ToFloat4()));
    }
}

TEST_CASE("FloatR11G11B10")
{
    SECTION("Test Conversion")
    {
        CHECK(Float3::NearEqual(Float3::Zero, FloatR11G11B10(Float3::Zero).ToFloat3()));
        CHECK(Float3::NearEqual(Float3::One, FloatR11G11B10(Float3::One).ToFloat3()));
        CHECK(Float3::NearEqual(Float3(0.5f, 0.5f, 0.5f), FloatR11G11B10(Float3(0.5f)).ToFloat3()));
        CHECK(Float3::NearEqual(Float3(1, 0, 0), FloatR11G11B10(Float3(1, 0, 0)).ToFloat3()));
        CHECK(Float3::NearEqual(Float3(0, 1, 0), FloatR11G11B10(Float3(0, 1, 0)).ToFloat3()));
        CHECK(Float3::NearEqual(Float3(0, 0, 1), FloatR11G11B10(Float3(0, 0, 1)).ToFloat3()));
        CHECK(Float3::NearEqual(Float3(10, 11, 12), FloatR11G11B10(Float3(10, 11, 12)).ToFloat3()));
    }
}

TEST_CASE("Quaternion")
{
    SECTION("Test Euler")
    {
        CHECK(Quaternion::NearEqual(Quaternion::Euler(90, 0, 0), Quaternion(0.7071068f, 0, 0, 0.7071068f)));
        CHECK(Quaternion::NearEqual(Quaternion::Euler(25, 0, 10), Quaternion(0.215616f, -0.018864f, 0.0850898f, 0.9725809f)));
        CHECK(Float3::NearEqual(Float3(25, 0, 10), Quaternion::Euler(25, 0, 10).GetEuler()));
        CHECK(Float3::NearEqual(Float3(25, -5, 10), Quaternion::Euler(25, -5, 10).GetEuler()));
    }
    SECTION("Test Multiply")
    {
        auto q = Quaternion::Identity;
        auto delta = Quaternion::Euler(0, 10, 0);
        for (int i = 0; i < 9; i++)
            q *= delta;
        CHECK(Quaternion::NearEqual(Quaternion::Euler(0, 90, 0), q, 0.00001f));
    }
}

TEST_CASE("Transform")
{
    SECTION("Test World Matrix")
    {
        Transform t1(Vector3(10, 1, 10), Quaternion::Euler(45, 0, -15), Float3(1.5f, 0.5f, 0.1f));

        Matrix a1 = t1.GetWorld();

        Matrix a2;
        {
            Matrix m1, m2;
            Matrix::Scaling(t1.Scale, a2);
            Matrix::RotationQuaternion(t1.Orientation, m2);
            Matrix::Multiply(a2, m2, m1);
            Matrix::Translation(t1.Translation, m2);
            Matrix::Multiply(m1, m2, a2);
        }

        Matrix a3;
        Matrix::Transformation(t1.Scale, t1.Orientation, t1.Translation, a3);

        CHECK(a1 == a2);
        CHECK(a1 == a3);
    }
    SECTION("Test Local To World")
    {
        Transform t1(Vector3(10, 1, 10), Quaternion::Euler(45, 0, -15), Float3(1.5f, 0.5f, 0.1f));
        Transform t2(Vector3(0, 20, 0), Quaternion::Euler(0, 0, 15), Float3(1.0f, 2.0f, 1.0f));

        Transform a1 = t1.LocalToWorld(t2);
        Vector3 a2 = t1.LocalToWorld(t2.Translation);

        Vector3 a3;
        {
            Vector3 result;
            Matrix scale, rotation, scaleRotation;
            Matrix::Scaling(t1.Scale, scale);
            Matrix::RotationQuaternion(t1.Orientation, rotation);
            Matrix::Multiply(scale, rotation, scaleRotation);
            Vector3::Transform(t2.Translation, scaleRotation, result);
            a3 = result + t1.Translation;
        }

        Vector3 a4T[1];
        Vector3 a4Ta[1] = { t2.Translation };
        for (int32 i = 0; i < ARRAY_COUNT(a4Ta); i++)
            a4T[i] = t1.LocalToWorld(a4Ta[i]);
        Vector3 a4 = a4T[0];

        CHECK(Float3::NearEqual(a1.Translation, a2));
        CHECK(Float3::NearEqual(a2, a3));
        CHECK(Float3::NearEqual(a2, a4));
    }
    SECTION("Test World To Local")
    {
        Transform t1 = Transform(Vector3(10, 1, 10), Quaternion::Euler(45, 0, -15), Float3(1.5f, 0.5f, 0.1f));
        Transform t2 = Transform(Vector3(0, 20, 0), Quaternion::Euler(0, 0, 15), Float3(1.0f, 2.0f, 1.0f));

        Transform a1 = t1.WorldToLocal(t2);
        Vector3 a2 = t1.WorldToLocal(t2.Translation);

        Vector3 a3;
        {
            Matrix scale, rotation, scaleRotation;
            Matrix::Scaling(t1.Scale, scale);
            Matrix::RotationQuaternion(t1.Orientation, rotation);
            Matrix::Multiply(scale, rotation, scaleRotation);
            Matrix::Invert(scaleRotation, scale);
            a3 = t2.Translation - t1.Translation;
            Vector3::Transform(a3, scale, a3);
        }

        Vector3 a4T[1];
        Vector3 a4Ta[1] = { t2.Translation };
        for (int32 i = 0; i < ARRAY_COUNT(a4Ta); i++)
            a4T[i] = t1.WorldToLocal(a4Ta[i]);
        Vector3 a4 = a4T[0];

        CHECK(Float3::NearEqual(a1.Translation, a2));
        CHECK(Float3::NearEqual(a2, a3, 0.0001f));
        CHECK(Float3::NearEqual(a2, a4));
    }
    SECTION("Test World Local Space")
    {
        Transform trans = Transform(Vector3(1, 2, 3));

        CHECK(Float3::NearEqual(Vector3(1, 2, 3), trans.LocalToWorld(Vector3(0, 0, 0))));
        CHECK(Float3::NearEqual(Vector3(4, 4, 4), trans.LocalToWorld(Vector3(3, 2, 1))));
        CHECK(Float3::NearEqual(Vector3(-1, -2, -3), trans.WorldToLocal(Vector3(0, 0, 0))));
        CHECK(Float3::NearEqual(Vector3(0, 0, 0), trans.WorldToLocal(Vector3(1, 2, 3))));

        trans = Transform(Vector3::Zero, Quaternion::Euler(0, 90, 0));
        CHECK(Float3::NearEqual(Vector3(0, 2, -1), trans.LocalToWorld(Vector3(1, 2, 0))));

        trans.Translation = Vector3(1, 0, 0);
        trans.Orientation = RotationX(PI * 0.5f);
        trans.Scale = Vector3(2, 2, 2);
        CHECK(Float3::NearEqual(Vector3(1, 0, 2), trans.LocalToWorld(Vector3(0, 1, 0))));

        Transform t1 = trans.LocalToWorld(Transform::Identity);
        CHECK(Float3::NearEqual(Vector3(1.0f, 0, 0), t1.Translation));
        CHECK(Quaternion::NearEqual(RotationX(PI * 0.5f), t1.Orientation));
        CHECK(Float3::NearEqual(Vector3(2.0f, 2.0f, 2.0f), t1.Scale));

        Transform t2 = trans.WorldToLocal(Transform::Identity);
        CHECK(Float3::NearEqual(Vector3(-0.5f, 0, 0), t2.Translation));
        CHECK(Quaternion::NearEqual(RotationX(PI * -0.5f), t2.Orientation));
        CHECK(Float3::NearEqual(Vector3(0.5f, 0.5f, 0.5f), t2.Scale));

        RandomStream rand(10);
        for (int32 i = 0; i < 10; i++)
        {
            Transform a = Transform(rand.GetVector3(), Quaternion::Euler((float)i * 10, 0, (float)i), rand.GetVector3() * 10.0f);
            Transform b = Transform(rand.GetVector3(), Quaternion::Euler((float)i, 1, 22), rand.GetVector3() * 0.3f);

            Transform ab = a.LocalToWorld(b);
            Transform ba = a.WorldToLocal(ab);

            CHECK(Transform::NearEqual(b, ba, 0.00001f));
        }
    }
    SECTION("Test Add Subtract")
    {
        RandomStream rand(10);
        for (int32 i = 0; i < 10; i++)
        {
            Transform a = Transform(rand.GetVector3(), Quaternion::Euler((float)i * 10, 0, (float)i), rand.GetVector3() * 10.0f);
            Transform b = Transform(rand.GetVector3(), Quaternion::Euler((float)i, 1, 22), rand.GetVector3() * 0.3f);

            Transform ab = a + b;
            Transform newA = ab - b;
            CHECK(Transform::NearEqual(a, newA, 0.00001f));

            Transform ba = b + a;
            Transform newB = ba - a;
            CHECK(Transform::NearEqual(b, newB, 0.00001f));
        }
    }
}
