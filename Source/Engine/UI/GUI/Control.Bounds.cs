// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine.GUI
{
    public partial class Control
    {
        /// <summary>
        /// Gets or sets X coordinate of the upper-left corner of the control relative to the upper-left corner of its container.
        /// </summary>
        [HideInEditor, NoSerialize]
        public float X
        {
            get => _bounds.X;
            set => Bounds = new Rectangle(value, Y, _bounds.Size);
        }

        /// <summary>
        /// Gets or sets Y coordinate of the upper-left corner of the control relative to the upper-left corner of its container.
        /// </summary>
        [HideInEditor, NoSerialize]
        public float Y
        {
            get => _bounds.Y;
            set => Bounds = new Rectangle(X, value, _bounds.Size);
        }

        /// <summary>
        /// Gets or sets the normalized position in the parent control that the upper left corner is anchored to (range 0-1).
        /// </summary>
        [Serialize]
        [ExpandGroups, Limit(0.0f, 1.0f, 0.01f), EditorDisplay("Transform"), EditorOrder(990), Tooltip("The normalized position in the parent control that the upper left corner is anchored to (range 0-1).")]
        public Vector2 AnchorMin
        {
            get => _anchorMin;
            set
            {
                if (!_anchorMin.Equals(ref value))
                {
                    var bounds = Bounds;
                    _anchorMin = value;
                    UpdateBounds();
                    Bounds = bounds;
                }
            }
        }

        /// <summary>
        /// Gets or sets the normalized position in the parent control that the bottom right corner is anchored to (range 0-1).
        /// </summary>
        [Serialize]
        [ExpandGroups, Limit(0.0f, 1.0f, 0.01f), EditorDisplay("Transform"), EditorOrder(991), Tooltip("The normalized position in the parent control that the bottom right corner is anchored to (range 0-1).")]
        public Vector2 AnchorMax
        {
            get => _anchorMax;
            set
            {
                if (!_anchorMax.Equals(ref value))
                {
                    var bounds = Bounds;
                    _anchorMax = value;
                    UpdateBounds();
                    Bounds = bounds;
                }
            }
        }

        /// <summary>
        /// Gets or sets the offsets of the corners of the control relative to its anchors.
        /// </summary>
        [Serialize]
        [ExpandGroups, EditorDisplay("Transform"), EditorOrder(992), Tooltip("The offsets of the corners of the control relative to its anchors.")]
        public Margin Offsets
        {
            get => _offsets;
            set
            {
                if (!_offsets.Equals(ref value))
                {
                    _offsets = value;
                    UpdateBounds();
                }
            }
        }

        /// <summary>
        /// Gets or sets coordinates of the upper-left corner of the control relative to the upper-left corner of its container.
        /// </summary>
        [NoSerialize]
        [ExpandGroups, EditorDisplay("Transform"), EditorOrder(1000), Tooltip("The location of the upper-left corner of the control relative to he upper-left corner of its container.")]
        public Vector2 Location
        {
            get => _bounds.Location;
            set => Bounds = new Rectangle(value, _bounds.Size);
        }

        /// <summary>
        /// Gets or sets width of the control.
        /// </summary>
        [NoSerialize, HideInEditor]
        public float Width
        {
            get => _bounds.Width;
            set => Bounds = new Rectangle(_bounds.Location, value, _bounds.Height);
        }

        /// <summary>
        /// Gets or sets height of the control.
        /// </summary>
        [NoSerialize, HideInEditor]
        public float Height
        {
            get => _bounds.Height;
            set => Bounds = new Rectangle(_bounds.Location, _bounds.Width, value);
        }

        /// <summary>
        /// Gets or sets control's size.
        /// </summary>
        [NoSerialize]
        [EditorDisplay("Transform"), EditorOrder(1010), Tooltip("The size of the control bounds.")]
        public Vector2 Size
        {
            get => _bounds.Size;
            set => Bounds = new Rectangle(_bounds.Location, value);
        }

        /// <summary>
        /// Gets Y coordinate of the top edge of the control relative to the upper-left corner of its container.
        /// </summary>
        public float Top => _bounds.Top;

        /// <summary>
        /// Gets Y coordinate of the bottom edge of the control relative to the upper-left corner of its container.
        /// </summary>
        public float Bottom => _bounds.Bottom;

        /// <summary>
        /// Gets X coordinate of the left edge of the control relative to the upper-left corner of its container.
        /// </summary>
        public float Left => _bounds.Left;

        /// <summary>
        /// Gets X coordinate of the right edge of the control relative to the upper-left corner of its container.
        /// </summary>
        public float Right => _bounds.Right;

        /// <summary>
        /// Gets position of the upper left corner of the control relative to the upper-left corner of its container.
        /// </summary>
        public Vector2 UpperLeft => _bounds.UpperLeft;

        /// <summary>
        /// Gets position of the upper right corner of the control relative to the upper-left corner of its container.
        /// </summary>
        public Vector2 UpperRight => _bounds.UpperRight;

        /// <summary>
        /// Gets position of the bottom right corner of the control relative to the upper-left corner of its container.
        /// </summary>
        public Vector2 BottomRight => _bounds.BottomRight;

        /// <summary>
        /// Gets position of the bottom left of the control relative to the upper-left corner of its container.
        /// </summary>
        public Vector2 BottomLeft => _bounds.BottomLeft;

        /// <summary>
        /// Gets center position of the control relative to the upper-left corner of its container.
        /// </summary>
        public Vector2 Center => _bounds.Center;

        /// <summary>
        /// Gets or sets control's bounds rectangle.
        /// </summary>
        [HideInEditor, NoSerialize]
        public Rectangle Bounds
        {
            get => _bounds;
            set
            {
                if (!_bounds.Equals(ref value))
                {
                    // Calculate anchors based on the parent container client area
                    Margin anchors;
                    if (_parent != null)
                    {
                        _parent.GetDesireClientArea(out var parentBounds);
                        anchors = new Margin
                        (
                         _anchorMin.X * parentBounds.Size.X + parentBounds.Location.X,
                         _anchorMax.X * parentBounds.Size.X,
                         _anchorMin.Y * parentBounds.Size.Y + parentBounds.Location.Y,
                         _anchorMax.Y * parentBounds.Size.Y
                        );
                    }
                    else
                    {
                        anchors = Margin.Zero;
                    }

                    // Calculate offsets on X axis
                    _offsets.Left = value.Location.X - anchors.Left;
                    if (_anchorMin.X != _anchorMax.X)
                    {
                        _offsets.Right = anchors.Right - value.Location.X - value.Size.X;
                    }
                    else
                    {
                        _offsets.Right = value.Size.X;
                    }

                    // Calculate offsets on Y axis
                    _offsets.Top = value.Location.Y - anchors.Top;
                    if (_anchorMin.Y != _anchorMax.Y)
                    {
                        _offsets.Bottom = anchors.Bottom - value.Location.Y - value.Size.Y;
                    }
                    else
                    {
                        _offsets.Bottom = value.Size.Y;
                    }

                    // Flush the control bounds
                    UpdateBounds();
                }
            }
        }

        /// <summary>
        /// Gets or sets the scale.
        /// </summary>
        [EditorDisplay("Transform"), Limit(float.MinValue, float.MaxValue, 0.1f), EditorOrder(1020), Tooltip("The control scale parameter.")]
        public Vector2 Scale
        {
            get => _scale;
            set
            {
                if (!_scale.Equals(ref value))
                {
                    SetScaleInternal(ref value);
                }
            }
        }

        /// <summary>
        /// Gets or sets the normalized pivot location (used to transform control around it). Point (0,0) is upper left corner, (0.5,0.5) is center, (1,1) is bottom right corner.
        /// </summary>
        [EditorDisplay("Transform"), Limit(0.0f, 1.0f, 0.1f), EditorOrder(1030), Tooltip("The control rotation pivot location in normalized control size. Point (0,0) is upper left corner, (0.5,0.5) is center, (1,1) is bottom right corner.")]
        public Vector2 Pivot
        {
            get => _pivot;
            set
            {
                if (!_pivot.Equals(ref value))
                {
                    SetPivotInternal(ref value);
                }
            }
        }

        /// <summary>
        /// Gets or sets the shear transform angles (x, y). Defined in degrees.
        /// </summary>
        [EditorDisplay("Transform"), EditorOrder(1040), Tooltip("The shear transform angles (x, y). Defined in degrees.")]
        public Vector2 Shear
        {
            get => _shear;
            set
            {
                if (!_shear.Equals(ref value))
                {
                    SetShearInternal(ref value);
                }
            }
        }

        /// <summary>
        /// Gets or sets the rotation angle (in degrees).
        /// </summary>
        [EditorDisplay("Transform"), EditorOrder(1050), Tooltip("The control rotation angle (in degrees).")]
        public float Rotation
        {
            get => _rotation;
            set
            {
                if (!Mathf.NearEqual(_rotation, value))
                {
                    SetRotationInternal(value);
                }
            }
        }

        /// <summary>
        /// Updates the control cached bounds (based on anchors and offsets).
        /// </summary>
        [NoAnimate]
        public void UpdateBounds()
        {
            var prevBounds = _bounds;

            // Calculate anchors based on the parent container client area
            Margin anchors;
            Vector2 offset;
            if (_parent != null)
            {
                _parent.GetDesireClientArea(out var parentBounds);
                anchors = new Margin
                (
                 _anchorMin.X * parentBounds.Size.X,
                 _anchorMax.X * parentBounds.Size.X,
                 _anchorMin.Y * parentBounds.Size.Y,
                 _anchorMax.Y * parentBounds.Size.Y
                );
                offset = parentBounds.Location;
            }
            else
            {
                anchors = Margin.Zero;
                offset = Vector2.Zero;
            }

            // Calculate position and size on X axis
            _bounds.Location.X = anchors.Left + _offsets.Left;
            if (_anchorMin.X != _anchorMax.X)
            {
                _bounds.Size.X = anchors.Right - _bounds.Location.X - _offsets.Right;
            }
            else
            {
                _bounds.Size.X = _offsets.Right;
            }

            // Calculate position and size on Y axis
            _bounds.Location.Y = anchors.Top + _offsets.Top;
            if (_anchorMin.Y != _anchorMax.Y)
            {
                _bounds.Size.Y = anchors.Bottom - _bounds.Location.Y - _offsets.Bottom;
            }
            else
            {
                _bounds.Size.Y = _offsets.Bottom;
            }

            // Apply the offset
            _bounds.Location += offset;

            // Update cached transformation matrix
            UpdateTransform();

            // Handle location/size changes
            if (_bounds.Location != prevBounds.Location)
            {
                OnLocationChanged();
            }

            if (_bounds.Size != prevBounds.Size)
            {
                OnSizeChanged();
            }
        }

        /// <summary>
        /// Updates the control cached transformation matrix (based on bounds).
        /// </summary>
        [NoAnimate]
        public void UpdateTransform()
        {
            // Actual pivot and negative pivot
            Vector2.Multiply(ref _pivot, ref _bounds.Size, out Vector2 v1);
            Vector2.Negate(ref v1, out Vector2 v2);
            Vector2.Add(ref v1, ref _bounds.Location, out v1);

            // ------ Matrix3x3 based version:

            /*
            // Negative pivot
            Matrix3x3 m1, m2;
            Matrix3x3.Translation2D(ref v2, out m1);

            // Scale
            Matrix3x3.Scaling(_scale.X, _scale.Y, 1, out m2);
            Matrix3x3.Multiply(ref m1, ref m2, out m1);

            // Shear
            Matrix3x3.Shear(ref _shear, out m2);
            Matrix3x3.Multiply(ref m1, ref m2, out m1);

            // Rotation
            Matrix3x3.RotationZ(_rotation * Mathf.DegreesToRadians, out m2);
            Matrix3x3.Multiply(ref m1, ref m2, out m1);

            // Pivot + Location
            Matrix3x3.Translation2D(ref v1, out m2);
            Matrix3x3.Multiply(ref m1, ref m2, out _cachedTransform);
            */

            // ------ Matrix2x2 based version:

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

        /// <summary>
        /// Sets the anchor preset for the control. Can be use to auto-place the control for a given preset or can preserve the current control bounds.
        /// </summary>
        /// <param name="anchorPreset">The anchor preset to set.</param>
        /// <param name="preserveBounds">True if preserve current control bounds, otherwise will align control position accordingly to the anchor location.</param>
        public void SetAnchorPreset(AnchorPresets anchorPreset, bool preserveBounds)
        {
            for (int i = 0; i < AnchorPresetsData.Length; i++)
            {
                if (AnchorPresetsData[i].Preset == anchorPreset)
                {
                    var anchorMin = AnchorPresetsData[i].Min;
                    var anchorMax = AnchorPresetsData[i].Max;
                    var bounds = _bounds;
                    if (!Vector2.NearEqual(ref _anchorMin, ref anchorMin) ||
                        !Vector2.NearEqual(ref _anchorMax, ref anchorMax))
                    {
                        // Disable scrolling for anchored controls (by default but can be manually restored)
                        if (!anchorMin.IsZero || !anchorMax.IsZero)
                            IsScrollable = false;

                        _anchorMin = anchorMin;
                        _anchorMax = anchorMax;
                        if (preserveBounds)
                        {
                            UpdateBounds();
                            Bounds = bounds;
                        }
                    }
                    if (!preserveBounds)
                    {
                        if (_parent != null)
                        {
                            _parent.GetDesireClientArea(out var parentBounds);
                            switch (anchorPreset)
                            {
                            case AnchorPresets.TopLeft:
                                bounds.Location = Vector2.Zero;
                                break;
                            case AnchorPresets.TopCenter:
                                bounds.Location = new Vector2(parentBounds.Width * 0.5f - bounds.Width * 0.5f, 0);
                                break;
                            case AnchorPresets.TopRight:
                                bounds.Location = new Vector2(parentBounds.Width - bounds.Width, 0);
                                break;
                            case AnchorPresets.MiddleLeft:
                                bounds.Location = new Vector2(0, parentBounds.Height * 0.5f - bounds.Height * 0.5f);
                                break;
                            case AnchorPresets.MiddleCenter:
                                bounds.Location = new Vector2(parentBounds.Width * 0.5f - bounds.Width * 0.5f, parentBounds.Height * 0.5f - bounds.Height * 0.5f);
                                break;
                            case AnchorPresets.MiddleRight:
                                bounds.Location = new Vector2(parentBounds.Width - bounds.Width, parentBounds.Height * 0.5f - bounds.Height * 0.5f);
                                break;
                            case AnchorPresets.BottomLeft:
                                bounds.Location = new Vector2(0, parentBounds.Height - bounds.Height);
                                break;
                            case AnchorPresets.BottomCenter:
                                bounds.Location = new Vector2(parentBounds.Width * 0.5f - bounds.Width * 0.5f, parentBounds.Height - bounds.Height);
                                break;
                            case AnchorPresets.BottomRight:
                                bounds.Location = new Vector2(parentBounds.Width - bounds.Width, parentBounds.Height - bounds.Height);
                                break;
                            case AnchorPresets.VerticalStretchLeft:
                                bounds.Location = Vector2.Zero;
                                bounds.Size = new Vector2(bounds.Width, parentBounds.Height);
                                break;
                            case AnchorPresets.VerticalStretchCenter:
                                bounds.Location = new Vector2(parentBounds.Width * 0.5f - bounds.Width * 0.5f, 0);
                                bounds.Size = new Vector2(bounds.Width, parentBounds.Height);
                                break;
                            case AnchorPresets.VerticalStretchRight:
                                bounds.Location = new Vector2(parentBounds.Width - bounds.Width, 0);
                                bounds.Size = new Vector2(bounds.Width, parentBounds.Height);
                                break;
                            case AnchorPresets.HorizontalStretchTop:
                                bounds.Location = Vector2.Zero;
                                bounds.Size = new Vector2(parentBounds.Width, bounds.Height);
                                break;
                            case AnchorPresets.HorizontalStretchMiddle:
                                bounds.Location = new Vector2(0, parentBounds.Height * 0.5f - bounds.Height * 0.5f);
                                bounds.Size = new Vector2(parentBounds.Width, bounds.Height);
                                break;
                            case AnchorPresets.HorizontalStretchBottom:
                                bounds.Location = new Vector2(0, parentBounds.Height - bounds.Height);
                                bounds.Size = new Vector2(parentBounds.Width, bounds.Height);
                                break;
                            case AnchorPresets.StretchAll:
                                bounds.Location = Vector2.Zero;
                                bounds.Size = parentBounds.Size;
                                break;
                            default: throw new ArgumentOutOfRangeException(nameof(anchorPreset), anchorPreset, null);
                            }
                            bounds.Location += parentBounds.Location;
                        }
                        Bounds = bounds;
                    }
                    return;
                }
            }
        }
    }
}
