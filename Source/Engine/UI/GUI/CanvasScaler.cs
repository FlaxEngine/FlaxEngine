// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System.ComponentModel;

namespace FlaxEngine.GUI
{
    /// <summary>
    /// UI canvas scaling component for user interface that targets multiple different game resolutions (eg. mobile screens).
    /// </summary>
    [ActorToolbox("GUI")]
    public class CanvasScaler : ContainerControl
    {
        /// <summary>
        /// Canvas scaling modes.
        /// </summary>
        public enum ScalingMode
        {
            /// <summary>
            /// Applies constant scale to the whole UI in pixels.
            /// </summary>
            ConstantPixelSize,

            /// <summary>
            /// Applies constant scale to the whole UI in physical units (depends on the screen DPI). Ensures the UI will have specific real-world size no matter the screen resolution.
            /// </summary>
            ConstantPhysicalSize,

            /// <summary>
            /// Applies min/max scaling to the UI depending on the screen resolution. Ensures the UI size won't go below min or above max resolution to maintain it's readability.
            /// </summary>
            ScaleWithResolution,

            /// <summary>
            /// Applies scaling curve to the UI depending on the screen DPI.
            /// </summary>
            ScaleWithDpi,
        }

        /// <summary>
        /// Physical unit types for canvas scaling.
        /// </summary>
        public enum PhysicalUnitMode
        {
            /// <summary>
            /// Centimeters (0.01 meter).
            /// </summary>
            Centimeters,

            /// <summary>
            /// Millimeters (0.1 centimeter, 0.001 meter).
            /// </summary>
            Millimeters,

            /// <summary>
            /// Inches (2.54 centimeters).
            /// </summary>
            Inches,

            /// <summary>
            /// Points (1/72 inch, 1/112 of pica).
            /// </summary>
            Points,

            /// <summary>
            /// Pica (1/6 inch).
            /// </summary>
            Picas,
        }

        /// <summary>
        /// Resolution scaling modes.
        /// </summary>
        public enum ResolutionScalingMode
        {
            /// <summary>
            /// Uses the shortest side of the screen to scale the canvas for min/max rule.
            /// </summary>
            ShortestSide,

            /// <summary>
            /// Uses the longest side of the screen to scale the canvas for min/max rule.
            /// </summary>
            LongestSide,

            /// <summary>
            /// Uses the horizontal (X, width) side of the screen to scale the canvas for min/max rule.
            /// </summary>
            Horizontal,

            /// <summary>
            /// Uses the vertical (Y, height) side of the screen to scale the canvas for min/max rule.
            /// </summary>
            Vertical,
        }

        private ScalingMode _scalingMode = ScalingMode.ConstantPixelSize;
        private PhysicalUnitMode _physicalUnit = PhysicalUnitMode.Points;
        private ResolutionScalingMode _resolutionMode = ResolutionScalingMode.ShortestSide;
        private float _scale = 1.0f;
        private float _scaleFactor = 1.0f;
        private float _physicalUnitSize = 1.0f;
        private Float2 _resolutionMin = new Float2(1f, 1f);
        private Float2 _resolutionMax = new Float2(7680f, 4320f);

        /// <summary>
        /// Gets the current UI scale. Computed based on the setup when performing layout.
        /// </summary>
        public float CurrentScale => _scale;

        /// <summary>
        /// The UI Canvas scaling mode.
        /// </summary>
        [EditorOrder(0), EditorDisplay("Canvas Scaler"), ExpandGroups, DefaultValue(ScalingMode.ConstantPixelSize)]
        public ScalingMode Scaling
        {
            get => _scalingMode;
            set
            {
                if (_scalingMode == value)
                    return;
                _scalingMode = value;
                PerformLayout();
            }
        }

