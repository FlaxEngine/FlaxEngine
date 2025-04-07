// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;

namespace FlaxEngine.GUI
{
    /// <summary>
    /// Root control implementation used by the <see cref="UICanvas"/> actor.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.RootControl" />
    [HideInEditor]
    public sealed class CanvasRootControl : RootControl
    {
        private UICanvas _canvas;
        private Float2 _mousePosition;
        private float _navigationHeldTimeUp, _navigationHeldTimeDown, _navigationHeldTimeLeft, _navigationHeldTimeRight, _navigationHeldTimeSubmit;
        private float _navigationRateTimeUp, _navigationRateTimeDown, _navigationRateTimeLeft, _navigationRateTimeRight, _navigationRateTimeSubmit;

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
        }

        /// <summary>
        /// Checks if the 3D canvas intersects with a given 3D mouse ray.
        /// </summary>
        /// <param name="ray">The input ray to test (in world-space).</param>
        /// <param name="canvasLocation">Output canvas-space local position.</param>
        /// <param name="precise">True if perform precise intersection test against the control content (eg. with hit mask or transparency threshold). Otherwise, only simple bounds-check will be performed.</param>
        /// <returns>True if canvas intersects with that point, otherwise false.</returns>
        public bool Intersects3D(ref Ray ray, out Float2 canvasLocation, bool precise = false)
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
                return ContainsPoint(ref canvasLocation, precise);
            }

            canvasLocation = Float2.Zero;
            return false;
        }

        private bool SkipEvents => !_canvas.ReceivesEvents || !_canvas.IsVisible();

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
                return base.PointToParent(ref location);

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
                return base.PointFromParent(ref locationParent);

            var camera = Camera.MainCamera;
            if (!camera)
                return locationParent;

            // Use world-space ray to convert it to the local-space of the canvas
            UICanvas.CalculateRay(ref locationParent, out Ray ray);
            Intersects3D(ref ray, out var location);
            return location;
        }

        /// <inheritdoc />
        public override bool ContainsPoint(ref Float2 location, bool precise)
        {
            return base.ContainsPoint(ref location, precise)
                   && (_canvas.TestCanvasIntersection == null || _canvas.TestCanvasIntersection(ref location));
        }

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            base.Update(deltaTime);

            // Update navigation
            if (SkipEvents)
            {
                _navigationHeldTimeUp = _navigationHeldTimeDown = _navigationHeldTimeLeft = _navigationHeldTimeRight = 0;
                _navigationRateTimeUp = _navigationRateTimeDown = _navigationRateTimeLeft = _navigationRateTimeRight = 0;
                return;
            }
            if (ContainsFocus || IndexInParent == 0)
            {
                UpdateNavigation(deltaTime, _canvas.NavigateUp.Name, NavDirection.Up, ref _navigationHeldTimeUp, ref _navigationRateTimeUp);
                UpdateNavigation(deltaTime, _canvas.NavigateDown.Name, NavDirection.Down, ref _navigationHeldTimeDown, ref _navigationRateTimeDown);
                UpdateNavigation(deltaTime, _canvas.NavigateLeft.Name, NavDirection.Left, ref _navigationHeldTimeLeft, ref _navigationRateTimeLeft);
                UpdateNavigation(deltaTime, _canvas.NavigateRight.Name, NavDirection.Right, ref _navigationHeldTimeRight, ref _navigationRateTimeRight);
                UpdateNavigation(deltaTime, _canvas.NavigateSubmit.Name, ref _navigationHeldTimeSubmit, ref _navigationRateTimeSubmit, SubmitFocused);
            }
        }

        private void ConditionalNavigate(NavDirection direction)
        {
            // Only currently focused canvas updates its navigation
            if (!ContainsFocus)
            {
                // Special case when no canvas nor game UI is focused so let the first canvas to start the navigation into the UI
                if (IndexInParent == 0 && Parent is CanvasContainer canvasContainer && !canvasContainer.ContainsFocus && GameRoot.ContainsFocus)
                {
                    // Nothing is focused so go to the first control
                    var focused = OnNavigate(direction, Float2.Zero, this, new List<Control>());
                    focused?.NavigationFocus();
                    return;
                }
            }
            Navigate(direction);
        }

        private void UpdateNavigation(float deltaTime, string actionName, NavDirection direction, ref float heldTime, ref float rateTime)
        {
            if (Input.GetAction(actionName))
            {
                if (heldTime <= Mathf.Epsilon)
                {
                    ConditionalNavigate(direction);
                }
                if (heldTime > _canvas.NavigationInputRepeatDelay)
                {
                    rateTime += deltaTime;
                }
                if (rateTime > _canvas.NavigationInputRepeatRate)
                {
                    ConditionalNavigate(direction);
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
            if (SkipEvents)
                return false;

            return base.OnCharInput(c);
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragDrop(ref Float2 location, DragData data)
        {
            if (SkipEvents)
                return DragDropEffect.None;

            return base.OnDragDrop(ref location, data);
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragEnter(ref Float2 location, DragData data)
        {
            if (SkipEvents)
                return DragDropEffect.None;

            return base.OnDragEnter(ref location, data);
        }

        /// <inheritdoc />
        public override void OnDragLeave()
        {
            if (SkipEvents)
                return;

            base.OnDragLeave();
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragMove(ref Float2 location, DragData data)
        {
            if (SkipEvents)
                return DragDropEffect.None;

            return base.OnDragMove(ref location, data);
        }

        /// <inheritdoc />
        public override bool OnKeyDown(KeyboardKeys key)
        {
            if (SkipEvents)
                return false;

            return base.OnKeyDown(key);
        }

        /// <inheritdoc />
        public override void OnKeyUp(KeyboardKeys key)
        {
            if (SkipEvents)
                return;

            base.OnKeyUp(key);
        }

        /// <inheritdoc />
        public override bool OnMouseDoubleClick(Float2 location, MouseButton button)
        {
            if (SkipEvents)
                return false;

            return base.OnMouseDoubleClick(location, button);
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (SkipEvents)
                return false;

            return base.OnMouseDown(location, button);
        }

        /// <inheritdoc />
        public override void OnMouseEnter(Float2 location)
        {
            if (SkipEvents)
                return;

            _mousePosition = location;
            base.OnMouseEnter(location);
        }

        /// <inheritdoc />
        public override void OnMouseLeave()
        {
            _mousePosition = Float2.Zero;
            if (SkipEvents)
                return;

            base.OnMouseLeave();
        }

        /// <inheritdoc />
        public override void OnMouseMove(Float2 location)
        {
            if (SkipEvents)
                return;

            _mousePosition = location;
            base.OnMouseMove(location);
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (SkipEvents)
                return false;

            return base.OnMouseUp(location, button);
        }

        /// <inheritdoc />
        public override bool OnMouseWheel(Float2 location, float delta)
        {
            if (SkipEvents)
                return false;

            return base.OnMouseWheel(location, delta);
        }

        /// <inheritdoc />
        public override void OnTouchEnter(Float2 location, int pointerId)
        {
            if (SkipEvents)
                return;

            base.OnTouchEnter(location, pointerId);
        }

        /// <inheritdoc />
        public override bool OnTouchDown(Float2 location, int pointerId)
        {
            if (SkipEvents)
                return false;

            return base.OnTouchDown(location, pointerId);
        }

        /// <inheritdoc />
        public override void OnTouchMove(Float2 location, int pointerId)
        {
            if (SkipEvents)
                return;

            base.OnTouchMove(location, pointerId);
        }

        /// <inheritdoc />
        public override bool OnTouchUp(Float2 location, int pointerId)
        {
            if (SkipEvents)
                return false;

            return base.OnTouchUp(location, pointerId);
        }

        /// <inheritdoc />
        public override void OnTouchLeave(int pointerId)
        {
            if (SkipEvents)
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
