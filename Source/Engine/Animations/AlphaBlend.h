// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Math/Math.h"

/// <summary>
/// Alpha blending modes.
/// </summary>
API_ENUM() enum class AlphaBlendMode : byte
{
    /// <summary>
    /// Linear interpolation.
    /// </summary>
    Linear = 0,

    /// <summary>
    /// Cubic-in interpolation.
    /// </summary>
    Cubic,

    /// <summary>
    /// Hermite-Cubic.
    /// </summary>
    HermiteCubic,

    /// <summary>
    /// Sinusoidal interpolation.
    /// </summary>
    Sinusoidal,

    /// <summary>
    /// Quadratic in-out interpolation.
    /// </summary>
    QuadraticInOut,

    /// <summary>
    /// Cubic in-out interpolation.
    /// </summary>
    CubicInOut,

    /// <summary>
    /// Quartic in-out interpolation.
    /// </summary>
    QuarticInOut,

    /// <summary>
    /// Quintic in-out interpolation.
    /// </summary>
    QuinticInOut,

    /// <summary>
    /// Circular-in interpolation.
    /// </summary>
    CircularIn,

    /// <summary>
    /// Circular-out interpolation.
    /// </summary>
    CircularOut,

    /// <summary>
    /// Circular in-out interpolation.
    /// </summary>
    CircularInOut,

    /// <summary>
    /// Exponential-in interpolation.
    /// </summary>
    ExpIn,

    /// <summary>
    /// Exponential-Out interpolation.
    /// </summary>
    ExpOut,

    /// <summary>
    /// Exponential in-out interpolation.
    /// </summary>
    ExpInOut,
};

/// <summary>
/// Alpha blending utilities.
/// </summary>
class AlphaBlend
{
public:
    /// <summary>
    /// Converts the input alpha value from a linear 0-1 value into the output alpha described by blend mode.
    /// </summary>
    /// <param name="alpha">The alpha (normalized to 0-1).</param>
    /// <param name="mode">The mode.</param>
    /// <returns>The output alpha (normalized to 0-1).</returns>
    static float Process(float alpha, AlphaBlendMode mode)
    {
        switch (mode)
        {
        case AlphaBlendMode::Sinusoidal:
            alpha = (Math::Sin(alpha * PI - PI_HALF) + 1.0f) / 2.0f;
            break;
        case AlphaBlendMode::Cubic:
            alpha = Math::CubicInterp<float>(0.0f, 0.0f, 1.0f, 0.0f, alpha);
            break;
        case AlphaBlendMode::QuadraticInOut:
            alpha = Math::InterpEaseInOut<float>(0.0f, 1.0f, alpha, 2);
            break;
        case AlphaBlendMode::CubicInOut:
            alpha = Math::InterpEaseInOut<float>(0.0f, 1.0f, alpha, 3);
            break;
        case AlphaBlendMode::HermiteCubic:
            alpha = Math::SmoothStep(0.0f, 1.0f, alpha);
            break;
        case AlphaBlendMode::QuarticInOut:
            alpha = Math::InterpEaseInOut<float>(0.0f, 1.0f, alpha, 4);
            break;
        case AlphaBlendMode::QuinticInOut:
            alpha = Math::InterpEaseInOut<float>(0.0f, 1.0f, alpha, 5);
            break;
        case AlphaBlendMode::CircularIn:
            alpha = Math::InterpCircularIn<float>(0.0f, 1.0f, alpha);
            break;
        case AlphaBlendMode::CircularOut:
            alpha = Math::InterpCircularOut<float>(0.0f, 1.0f, alpha);
            break;
        case AlphaBlendMode::CircularInOut:
            alpha = Math::InterpCircularInOut<float>(0.0f, 1.0f, alpha);
            break;
        case AlphaBlendMode::ExpIn:
            alpha = Math::InterpExpoIn<float>(0.0f, 1.0f, alpha);
            break;
        case AlphaBlendMode::ExpOut:
            alpha = Math::InterpExpoOut<float>(0.0f, 1.0f, alpha);
            break;
        case AlphaBlendMode::ExpInOut:
            alpha = Math::InterpExpoInOut<float>(0.0f, 1.0f, alpha);
            break;
        default: ;
        }

        return Math::Saturate<float>(alpha);
    }
};