        /// <summary>
        /// The UI Canvas scale. Applied in all scaling modes for custom UI sizing.
        /// </summary>
        [EditorOrder(10), EditorDisplay("Canvas Scaler"), DefaultValue(1.0f), Limit(0.001f, 1000.0f, 0.01f)]
        public float ScaleFactor
        {
            get => _scaleFactor;
            set
            {
                if (Mathf.NearEqual(_scaleFactor, value))
                    return;
                _scaleFactor = value;
                PerformLayout();
            }
        }

        /// <summary>
        /// The UI Canvas physical unit to use for scaling via PhysicalUnitSize. Used only in ConstantPhysicalSize mode.
        /// </summary>
#if FLAX_EDITOR
        [EditorOrder(100), EditorDisplay("Canvas Scaler"), DefaultValue(PhysicalUnitMode.Points), VisibleIf(nameof(IsConstantPhysicalSize))]
#endif
        public PhysicalUnitMode PhysicalUnit
        {
            get => _physicalUnit;
            set
            {
                if (_physicalUnit == value)
                    return;
                _physicalUnit = value;
#if FLAX_EDITOR
                if (FlaxEditor.CustomEditors.CustomEditor.IsSettingValue)
                {
                    // Set auto-default physical unit value for easier tweaking in Editor
                    _physicalUnitSize = GetUnitDpi(_physicalUnit) / Platform.Dpi;
                }
#endif
                PerformLayout();
            }
        }

        /// <summary>
        /// The UI Canvas physical unit value. Used only in ConstantPhysicalSize mode.
        /// </summary>
#if FLAX_EDITOR
        [EditorOrder(110), EditorDisplay("Canvas Scaler"), DefaultValue(1.0f), Limit(0.000001f, 1000000.0f, 0.0f), VisibleIf(nameof(IsConstantPhysicalSize))]
#endif
        public float PhysicalUnitSize
        {
            get => _physicalUnitSize;
            set
            {
                if (Mathf.NearEqual(_physicalUnitSize, value))
                    return;
                _physicalUnitSize = value;
                PerformLayout();
            }
        }

        /// <summary>
        /// The UI Canvas resolution scaling mode. Controls min/max resolutions usage in relation to the current screen resolution to compute the UI scale. Used only in ScaleWithResolution mode.
        /// </summary>
#if FLAX_EDITOR
        [EditorOrder(120), EditorDisplay("Canvas Scaler"), VisibleIf(nameof(IsScaleWithResolution))]
#endif
        public ResolutionScalingMode ResolutionMode
        {
            get => _resolutionMode;
            set
            {
                if (_resolutionMode == value)
                    return;
                _resolutionMode = value;
                PerformLayout();
            }
        }

        /// <summary>
        /// The UI Canvas minimum resolution. If the screen has lower size, then the interface will be scaled accordingly. Used only in ScaleWithResolution mode.
        /// </summary>
#if FLAX_EDITOR
        [EditorOrder(120), EditorDisplay("Canvas Scaler"), VisibleIf(nameof(IsScaleWithResolution))]
#endif
        public Float2 ResolutionMin
        {
            get => _resolutionMin;
            set
            {
                value = Float2.Max(value, Float2.One);
                if (Float2.NearEqual(ref _resolutionMin, ref value))
                    return;
                _resolutionMin = value;
                PerformLayout();
            }
        }

        /// <summary>
        /// The UI Canvas maximum resolution. If the screen has higher size, then the interface will be scaled accordingly. Used only in ScaleWithResolution mode.
        /// </summary>
#if FLAX_EDITOR
        [EditorOrder(130), EditorDisplay("Canvas Scaler"), VisibleIf(nameof(IsScaleWithResolution))]
#endif
        public Float2 ResolutionMax
        {
            get => _resolutionMax;
            set
            {
                value = Float2.Max(value, Float2.One);
                if (Float2.NearEqual(ref _resolutionMax, ref value))
                    return;
                _resolutionMax = value;
                PerformLayout();
            }
        }

