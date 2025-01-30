// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.ComponentModel;

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
        /// Gets or sets the local X coordinate of the pivot of the control relative to the anchor in parent of its container.
        /// </summary>
        [HideInEditor, NoSerialize]
        public float LocalX
        {
            get => LocalLocation.X;
            set => LocalLocation = new Float2(value, LocalLocation.Y);
        }

        /// <summary>
        /// Gets or sets the local Y coordinate of the pivot of the control relative to the anchor in parent of its container.
        /// </summary>
        [HideInEditor, NoSerialize]
        public float LocalY
        {
            get => LocalLocation.Y;
            set => LocalLocation = new Float2(LocalLocation.X, value);
        }

        /// <summary>
        /// Gets or sets the normalized position in the parent control that the upper left corner is anchored to (range 0-1).
        /// </summary>
        [Serialize, HideInEditor, Limit(0, 1, 0.01f)]
        public Float2 AnchorMin
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
        [Serialize, HideInEditor, Limit(0, 1, 0.01f)]
        public Float2 AnchorMax
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
        [Serialize, HideInEditor]
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

#if FLAX_EDITOR
        /// <summary>
        /// Helper for Editor UI (see UIControlControlEditor).
        /// </summary>
        [NoSerialize, HideInEditor, NoAnimate]
        internal float Proxy_Offset_Left
        {
            get => _offsets.Left;
            set => Offsets = new Margin(value, _offsets.Right, _offsets.Top, _offsets.Bottom);
        }

        /// <summary>
        /// Helper for Editor UI (see UIControlControlEditor).
        /// </summary>
        [NoSerialize, HideInEditor, NoAnimate]
        internal float Proxy_Offset_Right
        {
            get => _offsets.Right;
            set => Offsets = new Margin(_offsets.Left, value, _offsets.Top, _offsets.Bottom);
        }

        /// <summary>
        /// Helper for Editor UI (see UIControlControlEditor).
        /// </summary>
        [NoSerialize, HideInEditor, NoAnimate]
        internal float Proxy_Offset_Top
        {
            get => _offsets.Top;
            set => Offsets = new Margin(_offsets.Left, _offsets.Right, value, _offsets.Bottom);
        }

        /// <summary>
        /// Helper for Editor UI (see UIControlControlEditor).
        /// </summary>
        [NoSerialize, HideInEditor, NoAnimate]
        internal float Proxy_Offset_Bottom
        {
            get => _offsets.Bottom;
            set => Offsets = new Margin(_offsets.Left, _offsets.Right, _offsets.Top, value);
        }
