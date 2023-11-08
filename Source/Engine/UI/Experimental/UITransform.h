#pragma once
#include <Engine/Core/Math/Rectangle.h>
#include <Engine/Core/Math/Matrix3x3.h>
#include <Engine/Core/Types/BaseTypes.h>
#include <Engine/Core/Collections/Array.h>
/// <summary>
/// Transform
/// </summary>
API_CLASS()
class FLAXENGINE_API UITransform
{
    /// <summary>
    /// the Anchor
    /// </summary>
    API_STRUCT()
    struct FLAXENGINE_API Anchor
    {
    public:
        /// <summary>
        /// UI control anchors presets.
        /// </summary>
        API_ENUM()
        enum FLAXENGINE_API Presets
        {
            /// <summary>
            /// The empty preset.
            /// </summary>
            Custom,

            /// <summary>
            /// The top left corner of the parent control.
            /// </summary>
            TopLeft,

            /// <summary>
            /// The center of the top edge of the parent control.
            /// </summary>
            TopCenter,

            /// <summary>
            /// The top right corner of the parent control.
            /// </summary>
            TopRight,

            /// <summary>
            /// The middle of the left edge of the parent control.
            /// </summary>
            MiddleLeft,

            /// <summary>
            /// The middle center! Right in the middle of the parent control.
            /// </summary>
            MiddleCenter,

            /// <summary>
            /// The middle of the right edge of the parent control.
            /// </summary>
            MiddleRight,

            /// <summary>
            /// The bottom left corner of the parent control.
            /// </summary>
            BottomLeft,

            /// <summary>
            /// The center of the bottom edge of the parent control.
            /// </summary>
            BottomCenter,

            /// <summary>
            /// The bottom right corner of the parent control.
            /// </summary>
            BottomRight,

            /// <summary>
            /// The vertical stretch on the left of the parent control.
            /// </summary>
            VerticalStretchLeft,

            /// <summary>
            /// The vertical stretch on the center of the parent control.
            /// </summary>
            VerticalStretchCenter,

            /// <summary>
            /// The vertical stretch on the right of the parent control.
            /// </summary>
            VerticalStretchRight,

            /// <summary>
            /// The horizontal stretch on the top of the parent control.
            /// </summary>
            HorizontalStretchTop,

            /// <summary>
            /// The horizontal stretch in the middle of the parent control.
            /// </summary>
            HorizontalStretchMiddle,

            /// <summary>
            /// The horizontal stretch on the bottom of the parent control.
            /// </summary>
            HorizontalStretchBottom,

            /// <summary>
            /// All parent control edges.
            /// </summary>
            StretchAll,
        };

        /// <summary>
        /// min value
        /// </summary>
        API_PROPERTY()
        Float2 Min;

        /// <summary>
        /// max value
        /// </summary>
        API_PROPERTY()
        Float2 Max;

        /// <summary>
        /// sets the preset for anchor
        /// </summary>
        API_FUNCTION()
        void SetPreset(Presets presets)
        {
            switch (presets)
            {
            case Presets::TopLeft:                  Min = Float2(0, 0);         Max = Float2(0, 0); break;
            case Presets::TopCenter:                Min = Float2(0.5f, 0);      Max = Float2(0.5f, 0); break;
            case Presets::TopRight:                 Min = Float2(1, 0);         Max = Float2(1, 0); break;
            case Presets::MiddleLeft:               Min = Float2(0, 0.5f);      Max = Float2(0, 0.5f); break;
            case Presets::MiddleCenter:             Min = Float2(0.5f, 0.5f);   Max = Float2(0.5f, 0.5f); break;
            case Presets::MiddleRight:              Min = Float2(1, 0.5f);      Max = Float2(1, 0.5f); break;
            case Presets::BottomLeft:               Min = Float2(0, 1);         Max = Float2(0, 1); break;
            case Presets::BottomCenter:             Min = Float2(0.5f, 1);      Max = Float2(0.5f, 1); break;
            case Presets::BottomRight:              Min = Float2(1, 1); Max = Float2(1, 1); break;
            case Presets::HorizontalStretchTop:     Min = Float2(0, 0); Max = Float2(1, 0); break;
            case Presets::HorizontalStretchMiddle:  Min = Float2(0, 0.5f); Max = Float2(1, 0.5f); break;
            case Presets::HorizontalStretchBottom:  Min = Float2(0, 1); Max = Float2(1, 1); break;
            case Presets::VerticalStretchLeft:      Min = Float2(0, 0); Max = Float2(0, 1); break;
            case Presets::VerticalStretchCenter:    Min = Float2(0.5f, 0); Max = Float2(0.5f, 1); break;
            case Presets::VerticalStretchRight:     Min = Float2(1, 0); Max = Float2(1, 1); break;
            case Presets::StretchAll:               Min = Float2(0, 0); Max = Float2(1, 1); break;
            }
        }
    };
private:
    Rectangle _transformation;
    Float2 _scale =  Float2(1.0f);
    Float2 _pivot =  Float2(0.5f);
    Float2 _shear;
    float _rotation;
    Anchor _anchor;
    Matrix3x3 _cachedTransform;
    Matrix3x3 _cachedTransformInv;
    UITransform* Parent;
    Array<UITransform*> Children = Array<UITransform*>();
public:
    /// <summary>
    /// crates  UITransform
    /// </summary>
    /// <param name="transformation"></param>
    /// <param name="scale"></param>
    /// <param name="shear"></param>
    /// <param name="rotation"></param>
    UITransform(Rectangle transformation, Float2 scale, Float2 shear, float rotation)
    {
        _transformation = transformation;
        _scale = scale;
        _shear = shear;
        _rotation = rotation;
        UpdateTransform();
    }