        /// <summary>
        /// The UI Canvas scaling curve based on screen resolution - shortest/longest/vertical/horizontal (key is resolution, value is scale factor). Clear keyframes to skip using it and follow min/max rules only. Used only in ScaleWithResolution mode.
        /// </summary>
#if FLAX_EDITOR
        [EditorOrder(140), EditorDisplay("Canvas Scaler"), VisibleIf(nameof(IsScaleWithResolution))]
#endif
        public LinearCurve<float> ResolutionCurve = new LinearCurve<float>(new[]
        {
            new LinearCurve<float>.Keyframe(0f, 0f), // 0p
            new LinearCurve<float>.Keyframe(480f, 0.444f), // 480p
            new LinearCurve<float>.Keyframe(720f, 0.666f), // 720p
            new LinearCurve<float>.Keyframe(1080f, 1.0f), // 1080p
            new LinearCurve<float>.Keyframe(8640f, 8.0f), // 8640p
        });

        /// <summary>
        /// The UI Canvas scaling curve based on screen DPI (key is DPI, value is scale factor). Used only in ScaleWithDpi mode.
        /// </summary>
#if FLAX_EDITOR
        [EditorOrder(150), EditorDisplay("Canvas Scaler"), VisibleIf(nameof(IsScaleWithDpi))]
#endif
        public LinearCurve<float> DpiCurve = new LinearCurve<float>(new[]
        {
            new LinearCurve<float>.Keyframe(1.0f, 1.0f),
            new LinearCurve<float>.Keyframe(96.0f, 1.0f),
            new LinearCurve<float>.Keyframe(200.0f, 2.0f),
            new LinearCurve<float>.Keyframe(400.0f, 4.0f),
        });

#if FLAX_EDITOR
        private bool IsConstantPhysicalSize => _scalingMode == ScalingMode.ConstantPhysicalSize;
        private bool IsScaleWithResolution => _scalingMode == ScalingMode.ScaleWithResolution;
        private bool IsScaleWithDpi => _scalingMode == ScalingMode.ScaleWithDpi;
#endif

        /// <summary>
        /// Initializes a new instance of the <see cref="CanvasScaler"/> class.
        /// </summary>
        public CanvasScaler()
        {
            // Fill the canvas by default
            Offsets = Margin.Zero;
            AnchorPreset = AnchorPresets.StretchAll;
            AutoFocus = false;
        }

        /// <summary>
        /// Updates the scaler for the current setup.
        /// </summary>
        public void UpdateScale()
        {
            float scale = 1.0f;
            if (Parent != null)
            {
                UICanvas canvas = (Root as CanvasRootControl)?.Canvas;
                float dpi = Platform.Dpi;
                switch (canvas?.RenderMode ?? CanvasRenderMode.ScreenSpace)
                {
                case CanvasRenderMode.WorldSpace:
                case CanvasRenderMode.WorldSpaceFaceCamera:
                    scale = 1.0f;
                    break;
                default:
                    switch (_scalingMode)
                    {
                    case ScalingMode.ConstantPixelSize:
                        scale = 1.0f;
                        break;
                    case ScalingMode.ConstantPhysicalSize:
                    {
                        float targetDpi = GetUnitDpi(_physicalUnit);
                        scale = dpi / targetDpi * _physicalUnitSize;
                        break;
                    }
                    case ScalingMode.ScaleWithResolution:
                    {
                        Float2 resolution = Float2.Max(Size, Float2.One);
                        int axis = 0;
                        switch (_resolutionMode)
                        {
                        case ResolutionScalingMode.ShortestSide:
                            axis = resolution.X > resolution.Y ? 1 : 0;
                            break;
                        case ResolutionScalingMode.LongestSide:
                            axis = resolution.X > resolution.Y ? 0 : 1;
                            break;
                        case ResolutionScalingMode.Horizontal:
                            axis = 0;
                            break;
                        case ResolutionScalingMode.Vertical:
                            axis = 1;
                            break;
                        }
                        float min = _resolutionMin[axis], max = _resolutionMax[axis], value = resolution[axis];
                        if (value < min)
                            scale = min / value;
                        else if (value > max)
                            scale = max / value;
                        if (ResolutionCurve != null && ResolutionCurve.Keyframes?.Length != 0f)
                        {
                            ResolutionCurve.Evaluate(out var curveScale, value, false);
                            scale *= curveScale;
                        }
                        break;
                    }
                    case ScalingMode.ScaleWithDpi:
                        DpiCurve?.Evaluate(out scale, dpi, false);
                        break;
                    }
                    break;
                }
            }
            _scale = Mathf.Max(scale * _scaleFactor, 0.01f);
        }

