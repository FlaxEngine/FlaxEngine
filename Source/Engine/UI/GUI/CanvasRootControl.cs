// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine.GUI
{
    /// <summary>
    /// Root control implementation used by the <see cref="UICanvas"/> actor.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.RootControl" />
    [HideInEditor]
    public sealed class CanvasRootControl : RootControl
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
        
        private UICanvas _canvas;
        private Float2 _mousePosition;
        private float _navigationHeldTimeUp, _navigationHeldTimeDown, _navigationHeldTimeLeft, _navigationHeldTimeRight, _navigationHeldTimeSubmit;
        private float _navigationRateTimeUp, _navigationRateTimeDown, _navigationRateTimeLeft, _navigationRateTimeRight, _navigationRateTimeSubmit;

        // Canvas Scalar
        /// <summary>
        /// The canvas scaling mode.
        /// </summary>
        public ScalingMode CanvasScalingMode = ScalingMode.ConstantPixelSize;
        
        /// <summary>
        /// The canvas physical units.
        /// </summary>
        public PhysicalUnitMode CanvasPhysicalUnit = PhysicalUnitMode.Points;
        
        /// <summary>
        /// The canvas resolution mode.
        /// </summary>
        public ResolutionScalingMode CanvasResolutionMode = ResolutionScalingMode.ShortestSide;
        
        /// <summary>
        /// The canvas scale.
        /// </summary>
        public float CanvasScale = 1.0f;
        
        /// <summary>
        /// The canvas scale factor.
        /// </summary>
        public float CanvasScaleFactor = 1.0f;
        
        /// <summary>
        /// The canvas physical unit size.
        /// </summary>
        public float CanvasPhysicalUnitSize = 1.0f;
        
        /// <summary>
        /// The canvas resolution minimum.
        /// </summary>
        public Float2 CanvasResolutionMin = new Float2(1, 1);
        
        /// <summary>
        /// The canvas resolution maximum.
        /// </summary>
        public Float2 CanvasResolutionMax = new Float2(10000, 10000);
        
        /// <summary>
        /// Gets the owning canvas.
        /// </summary>
        public UICanvas Canvas => _canvas;

        /// <summary>
        /// Gets a value indicating whether canvas is 2D (screen-space).
        /// </summary>
        public bool Is2D => _canvas.RenderMode == CanvasRenderMode.ScreenSpace;

        /// <summary>
        /// Gets a value indicating whether canvas is 3D (world-space or camera-space).
        /// </summary>
        public bool Is3D => _canvas.RenderMode != CanvasRenderMode.ScreenSpace;

        /// <summary>
        /// Initializes a new instance of the <see cref="CanvasRootControl"/> class.
        /// </summary>
        /// <param name="canvas">The canvas.</param>
        internal CanvasRootControl(UICanvas canvas)
        {
            _canvas = canvas;
            
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
                    switch (CanvasScalingMode)
                    {
                    case ScalingMode.ConstantPixelSize:
                        scale = 1.0f;
                        break;
                    case ScalingMode.ConstantPhysicalSize:
                    {
                        float targetDpi = GetUnitDpi(CanvasPhysicalUnit);
                        scale = dpi / targetDpi * CanvasPhysicalUnitSize;
                        break;
                    }
                    case ScalingMode.ScaleWithResolution:
                    {
                        Float2 resolution = Float2.Max(Size, Float2.One);
                        int axis = 0;
                        switch (CanvasResolutionMode)
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
                        float min = CanvasResolutionMin[axis], max = CanvasResolutionMax[axis], value = resolution[axis];
                        if (value < min)
                            scale = min / value;
                        else if (value > max)
                            scale = max / value;
                        if (_canvas.ResolutionCurve != null && _canvas.ResolutionCurve.Keyframes?.Length != 0)
                        {
                            _canvas.ResolutionCurve.Evaluate(out var curveScale, value, false);
                            scale *= curveScale;
                        }
                        break;
                    }
                    case ScalingMode.ScaleWithDpi:
                        _canvas.DpiCurve?.Evaluate(out scale, dpi, false);
                        break;
                    }
                    break;
                }
            }
            CanvasScale = Mathf.Max(scale * CanvasScaleFactor, 0.01f);
        }

        /// <summary>
        /// Gets the unit's DPI based on the physical unit mode.
        /// </summary>
        /// <param name="unit"></param>
        /// <returns></returns>
        public float GetUnitDpi(PhysicalUnitMode unit)
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
                dpi = 1;
                break;
            case PhysicalUnitMode.Points:
                dpi = 72;
                break;
            case PhysicalUnitMode.Picas:
                dpi = 6;
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
#if FLAX_EDITOR
        /// <inheritdoc />
        public override Rectangle EditorBounds => new Rectangle(Float2.Zero, Size / CanvasScale);
