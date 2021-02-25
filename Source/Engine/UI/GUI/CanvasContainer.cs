// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

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

        private bool IntersectsChildContent(CanvasRootControl child, ref Ray ray, out Vector2 childSpaceLocation)
        {
            // Inline bounds calculations (it will reuse world matrix)
            OrientedBoundingBox bounds = new OrientedBoundingBox
            {
                Extents = new Vector3(child.Size * 0.5f, Mathf.Epsilon)
            };

            child.Canvas.GetWorldMatrix(out var world);
            Matrix.Translation(bounds.Extents.X, bounds.Extents.Y, 0, out var offset);
            Matrix.Multiply(ref offset, ref world, out bounds.Transformation);

            // Hit test
            if (bounds.Intersects(ref ray, out Vector3 hitPoint))
            {
                // Transform world-space hit point to canvas local-space
                world.Invert();
                Vector3.Transform(ref hitPoint, ref world, out Vector3 localHitPoint);

                childSpaceLocation = new Vector2(localHitPoint);
                return child.ContainsPoint(ref childSpaceLocation);
            }

            childSpaceLocation = Vector2.Zero;
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
            for (int i = 0; i < _children.Count; i++)
            {
                var child = (CanvasRootControl)_children[i];

                if (child.Visible && child.Is2D)
                {
                    child.Draw();
                }
            }
        }

        /// <inheritdoc />
        public override bool IntersectsChildContent(Control child, Vector2 location, out Vector2 childSpaceLocation)
        {
            childSpaceLocation = Vector2.Zero;
            return ((CanvasRootControl)child).Is2D && base.IntersectsChildContent(child, location, out childSpaceLocation);
        }

        /// <inheritdoc />
        public override void OnMouseEnter(Vector2 location)
        {
            // 2D GUI first
            base.OnMouseEnter(location);

            // Calculate 3D mouse ray
            UICanvas.CalculateRay(ref location, out Ray ray);

            // Test 3D
            for (int i = _children.Count - 1; i >= 0 && _children.Count > 0; i--)
            {
                var child = (CanvasRootControl)_children[i];
                if (child.Visible && child.Enabled && child.Is3D)
                {
                    if (IntersectsChildContent(child, ref ray, out var childLocation))
                    {
                        child.OnMouseEnter(childLocation);
                        return;
                    }
                }
            }
        }

        /// <inheritdoc />
        public override void OnMouseMove(Vector2 location)
        {
            // Calculate 3D mouse ray
            UICanvas.CalculateRay(ref location, out Ray ray);

            // Check all children collisions with mouse and fire events for them
            bool isFirst3DHandled = false;
            for (int i = _children.Count - 1; i >= 0 && _children.Count > 0; i--)
            {
                var child = (CanvasRootControl)_children[i];
                if (child.Visible && child.Enabled)
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
                        if (!isFirst3DHandled && IntersectsChildContent(child, ref ray, out var childLocation))
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
        public override bool OnMouseWheel(Vector2 location, float delta)
        {
            // 2D GUI first
            if (base.OnMouseWheel(location, delta))
                return true;

            // Calculate 3D mouse ray
            UICanvas.CalculateRay(ref location, out Ray ray);

            // Test 3D
            for (int i = _children.Count - 1; i >= 0 && _children.Count > 0; i--)
            {
                var child = (CanvasRootControl)_children[i];
                if (child.Visible && child.Enabled && child.Is3D)
                {
                    if (IntersectsChildContent(child, ref ray, out var childLocation))
                    {
                        child.OnMouseWheel(childLocation, delta);
                        return true;
                    }
                }
            }

            return false;
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Vector2 location, MouseButton button)
        {
            // 2D GUI first
            if (base.OnMouseDown(location, button))
                return true;

            // Calculate 3D mouse ray
            UICanvas.CalculateRay(ref location, out Ray ray);

            // Test 3D
            for (int i = _children.Count - 1; i >= 0 && _children.Count > 0; i--)
            {
                var child = (CanvasRootControl)_children[i];
                if (child.Visible && child.Enabled && child.Is3D)
                {
                    if (IntersectsChildContent(child, ref ray, out var childLocation))
                    {
                        child.OnMouseDown(childLocation, button);
                        return true;
                    }
                }
            }

            return false;
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Vector2 location, MouseButton button)
        {
            // 2D GUI first
            if (base.OnMouseUp(location, button))
                return true;

            // Calculate 3D mouse ray
            UICanvas.CalculateRay(ref location, out Ray ray);

            // Test 3D
            for (int i = _children.Count - 1; i >= 0 && _children.Count > 0; i--)
            {
                var child = (CanvasRootControl)_children[i];
                if (child.Visible && child.Enabled && child.Is3D)
                {
                    if (IntersectsChildContent(child, ref ray, out var childLocation))
                    {
                        child.OnMouseUp(childLocation, button);
                        return true;
                    }
                }
            }

            return false;
        }

        /// <inheritdoc />
        public override bool OnMouseDoubleClick(Vector2 location, MouseButton button)
        {
            // 2D GUI first
            if (base.OnMouseDoubleClick(location, button))
                return true;

            // Calculate 3D mouse ray
            UICanvas.CalculateRay(ref location, out Ray ray);

            // Test 3D
            for (int i = _children.Count - 1; i >= 0 && _children.Count > 0; i--)
            {
                var child = (CanvasRootControl)_children[i];
                if (child.Visible && child.Enabled && child.Is3D)
                {
                    if (IntersectsChildContent(child, ref ray, out var childLocation))
                    {
                        child.OnMouseDoubleClick(childLocation, button);
                        return true;
                    }
                }
            }

            return false;
        }
    }
}
