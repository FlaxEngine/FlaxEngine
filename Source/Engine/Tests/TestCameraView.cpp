// Copyright (c) Wojciech Figat. All rights reserved.

#include "Engine/Core/Math/Matrix.h"
#include "Engine/Core/Math/Quaternion.h"
#include "Engine/Core/Math/Viewport.h"
#include "Engine/Graphics/RenderView.h"
#include "Engine/Level/Actors/Camera.h"
#include <ThirdParty/catch2/catch.hpp>

namespace
{
    // The yaw (in degrees) of a view matrix's forward axis (LookAt keeps it in the third column)
    double GetViewYaw(const Matrix& view)
    {
        return Math::Atan2((double)view.M13, (double)view.M33) * RadiansToDegrees;
    }

    struct YawSteps
    {
        int32 Distinct = 0;
        double MaxError = 0.0;
    };

    // Turns a camera in 0.01 degree steps over 2 degrees and reads the yaw back from its view matrix
    YawSteps MeasureCamera(Camera* camera, const Vector3& position)
    {
        YawSteps result;
        camera->SetPosition(position);
        double last = -1000.0;
        for (int32 i = 0; i <= 200; i++)
        {
            const float yaw = 30.0f + (float)i * 0.01f;
            camera->SetOrientation(Quaternion::Euler(0.0f, yaw, 0.0f));
            Matrix view, projection;
            camera->GetMatrices(view, projection, Viewport(0, 0, 1920, 1080), Vector3::Zero);
            const double measured = GetViewYaw(view);
            if (Math::Abs(measured - last) > 1e-6)
                result.Distinct++;
            last = measured;
            result.MaxError = Math::Max(result.MaxError, Math::Abs(measured - (double)yaw));
        }
        return result;
    }
}

TEST_CASE("CameraView")
{
    SECTION("Test Camera view direction far from the origin")
    {
        // A view direction built as LookAt(position, position + direction) is rounded to the precision of position
        Camera* camera = Camera::Spawn(ScriptingObject::SpawnParams(Guid::New(), Camera::TypeInitializer));
        REQUIRE(camera);
        const YawSteps nearOrigin = MeasureCamera(camera, Vector3(0, 500, 0));
        CHECK(nearOrigin.Distinct == 201);
        CHECK(nearOrigin.MaxError < 0.001);
        const YawSteps far = MeasureCamera(camera, Vector3(407037, 100490, 526640));
        CHECK(far.Distinct == 201);
        CHECK(far.MaxError < 0.001);

        // The view matrix is the same as LookAt (near the origin, where it has no rounding)
        camera->SetPosition(Vector3(1.2f, 0.5f, -3.4f));
        camera->SetOrientation(Quaternion::Euler(10.0f, 30.0f, 0.0f));
        Matrix view, projection;
        camera->GetMatrices(view, projection, Viewport(0, 0, 1920, 1080), Vector3::Zero);
        const Float3 direction = Float3::Transform(Float3::Forward, camera->GetOrientation());
        const Float3 up = Float3::Transform(Float3::Up, camera->GetOrientation());
        const Float3 position = camera->GetPosition();
        Matrix expected;
        Matrix::LookAt(position, position + direction, up, expected);
        for (int32 i = 0; i < 16; i++)
            CHECK(Math::Abs(view.Raw[i] - expected.Raw[i]) < 0.0001f);
        camera->DeleteObjectNow();
    }

    SECTION("Test RenderView::SetProjector view direction far from the origin")
    {
        RenderView view;
        int32 distinct = 0;
        double last = -1000.0, maxError = 0.0;
        for (int32 i = 0; i <= 200; i++)
        {
            const float yaw = 30.0f + (float)i * 0.01f;
            const Float3 direction = Float3::Transform(Float3::Forward, Quaternion::Euler(0.0f, yaw, 0.0f));
            view.SetProjector(10.0f, 1000.0f, Float3(407037, 100490, 526640), direction, Float3::Up, 90.0f);
            const double measured = GetViewYaw(view.View);
            if (Math::Abs(measured - last) > 1e-6)
                distinct++;
            last = measured;
            maxError = Math::Max(maxError, Math::Abs(measured - (double)yaw));
        }
        CHECK(distinct == 201);
        CHECK(maxError < 0.001);
    }
}