#endif
        
        /// <inheritdoc />
        public override void Draw()
        {
            DrawSelf();

            // Draw children with scale
            var scaling = new Float3(CanvasScale, CanvasScale, 1);
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
            rect = new Rectangle(Float2.Zero, Size / CanvasScale);
        }
        
        /// <inheritdoc />
        public override bool IntersectsContent(ref Float2 locationParent, out Float2 location)
        {
            // Skip local PointFromParent but use base code
            location = base.PointFromParent(ref locationParent);
            return ContainsPoint(ref location);
        }

        /// <summary>
        /// Checks if the 3D canvas intersects with a given 3D mouse ray.
        /// </summary>
        /// <param name="ray">The input ray to test (in world-space).</param>
        /// <param name="canvasLocation">Output canvas-space local position.</param>
        /// <returns>True if canvas intersects with that point, otherwise false.</returns>
        public bool Intersects3D(ref Ray ray, out Float2 canvasLocation)
        {
            // Inline bounds calculations (it will reuse world matrix)
            var bounds = new OrientedBoundingBox
            {
                Extents = new Vector3(Size * 0.5f, Mathf.Epsilon)
            };

            _canvas.GetWorldMatrix(out var world);
            Matrix.Translation((float)bounds.Extents.X, (float)bounds.Extents.Y, 0, out var offset);
            Matrix.Multiply(ref offset, ref world, out var boxWorld);
            boxWorld.Decompose(out bounds.Transformation);

            // Hit test
            if (bounds.Intersects(ref ray, out Vector3 hitPoint))
            {
                // Transform world-space hit point to canvas local-space
                world.Invert();
                Vector3.Transform(ref hitPoint, ref world, out Vector3 localHitPoint);

                canvasLocation = new Float2(localHitPoint);
                return ContainsPoint(ref canvasLocation);
            }

            canvasLocation = Float2.Zero;
            return false;
        }

        /// <inheritdoc />
        public override CursorType Cursor
        {
            get => CursorType.Default;
            set { }
        }

        /// <inheritdoc />
        public override Control FocusedControl
        {
            get => Parent?.Root.FocusedControl;
            set
            {
                if (Parent != null)
                    Parent.Root.FocusedControl = value;
            }
        }

        /// <inheritdoc />
        public override Float2 TrackingMouseOffset => Float2.Zero;

        /// <inheritdoc />
        public override Float2 MousePosition
        {
            get => _mousePosition;
            set { }
        }

        /// <inheritdoc />
        public override void StartTrackingMouse(Control control, bool useMouseScreenOffset)
        {
            var parent = Parent?.Root;
            parent?.StartTrackingMouse(control, useMouseScreenOffset);
        }

        /// <inheritdoc />
        public override void EndTrackingMouse()
        {
            var parent = Parent?.Root;
            parent?.EndTrackingMouse();
        }

        /// <inheritdoc />
        public override bool GetKey(KeyboardKeys key)
        {
            return Input.GetKey(key);
        }

        /// <inheritdoc />
        public override bool GetKeyDown(KeyboardKeys key)
        {
            return Input.GetKeyDown(key);
        }

        /// <inheritdoc />
        public override bool GetKeyUp(KeyboardKeys key)
        {
            return Input.GetKeyUp(key);
        }

        /// <inheritdoc />
        public override bool GetMouseButton(MouseButton button)
        {
            return Input.GetMouseButton(button);
        }

        /// <inheritdoc />
        public override bool GetMouseButtonDown(MouseButton button)
        {
            return Input.GetMouseButtonDown(button);
        }

        /// <inheritdoc />
        public override bool GetMouseButtonUp(MouseButton button)
        {
            return Input.GetMouseButtonUp(button);
        }

        /// <inheritdoc />
        public override Float2 PointToParent(ref Float2 location)
        {
            if (Is2D)
            {
                var result = base.PointToParent(ref location);
                result *= CanvasScale;
                return result;
            }

            var camera = Camera.MainCamera;
            if (!camera)
                return location;

            // Transform canvas local-space point to the game root location
            _canvas.GetWorldMatrix(out Matrix world);
            Vector3 locationCanvasSpace = new Vector3(location, 0.0f);
            Vector3.Transform(ref locationCanvasSpace, ref world, out Vector3 locationWorldSpace);
            camera.ProjectPoint(locationWorldSpace, out location);
            return location;
        }

        /// <inheritdoc />
        public override Float2 PointFromParent(ref Float2 locationParent)
        {
            if (Is2D)
            {
                var result = base.PointFromParent(ref locationParent);
                result /= CanvasScale;
                return result;
            }

            var camera = Camera.MainCamera;
            if (!camera)
                return locationParent;

            // Use world-space ray to convert it to the local-space of the canvas
            UICanvas.CalculateRay(ref locationParent, out Ray ray);
            Intersects3D(ref ray, out var location);
            return location;
        }

        /// <inheritdoc />
        public override bool ContainsPoint(ref Float2 location)
        {
            return base.ContainsPoint(ref location)
                   && (_canvas.TestCanvasIntersection == null || _canvas.TestCanvasIntersection(ref location));
        }

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            // UI navigation
            if (_canvas.ReceivesEvents)
            {
                UpdateNavigation(deltaTime, _canvas.NavigateUp.Name, NavDirection.Up, ref _navigationHeldTimeUp, ref _navigationRateTimeUp);
                UpdateNavigation(deltaTime, _canvas.NavigateDown.Name, NavDirection.Down, ref _navigationHeldTimeDown, ref _navigationRateTimeDown);
                UpdateNavigation(deltaTime, _canvas.NavigateLeft.Name, NavDirection.Left, ref _navigationHeldTimeLeft, ref _navigationRateTimeLeft);
                UpdateNavigation(deltaTime, _canvas.NavigateRight.Name, NavDirection.Right, ref _navigationHeldTimeRight, ref _navigationRateTimeRight);
                UpdateNavigation(deltaTime, _canvas.NavigateSubmit.Name, ref _navigationHeldTimeSubmit, ref _navigationRateTimeSubmit, SubmitFocused);
            }
            else
            {
                _navigationHeldTimeUp = _navigationHeldTimeDown = _navigationHeldTimeLeft = _navigationHeldTimeRight = 0;
                _navigationRateTimeUp = _navigationRateTimeDown = _navigationRateTimeLeft = _navigationRateTimeRight = 0;
            }

            base.Update(deltaTime);
        }

        private void UpdateNavigation(float deltaTime, string actionName, NavDirection direction, ref float heldTime, ref float rateTime)
        {
            if (Input.GetAction(actionName))
            {
                if (heldTime <= Mathf.Epsilon)
                {
                    Navigate(direction);
                }
                if (heldTime > _canvas.NavigationInputRepeatDelay)
                {
                    rateTime += deltaTime;
                }
                if (rateTime > _canvas.NavigationInputRepeatRate)
                {
                    Navigate(direction);
                    rateTime = 0;
                }
                heldTime += deltaTime;
            }
            else
            {
                heldTime = rateTime = 0;
            }
        }

        private void UpdateNavigation(float deltaTime, string actionName, ref float heldTime, ref float rateTime, Action action)
        {
            if (Input.GetAction(actionName))
            {
                if (heldTime <= Mathf.Epsilon)
                {
                    action();
                }
                if (heldTime > _canvas.NavigationInputRepeatDelay)
                {
                    rateTime += deltaTime;
                }
                if (rateTime > _canvas.NavigationInputRepeatRate)
                {
                    action();
                    rateTime = 0;
                }
                heldTime += deltaTime;
            }
            else
            {
                heldTime = rateTime = 0;
            }
        }

        /// <inheritdoc />
        public override bool OnCharInput(char c)
        {
            if (!_canvas.ReceivesEvents)
                return false;

            return base.OnCharInput(c);
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragDrop(ref Float2 location, DragData data)
        {
            if (!_canvas.ReceivesEvents)
                return DragDropEffect.None;

            location /= CanvasScale;
            return base.OnDragDrop(ref location, data);
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragEnter(ref Float2 location, DragData data)
        {
            if (!_canvas.ReceivesEvents)
                return DragDropEffect.None;

            location /= CanvasScale;
            return base.OnDragEnter(ref location, data);
        }

        /// <inheritdoc />
        public override void OnDragLeave()
        {
            if (!_canvas.ReceivesEvents)
                return;

            base.OnDragLeave();
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragMove(ref Float2 location, DragData data)
        {
            if (!_canvas.ReceivesEvents)
                return DragDropEffect.None;

            location /= CanvasScale;
            return base.OnDragMove(ref location, data);
        }

        /// <inheritdoc />
        public override bool OnKeyDown(KeyboardKeys key)
        {
            if (!_canvas.ReceivesEvents)
                return false;

            return base.OnKeyDown(key);
        }

        /// <inheritdoc />
        public override void OnKeyUp(KeyboardKeys key)
        {
            if (!_canvas.ReceivesEvents)
                return;

            base.OnKeyUp(key);
        }

        /// <inheritdoc />
        public override bool OnMouseDoubleClick(Float2 location, MouseButton button)
        {
            if (!_canvas.ReceivesEvents)
                return false;

            location /= CanvasScale;
            return base.OnMouseDoubleClick(location, button);
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (!_canvas.ReceivesEvents)
                return false;

            location /= CanvasScale;
            return base.OnMouseDown(location, button);
        }

        /// <inheritdoc />
        public override void OnMouseEnter(Float2 location)
        {
            if (!_canvas.ReceivesEvents)
                return;

            location /= CanvasScale;
            _mousePosition = location;
            base.OnMouseEnter(location);
        }

        /// <inheritdoc />
        public override void OnMouseLeave()
        {
            _mousePosition = Float2.Zero;

            if (!_canvas.ReceivesEvents)
                return;

            base.OnMouseLeave();
        }

        /// <inheritdoc />
        public override void OnMouseMove(Float2 location)
        {
            if (!_canvas.ReceivesEvents)
                return;

            location /= CanvasScale;
            _mousePosition = location;
            base.OnMouseMove(location);
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (!_canvas.ReceivesEvents)
                return false;

            location /= CanvasScale;
            return base.OnMouseUp(location, button);
        }

        /// <inheritdoc />
        public override bool OnMouseWheel(Float2 location, float delta)
        {
            if (!_canvas.ReceivesEvents)
                return false;

            location /= CanvasScale;
            return base.OnMouseWheel(location, delta);
        }

        /// <inheritdoc />
        public override void OnTouchEnter(Float2 location, int pointerId)
        {
            if (!_canvas.ReceivesEvents)
                return;

            location /= CanvasScale;
            base.OnTouchEnter(location, pointerId);
        }

        /// <inheritdoc />
        public override bool OnTouchDown(Float2 location, int pointerId)
        {
            if (!_canvas.ReceivesEvents)
                return false;

            location /= CanvasScale;
            return base.OnTouchDown(location, pointerId);
        }

        /// <inheritdoc />
        public override void OnTouchMove(Float2 location, int pointerId)
        {
            if (!_canvas.ReceivesEvents)
                return;

            location /= CanvasScale;
            base.OnTouchMove(location, pointerId);
        }

        /// <inheritdoc />
        public override bool OnTouchUp(Float2 location, int pointerId)
        {
            if (!_canvas.ReceivesEvents)
                return false;

            location /= CanvasScale;
            return base.OnTouchUp(location, pointerId);
        }

        /// <inheritdoc />
        public override void OnTouchLeave(int pointerId)
        {
            if (!_canvas.ReceivesEvents)
                return;

            base.OnTouchLeave(pointerId);
        }

        /// <inheritdoc />
        public override void Focus()
        {
        }

        /// <inheritdoc />
        public override void DoDragDrop(DragData data)
        {
        }
    }
}
