// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

namespace FlaxEngine.GUI
{
    /// <summary>
    /// The root container control used to sort and manage child UICanvas controls. Helps with sending input events.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.ContainerControl" />
    [HideInEditor]
    internal sealed class CanvasContainer : ContainerControl
    {
        internal CanvasContainer()
        {
            AnchorPreset = AnchorPresets.StretchAll;
            Offsets = Margin.Zero;
            AutoFocus = false;
        }

        /// <summary>
        /// Sorts the canvases by order.
        /// </summary>
        public void SortCanvases()
        {
            Children.Sort(SortCanvas);
        }

        private int SortCanvas(Control a, Control b)
        {
            return ((CanvasRootControl)a).Canvas.Order - ((CanvasRootControl)b).Canvas.Order;
        }

        private bool RayCast3D(ref Float2 location, out Control hit, out Float2 hitLocation)
        {
            hit = null;
            hitLocation = Float2.Zero;

            // Calculate 3D mouse ray
            UICanvas.CalculateRay(ref location, out Ray ray);

            // Test 3D canvases
            var layerMask = MainRenderTask.Instance?.ViewLayersMask ?? LayersMask.Default;
            for (int i = _children.Count - 1; i >= 0 && _children.Count > 0; i--)
            {
                var child = (CanvasRootControl)_children[i];
                if (child.Visible && child.Enabled && child.Is3D && layerMask.HasLayer(child.Canvas.Layer))
                {
                    if (child.Intersects3D(ref ray, out var childLocation))
                    {
                        hit = child;
                        hitLocation = childLocation;
                        return true;
                    }
                }
            }

            return false;
        }

        /// <inheritdoc />
        public override void OnChildrenChanged()
        {
            SortCanvases();

            base.OnChildrenChanged();
        }

        /// <inheritdoc />
        protected override void DrawChildren()
        {
            // Draw all screen space canvases
            var layerMask = MainRenderTask.Instance?.ViewLayersMask ?? LayersMask.Default;
            for (int i = 0; i < _children.Count; i++)
            {
                var child = (CanvasRootControl)_children[i];

                if (child.Visible && child.Is2D && layerMask.HasLayer(child.Canvas.Layer))
                {
                    child.Draw();
                }
            }
        }

        /// <inheritdoc />
        public override bool IntersectsChildContent(Control child, Float2 location, out Float2 childSpaceLocation)
        {
            childSpaceLocation = Float2.Zero;
            return ((CanvasRootControl)child).Is2D && base.IntersectsChildContent(child, location, out childSpaceLocation);
        }

        /// <inheritdoc />
        public override bool RayCast(ref Float2 location, out Control hit)
        {
            // Ignore self and only test children
            if (RayCastChildren(ref location, out hit))
                return true;

            // Test 3D
            if (RayCast3D(ref location, out hit, out var hitLocation))
            {
                // Test deeper to hit actual control not the root
                hit.RayCast(ref hitLocation, out hit);
                if (hit != null)
                    return true;
            }

            return false;
        }

        /// <inheritdoc />
        public override bool ContainsPoint(ref Float2 location, bool precise = false)
        {
            if (precise) // Ignore as utility-only element
                return false;
            return base.ContainsPoint(ref location, precise);
        }

        /// <inheritdoc />
        public override void OnMouseEnter(Float2 location)
        {
            // 2D GUI first
            base.OnMouseEnter(location);

            // Test 3D
            if (RayCast3D(ref location, out var hit, out var hitLocation))
            {
                hit.OnMouseEnter(hitLocation);
            }
        }

        /// <inheritdoc />
        public override void OnMouseMove(Float2 location)
        {
            // Calculate 3D mouse ray
            UICanvas.CalculateRay(ref location, out Ray ray);

            // Check all children collisions with mouse and fire events for them
            bool isFirst3DHandled = false;
            var layerMask = MainRenderTask.Instance?.ViewLayersMask ?? LayersMask.Default;
            for (int i = _children.Count - 1; i >= 0 && _children.Count > 0; i--)
            {
                var child = (CanvasRootControl)_children[i];
                if (child.Visible && child.Enabled && layerMask.HasLayer(child.Canvas.Layer))
                {
                    // Fire events
                    if (child.Is2D)
                    {
                        if (IntersectsChildContent(child, location, out var childLocation))
                        {
                            if (child.IsMouseOver)
                            {
                                // Move
                                child.OnMouseMove(childLocation);
                            }
                            else
                            {
                                // Enter
                                child.OnMouseEnter(childLocation);
                            }
                        }
                        else if (child.IsMouseOver)
                        {
                            // Leave
                            child.OnMouseLeave();
                        }
                    }
                    else
                    {
                        if (!isFirst3DHandled && child.Intersects3D(ref ray, out var childLocation))
                        {
                            isFirst3DHandled = true;

                            if (child.IsMouseOver)
                            {
                                // Move
                                child.OnMouseMove(childLocation);
                            }
                            else
                            {
                                // Enter
                                child.OnMouseEnter(childLocation);
                            }
                        }
                        else if (child.IsMouseOver)
                        {
                            // Leave
                            child.OnMouseLeave();
                        }
                    }
                }
            }
        }

        /// <inheritdoc />
        public override bool OnMouseWheel(Float2 location, float delta)
        {
            // 2D GUI first
            if (base.OnMouseWheel(location, delta))
                return true;

            // Test 3D
            if (RayCast3D(ref location, out var hit, out var hitLocation))
            {
                hit.OnMouseWheel(hitLocation, delta);
                return true;
            }

            return false;
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            // 2D GUI first
            if (base.OnMouseDown(location, button))
                return true;

            // Test 3D
            if (RayCast3D(ref location, out var hit, out var hitLocation))
            {
                hit.OnMouseDown(hitLocation, button);
                return true;
            }

            return false;
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            // 2D GUI first
            if (base.OnMouseUp(location, button))
                return true;

            // Test 3D
            if (RayCast3D(ref location, out var hit, out var hitLocation))
            {
                hit.OnMouseUp(hitLocation, button);
                return true;
            }

            return false;
        }

        /// <inheritdoc />
        public override bool OnMouseDoubleClick(Float2 location, MouseButton button)
        {
            // 2D GUI first
            if (base.OnMouseDoubleClick(location, button))
                return true;

            // Test 3D
            if (RayCast3D(ref location, out var hit, out var hitLocation))
            {
                hit.OnMouseDoubleClick(hitLocation, button);
                return true;
            }

            return false;
        }
    }
}