        private float GetUnitDpi(PhysicalUnitMode unit)
        {
            float dpi = 1.0f;
            switch (unit)
            {
            case PhysicalUnitMode.Centimeters:
                dpi = 2.54f;
                break;
            case PhysicalUnitMode.Millimeters:
                dpi = 25.4f;
                break;
            case PhysicalUnitMode.Inches:
                dpi = 1f;
                break;
            case PhysicalUnitMode.Points:
                dpi = 72f;
                break;
            case PhysicalUnitMode.Picas:
                dpi = 6f;
                break;
            }
            return dpi;
        }

        /// <inheritdoc />
        protected override void PerformLayoutBeforeChildren()
        {
            // Update current scaling before performing layout
            UpdateScale();

            base.PerformLayoutBeforeChildren();
        }

        #region UI Scale

#if FLAX_EDITOR
        /// <inheritdoc />
        public override Rectangle EditorBounds => new Rectangle(Float2.Zero, Size / _scale);
#endif

        /// <inheritdoc />
        public override void Draw()
        {
            DrawSelf();

            // Draw children with scale
            var scaling = new Float3(_scale, _scale, 1);
            Matrix3x3.Scaling(ref scaling, out Matrix3x3 scale);
            Render2D.PushTransform(scale);
            if (ClipChildren)
            {
                GetDesireClientArea(out var clientArea);
                Render2D.PushClip(ref clientArea);
                DrawChildren();
                Render2D.PopClip();
            }
            else
            {
                DrawChildren();
            }
            Render2D.PopTransform();
        }

        /// <inheritdoc />
        public override void GetDesireClientArea(out Rectangle rect)
        {
            // Scale the area for the client controls
            rect = new Rectangle(Float2.Zero, Size / _scale);
        }

        /// <inheritdoc />
        public override bool IntersectsContent(ref Float2 locationParent, out Float2 location)
        {
            // Skip local PointFromParent but use base code
            location = base.PointFromParent(ref locationParent);
            return ContainsPoint(ref location);
        }

        /// <inheritdoc />
        public override bool IntersectsChildContent(Control child, Float2 location, out Float2 childSpaceLocation)
        {
            location /= _scale;
            return base.IntersectsChildContent(child, location, out childSpaceLocation);
        }

        /// <inheritdoc />
        public override bool ContainsPoint(ref Float2 location, bool precise = false)
        {
            if (precise) // Ignore as utility-only element
                return false;
            return base.ContainsPoint(ref location, precise);
        }

        /// <inheritdoc />
        public override bool RayCast(ref Float2 location, out Control hit)
        {
            var p = location / _scale;
            if (RayCastChildren(ref p, out hit))
                return true;
            return base.RayCast(ref location, out hit);
        }

        /// <inheritdoc />
        public override Float2 PointToParent(ref Float2 location)
        {
            var result = base.PointToParent(ref location);
            result *= _scale;
            return result;
        }

        /// <inheritdoc />
        public override Float2 PointFromParent(ref Float2 location)
        {
            var result = base.PointFromParent(ref location);
            result /= _scale;
            return result;
        }

        #endregion
    }
}