    /// <summary>
    /// Get's the transformation location
    /// </summary>
    /// <returns>location</returns>
    API_FUNCTION()
    Vector2 GetLocation()
    {
        return _transformation.Location;
    }

    /// <summary>
    /// Set's the transformation location
    /// </summary>
    /// <param name="vector"> location</param>
    API_FUNCTION()
    void SetLocation(Vector2 vector)
    {
        _transformation.Location = vector;
        UpdateTransform();
    }

    /// <summary>
    /// Get's the transformation size
    /// </summary>
    /// <returns>location</returns>
    API_FUNCTION()
    Vector2 GetSize()
    {
        return _transformation.Size;
    }

    /// <summary>
    /// Set's the transformation size
    /// </summary>
    /// <param name="vector"> size</param>
    API_FUNCTION()
    void SetSize(Vector2 vector)
    {
        _transformation.Size = vector;
        UpdateTransform();
    }

    /// <summary>
    /// Get's the pivot
    /// </summary>
    /// <returns>pivot</returns>
    API_FUNCTION()
    Vector2 GetPivot()
    {
        return _pivot;
    }

    /// <summary>
    /// Set's the pivot
    /// </summary>
    /// <param name="vector"> pivot</param>
    API_FUNCTION()
    void SetPivot(Vector2 vector)
    {
        _pivot = vector;
        UpdateTransform();
    }

    /// <summary>
    /// Get's the shear
    /// </summary>
    /// <returns>shear</returns>
    API_FUNCTION()
    Vector2 GetShear()
    {
        return _shear;
    }
    /// <summary>
    /// Set's the shear
    /// </summary>
    /// <param name="vector"> shear</param>
    API_FUNCTION()
    void SetShear(Vector2 vector)
    {
        _shear = vector;
        UpdateTransform();
    }

    /// <summary>
    /// Get's the rotation
    /// </summary>
    /// <returns>rotation</returns>
    API_FUNCTION()
    float GetRotation()
    {
        return _rotation;
    }

    /// <summary>
    /// Set's the rotation
    /// </summary>
    /// <param name="vector"> rotation</param>
    API_FUNCTION()
    void SetRotation(float vector)
    {
        _rotation = vector;
        UpdateTransform();
    }

    /// <summary>
    /// Get's the anchor
    /// </summary>
    /// <param name="vector"> anchor</param>
    API_FUNCTION()
    Anchor& GetAnchor()
    {
        return _anchor;
    }

    /// <summary>
    /// Set's the anchor
    /// </summary>
    /// <param name="anchor"> anchor</param>
    API_FUNCTION()
    void SetRotation(Anchor& anchor)
    {
        _anchor = anchor;
        UpdateTransform();
    }

    /// <summary>
    /// Updates the control cached transformation matrix (based on bounds).
    /// </summary>
    void UpdateTransform()
    {
        // Actual pivot and negative pivot
        Float2.Multiply(ref _pivot, ref _bounds.Size, out var v1);  // v1 = _pivot * _bounds
        Float2.Negate(ref v1, out var v2);                          // v2 = -v1
        Float2.Add(ref v1, ref _bounds.Location, out v1);           // v1 = v1 + _bounds

        // 2D transformation
        Matrix2x2.Scale(ref _scale, out Matrix2x2 m1);
        Matrix2x2.Shear(ref _shear, out Matrix2x2 m2);
        Matrix2x2.Multiply(ref m1, ref m2, out m1);
        Matrix2x2.Rotation(Mathf.DegreesToRadians * _rotation, out m2);
        Matrix2x2.Multiply(ref m1, ref m2, out m1);

        // Mix all the stuff
        Matrix3x3.Translation2D(ref v2, out Matrix3x3 m3);
        Matrix3x3 m4 = (Matrix3x3)m1;
        Matrix3x3.Multiply(ref m3, ref m4, out m3);
        Matrix3x3.Translation2D(ref v1, out m4);
        Matrix3x3.Multiply(ref m3, ref m4, out _cachedTransform);

        // Cache inverted transform
        Matrix3x3.Invert(ref _cachedTransform, out _cachedTransformInv);
    }
}
