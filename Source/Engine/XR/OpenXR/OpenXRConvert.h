// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Math/Math.h"
#include "Engine/Core/Math/Matrix.h"
#include "Engine/Core/Math/Transform.h"
#include "Engine/Engine/Units.h"
#include <ThirdParty/OpenXR/openxr/openxr.h>

/// <summary>
/// The conversion utilities between the OpenXR and the engine coordinate systems. OpenXR is right-handed with -Z forward in meters, the engine is left-handed with +Z forward in units.
/// </summary>
namespace OpenXRConvert
{
    /// <summary>
    /// Converts the OpenXR position into the engine position.
    /// </summary>
    /// <param name="position">The OpenXR position (in meters).</param>
    /// <returns>The engine position (in units).</returns>
    FORCE_INLINE Float3 ToPosition(const XrVector3f& position)
    {
        return Float3(position.x, position.y, -position.z) * METERS_TO_UNITS_SCALE;
    }

    /// <summary>
    /// Converts the OpenXR orientation into the engine orientation.
    /// </summary>
    /// <param name="orientation">The OpenXR orientation.</param>
    /// <returns>The engine orientation.</returns>
    FORCE_INLINE Quaternion ToOrientation(const XrQuaternionf& orientation)
    {
        // Mirror the Z axis
        return Quaternion(-orientation.x, -orientation.y, orientation.z, orientation.w);
    }

    /// <summary>
    /// Converts the OpenXR pose into the engine transformation.
    /// </summary>
    /// <param name="pose">The OpenXR pose.</param>
    /// <returns>The engine transformation.</returns>
    FORCE_INLINE Transform ToTransform(const XrPosef& pose)
    {
        return Transform(ToPosition(pose.position), ToOrientation(pose.orientation));
    }

    /// <summary>
    /// Converts the OpenXR field of view into the engine projection matrix.
    /// </summary>
    /// <param name="fov">The OpenXR field of view (angles can be asymmetric).</param>
    /// <param name="nearPlane">The near plane distance.</param>
    /// <param name="farPlane">The far plane distance.</param>
    /// <param name="result">The result projection matrix.</param>
    FORCE_INLINE void ToProjection(const XrFovf& fov, float nearPlane, float farPlane, Matrix& result)
    {
        Matrix::PerspectiveOffCenter(nearPlane * Math::Tan(fov.angleLeft), nearPlane * Math::Tan(fov.angleRight), nearPlane * Math::Tan(fov.angleDown), nearPlane * Math::Tan(fov.angleUp), nearPlane, farPlane, result);
    }
}
