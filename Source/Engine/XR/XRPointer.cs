// Copyright (c) Wojciech Figat. All rights reserved.

#if USE_LARGE_WORLDS
using Real = System.Double;
#else
using Real = System.Single;
#endif

using FlaxEngine.GUI;

namespace FlaxEngine
{
    /// <summary>
    /// The pointer that uses the XR controller to interact with the world-space UI canvases (points with the actor forward direction). Add it to the <see cref="XRController"/> that follows the aim pose. The controller trigger is used as the left mouse button.
    /// </summary>
    /// <seealso cref="FlaxEngine.Script" />
    public class XRPointer : Script
    {
        private CanvasRootControl _hitCanvas;
        private Float2 _hitLocation;
        private bool _isPressed;

        /// <summary>
        /// Gets the UI canvas under the pointer. Null if the pointer doesn't point at any canvas.
        /// </summary>
        public UICanvas HitCanvas => _hitCanvas?.Canvas;

        /// <summary>
        /// Gets the point where the pointer hits the UI canvas (in world-space). Valid only if <see cref="HitCanvas"/> is not null.
        /// </summary>
        public Vector3 HitPoint { get; private set; }

        /// <inheritdoc />
        public override void OnUpdate()
        {
            // Find the closest canvas under the pointer
            CanvasRootControl hitCanvas = null;
            var hitLocation = Float2.Zero;
            var hitPoint = Vector3.Zero;
            if (XR.IsActive && (!(Actor is XRController controller) || controller.IsTracked))
            {
                var ray = new Ray(Actor.Position, Actor.Transform.Forward);
                var hitDistance = Real.MaxValue;
                var layerMask = MainRenderTask.Instance?.ViewLayersMask ?? LayersMask.Default;
                var canvases = RootControl.CanvasRoot.Children;
                for (int i = 0; i < canvases.Count; i++)
                {
                    if (canvases[i] is CanvasRootControl canvas && canvas.Visible && canvas.Enabled && canvas.Is3D && layerMask.HasLayer(canvas.Canvas.Layer) && canvas.Intersects3D(ref ray, out var location))
                    {
                        canvas.Canvas.GetWorldMatrix(out var world);
                        var point = Vector3.Transform(new Vector3(location, 0), world);
                        var distance = Vector3.Distance(ray.Position, point);
                        if (distance < hitDistance)
                        {
                            hitDistance = distance;
                            hitCanvas = canvas;
                            hitLocation = location;
                            hitPoint = point;
                        }
                    }
                }
            }
            HitPoint = hitPoint;

            // Send mouse events to the canvas
            if (hitCanvas != _hitCanvas)
            {
                _hitCanvas?.OnMouseLeave();
                _hitCanvas = hitCanvas;
                _hitCanvas?.OnMouseEnter(hitLocation);
            }
            else if (hitCanvas != null && hitLocation != _hitLocation)
            {
                hitCanvas.OnMouseMove(hitLocation);
            }
            _hitLocation = hitLocation;
            var isPressed = IsTriggerPressed();
            if (isPressed != _isPressed)
            {
                _isPressed = isPressed;
                if (isPressed)
                    _hitCanvas?.OnMouseDown(hitLocation, MouseButton.Left);
                else
                    _hitCanvas?.OnMouseUp(hitLocation, MouseButton.Left);
            }
        }

        /// <inheritdoc />
        public override void OnDisable()
        {
            if (_isPressed)
                _hitCanvas?.OnMouseUp(_hitLocation, MouseButton.Left);
            _hitCanvas?.OnMouseLeave();
            _hitCanvas = null;
            _isPressed = false;
        }

        private bool IsTriggerPressed()
        {
            var gamepad = XR.Gamepad;
            if (gamepad == null)
                return false;
            var hand = Actor is XRController controller ? controller.Hand : XRHand.Right;
            return gamepad.GetButton(hand == XRHand.Left ? GamepadButton.LeftTrigger : GamepadButton.RightTrigger);
        }
    }
}