#endif

        /// <summary>
        /// Gets or sets coordinates of the upper-left corner of the control relative to the upper-left corner of its container.
        /// </summary>
        [NoSerialize, HideInEditor]
        public Float2 Location
        {
            get => _bounds.Location;
            set
            {
                if (_bounds.Location.Equals(ref value))
                    return;
                var bounds = new Rectangle(value, _bounds.Size);
                SetBounds(ref bounds);
            }
        }

        /// <summary>
        /// Gets or sets the local position of the pivot of the control relative to the anchor in parent of its container.
        /// </summary>
        [NoSerialize, HideInEditor]
        public Float2 LocalLocation
        {
            get => _bounds.Location - (_parent != null ? _parent._bounds.Size * (_anchorMax + _anchorMin) * 0.5f : Float2.Zero) + _bounds.Size * _pivot;
            set => Bounds = new Rectangle(value + (_parent != null ? _parent.Bounds.Size * (_anchorMax + _anchorMin) * 0.5f : Float2.Zero) - _bounds.Size * _pivot, _bounds.Size);
        }

        /// <summary>
        /// Whether to resize the UI Control based on where the pivot is rather than just the top-left.
        /// </summary>
        [NoSerialize, HideInEditor]
        public bool PivotRelative
        {
            get => _pivotRelativeSizing;
            set => _pivotRelativeSizing = value;
        }

        /// <summary>
        /// Gets or sets width of the control.
        /// </summary>
        [NoSerialize, HideInEditor]
        public float Width
        {
            get => _bounds.Size.X;
            set
            {
                if (Mathf.NearEqual(_bounds.Size.X, value))
                    return;
                var bounds = new Rectangle(_bounds.Location, value, _bounds.Size.Y);
                if (_pivotRelativeSizing)
                {
                    var delta = _bounds.Size.X - value;
                    bounds.Location.X += delta * Pivot.X;
                }
                SetBounds(ref bounds);
            }
        }

        /// <summary>
        /// Gets or sets height of the control.
        /// </summary>
        [NoSerialize, HideInEditor]
        public float Height
        {
            get => _bounds.Size.Y;
            set
            {
                if (Mathf.NearEqual(_bounds.Size.Y, value))
                    return;
                var bounds = new Rectangle(_bounds.Location, _bounds.Size.X, value);
                if (_pivotRelativeSizing)
                {
                    var delta = _bounds.Size.Y - value;
                    bounds.Location.Y += delta * Pivot.Y;
                }
                SetBounds(ref bounds);
            }
        }

        /// <summary>
        /// Gets or sets control's size.
        /// </summary>
        [NoSerialize, HideInEditor]
        public Float2 Size
        {
            get => _bounds.Size;
            set
            {
                if (_bounds.Size.Equals(ref value))
                    return;
                var bounds = new Rectangle(_bounds.Location, value);
                if (_pivotRelativeSizing)
                {
                    var delta = _bounds.Size - value;
                    bounds.Location += delta * Pivot;
                }
                SetBounds(ref bounds);
            }
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
        public Float2 UpperLeft => _bounds.UpperLeft;

        /// <summary>
        /// Gets position of the upper right corner of the control relative to the upper-left corner of its container.
        /// </summary>
        public Float2 UpperRight => _bounds.UpperRight;

        /// <summary>
        /// Gets position of the bottom right corner of the control relative to the upper-left corner of its container.
        /// </summary>
        public Float2 BottomRight => _bounds.BottomRight;

        /// <summary>
        /// Gets position of the bottom left of the control relative to the upper-left corner of its container.
        /// </summary>
        public Float2 BottomLeft => _bounds.BottomLeft;

        /// <summary>
        /// Gets center position of the control relative to the upper-left corner of its container.
        /// </summary>
        [HideInEditor, NoSerialize]
        public Float2 Center
        {
            get => _bounds.Center;
            set => Location = value - Size * 0.5f;
        }

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
                    SetBounds(ref value);
            }
        }

        private void SetBounds(ref Rectangle value)
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

        /// <summary>
        /// Gets or sets the scale. Scales control according to its Pivot which by default is (0.5,0.5) (middle of the control). If you set pivot to (0,0) it will scale the control based on it's upper-left corner.
        /// </summary>
        [ExpandGroups, EditorDisplay("Transform"), Limit(float.MinValue, float.MaxValue, 0.1f), EditorOrder(1020), Tooltip("The control scale parameter. Scales control according to its Pivot which by default is (0.5,0.5) (middle of the control). If you set pivot to (0,0) it will scale the control based on it's upper-left corner.")]
        public Float2 Scale
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
        [ExpandGroups, EditorDisplay("Transform"), Limit(0.0f, 1.0f, 0.1f), EditorOrder(1030), Tooltip("The control rotation pivot location in normalized control size. Point (0,0) is upper left corner, (0.5,0.5) is center, (1,1) is bottom right corner.")]
        public Float2 Pivot
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
        /// Gets or sets the shear transform angles (x, y). Defined in degrees. Shearing happens relative to the control pivot point.
        /// </summary>
        [DefaultValue(typeof(Float2), "0,0")]
        [ExpandGroups, EditorDisplay("Transform"), EditorOrder(1040), Tooltip("The shear transform angles (x, y). Defined in degrees. Shearing happens relative to the control pivot point.")]
        public Float2 Shear
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
        /// Gets or sets the rotation angle (in degrees). Control is rotated around it's pivot point (middle of the control by default).
        /// </summary>
        [DefaultValue(0.0f)]
        [ExpandGroups, EditorDisplay("Transform"), EditorOrder(1050), Tooltip("The control rotation angle (in degrees). Control is rotated around it's pivot point (middle of the control by default).")]
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
            Float2 offset;
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
                offset = Float2.Zero;
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
            if (!_bounds.Location.Equals(ref prevBounds.Location))
            {
                OnLocationChanged();
            }

            if (!_bounds.Size.Equals(ref prevBounds.Size))
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
            //Float2.Multiply(ref _pivot, ref _bounds.Size, out var v1);
            //Float2.Negate(ref v1, out var v2);
            //Float2.Add(ref v1, ref _bounds.Location, out v1);
            var v1 = _pivot * _bounds.Size;
            var v2 = -v1;
            v1 += _bounds.Location;

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

            //Matrix2x2.Scale(ref _scale, out Matrix2x2 m1);
            //Matrix2x2.Shear(ref _shear, out Matrix2x2 m2);
            //Matrix2x2.Multiply(ref m1, ref m2, out m1);

            // Scale and Shear
            Matrix3x3 m1 = new Matrix3x3
                (
                _scale.X,
                _scale.X * (_shear.Y == 0 ? 0 : (1.0f / Mathf.Tan(Mathf.DegreesToRadians * (90 - Mathf.Clamp(_shear.Y, -89.0f, 89.0f))))),
                0,
                _scale.Y * (_shear.X == 0 ? 0 : (1.0f / Mathf.Tan(Mathf.DegreesToRadians * (90 - Mathf.Clamp(_shear.X, -89.0f, 89.0f))))),
                _scale.Y,
                0, 0, 0, 1
                );

            
            //Matrix2x2.Rotation(Mathf.DegreesToRadians * _rotation, out m2);
            float sin = Mathf.Sin(Mathf.DegreesToRadians * _rotation);
            float cos = Mathf.Cos(Mathf.DegreesToRadians * _rotation);

            //Matrix2x2.Multiply(ref m1, ref m2, out m1);
            m1.M11 = (_scale.X * cos) + (m1.M12 * -sin);
            m1.M12 = (_scale.X * sin) + (m1.M12 * cos);
            float m21 = (m1.M21 * cos) + (_scale.Y * -sin);
            m1.M22 = (m1.M21 * sin) + (_scale.Y * cos);
            m1.M21 = m21;
            // Mix all the stuff
            //Matrix3x3.Translation2D(ref v2, out Matrix3x3 m3);
            //Matrix3x3 m4 = (Matrix3x3)m1;
            //Matrix3x3.Multiply(ref m3, ref m4, out m3);
            //Matrix3x3.Translation2D(ref v1, out m4);
            //Matrix3x3.Multiply(ref m3, ref m4, out _cachedTransform);
            m1.M31 = (v2.X * m1.M11) + (v2.Y * m1.M21) + v1.X;
            m1.M32 = (v2.X * m1.M12) + (v2.Y * m1.M22) + v1.Y;
            _cachedTransform = m1;

            // Cache inverted transform
            Matrix3x3.Invert(ref _cachedTransform, out _cachedTransformInv);
        }

        /// <summary>
        /// Sets the anchor preset for the control. Can be used to auto-place the control for a given preset or can preserve the current control bounds.
        /// </summary>
        /// <param name="anchorPreset">The anchor preset to set.</param>
        /// <param name="preserveBounds">True if preserve current control bounds, otherwise will align control position accordingly to the anchor location.</param>
        /// <param name="setPivotToo">Whether or not we should set the pivot too, eg left-top 0,0, bottom-right 1,1</param>
        public void SetAnchorPreset(AnchorPresets anchorPreset, bool preserveBounds, bool setPivotToo = false)
        {
            for (int i = 0; i < AnchorPresetsData.Length; i++)
            {
                if (AnchorPresetsData[i].Preset == anchorPreset)
                {
                    var anchorMin = AnchorPresetsData[i].Min;
                    var anchorMax = AnchorPresetsData[i].Max;
                    var bounds = _bounds;
                    if (!Float2.NearEqual(ref _anchorMin, ref anchorMin) ||
                        !Float2.NearEqual(ref _anchorMax, ref anchorMax))
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
                                bounds.Location = Float2.Zero;
                                break;
                            case AnchorPresets.TopCenter:
                                bounds.Location = new Float2(parentBounds.Width * 0.5f - bounds.Width * 0.5f, 0);
                                break;
                            case AnchorPresets.TopRight:
                                bounds.Location = new Float2(parentBounds.Width - bounds.Width, 0);
                                break;
                            case AnchorPresets.MiddleLeft:
                                bounds.Location = new Float2(0, parentBounds.Height * 0.5f - bounds.Height * 0.5f);
                                break;
                            case AnchorPresets.MiddleCenter:
                                bounds.Location = new Float2(parentBounds.Width * 0.5f - bounds.Width * 0.5f, parentBounds.Height * 0.5f - bounds.Height * 0.5f);
                                break;
                            case AnchorPresets.MiddleRight:
                                bounds.Location = new Float2(parentBounds.Width - bounds.Width, parentBounds.Height * 0.5f - bounds.Height * 0.5f);
                                break;
                            case AnchorPresets.BottomLeft:
                                bounds.Location = new Float2(0, parentBounds.Height - bounds.Height);
                                break;
                            case AnchorPresets.BottomCenter:
                                bounds.Location = new Float2(parentBounds.Width * 0.5f - bounds.Width * 0.5f, parentBounds.Height - bounds.Height);
                                break;
                            case AnchorPresets.BottomRight:
                                bounds.Location = new Float2(parentBounds.Width - bounds.Width, parentBounds.Height - bounds.Height);
                                break;
                            case AnchorPresets.VerticalStretchLeft:
                                bounds.Location = Float2.Zero;
                                bounds.Size = new Float2(bounds.Width, parentBounds.Height);
                                break;
                            case AnchorPresets.VerticalStretchCenter:
                                bounds.Location = new Float2(parentBounds.Width * 0.5f - bounds.Width * 0.5f, 0);
                                bounds.Size = new Float2(bounds.Width, parentBounds.Height);
                                break;
                            case AnchorPresets.VerticalStretchRight:
                                bounds.Location = new Float2(parentBounds.Width - bounds.Width, 0);
                                bounds.Size = new Float2(bounds.Width, parentBounds.Height);
                                break;
                            case AnchorPresets.HorizontalStretchTop:
                                bounds.Location = Float2.Zero;
                                bounds.Size = new Float2(parentBounds.Width, bounds.Height);
                                break;
                            case AnchorPresets.HorizontalStretchMiddle:
                                bounds.Location = new Float2(0, parentBounds.Height * 0.5f - bounds.Height * 0.5f);
                                bounds.Size = new Float2(parentBounds.Width, bounds.Height);
                                break;
                            case AnchorPresets.HorizontalStretchBottom:
                                bounds.Location = new Float2(0, parentBounds.Height - bounds.Height);
                                bounds.Size = new Float2(parentBounds.Width, bounds.Height);
                                break;
                            case AnchorPresets.StretchAll:
                                bounds.Location = Float2.Zero;
                                bounds.Size = parentBounds.Size;
                                break;
                            default: throw new ArgumentOutOfRangeException(nameof(anchorPreset), anchorPreset, null);
                            }
                            bounds.Location += parentBounds.Location;
                        }
                        SetBounds(ref bounds);
                    }
                    if (setPivotToo)
                    {
                        Pivot = (anchorMin + anchorMax) / 2;
                    }
                    _parent?.PerformLayout();
                    return;
                }
            }
        }
    }
}
