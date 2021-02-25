// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

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
        private Vector2 _mousePosition;

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
        public override Vector2 TrackingMouseOffset => Vector2.Zero;

        /// <inheritdoc />
        public override Vector2 MousePosition
        {
            get => _mousePosition;
            set { }
        }

        /// <inheritdoc />
        public override void StartTrackingMouse(Control control, bool useMouseScreenOffset)
        {
            // Not used in games (editor-only feature)
        }

        /// <inheritdoc />
        public override void EndTrackingMouse()
        {
            // Not used in games (editor-only feature)
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
        public override Vector2 PointToParent(ref Vector2 location)
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
        public override bool ContainsPoint(ref Vector2 location)
        {
            return base.ContainsPoint(ref location)
                   && (_canvas.TestCanvasIntersection == null || _canvas.TestCanvasIntersection(ref location));
        }

        /// <inheritdoc />
        public override bool OnCharInput(char c)
        {
            if (!_canvas.ReceivesEvents)
                return false;

            return base.OnCharInput(c);
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragDrop(ref Vector2 location, DragData data)
        {
            if (!_canvas.ReceivesEvents)
                return DragDropEffect.None;

            return base.OnDragDrop(ref location, data);
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragEnter(ref Vector2 location, DragData data)
        {
            if (!_canvas.ReceivesEvents)
                return DragDropEffect.None;

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
        public override DragDropEffect OnDragMove(ref Vector2 location, DragData data)
        {
            if (!_canvas.ReceivesEvents)
                return DragDropEffect.None;

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
        public override bool OnMouseDoubleClick(Vector2 location, MouseButton button)
        {
            if (!_canvas.ReceivesEvents)
                return false;

            return base.OnMouseDoubleClick(location, button);
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Vector2 location, MouseButton button)
        {
            if (!_canvas.ReceivesEvents)
                return false;

            return base.OnMouseDown(location, button);
        }

        /// <inheritdoc />
        public override void OnMouseEnter(Vector2 location)
        {
            if (!_canvas.ReceivesEvents)
                return;

            _mousePosition = location;
            base.OnMouseEnter(location);
        }

        /// <inheritdoc />
        public override void OnMouseLeave()
        {
            _mousePosition = Vector2.Zero;

            if (!_canvas.ReceivesEvents)
                return;

            base.OnMouseLeave();
        }

        /// <inheritdoc />
        public override void OnMouseMove(Vector2 location)
        {
            if (!_canvas.ReceivesEvents)
                return;

            _mousePosition = location;
            base.OnMouseMove(location);
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Vector2 location, MouseButton button)
        {
            if (!_canvas.ReceivesEvents)
                return false;

            return base.OnMouseUp(location, button);
        }

        /// <inheritdoc />
        public override bool OnMouseWheel(Vector2 location, float delta)
        {
            if (!_canvas.ReceivesEvents)
                return false;

            return base.OnMouseWheel(location, delta);
        }

        /// <inheritdoc />
        public override void OnTouchEnter(Vector2 location, int pointerId)
        {
            if (!_canvas.ReceivesEvents)
                return;

            base.OnTouchEnter(location, pointerId);
        }

        /// <inheritdoc />
        public override bool OnTouchDown(Vector2 location, int pointerId)
        {
            if (!_canvas.ReceivesEvents)
                return false;

            return base.OnTouchDown(location, pointerId);
        }

        /// <inheritdoc />
        public override void OnTouchMove(Vector2 location, int pointerId)
        {
            if (!_canvas.ReceivesEvents)
                return;

            base.OnTouchMove(location, pointerId);
        }

        /// <inheritdoc />
        public override bool OnTouchUp(Vector2 location, int pointerId)
        {
            if (!_canvas.ReceivesEvents)
                return false;

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
