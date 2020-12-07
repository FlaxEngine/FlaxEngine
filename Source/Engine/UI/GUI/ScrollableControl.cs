// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

namespace FlaxEngine.GUI
{
    /// <summary>
    /// Base class for container controls that can offset controls in a view (eg. scroll panels).
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.ContainerControl" />
    [HideInEditor]
    public class ScrollableControl : ContainerControl
    {
        /// <summary>
        /// The view offset. Useful to offset contents of the container (used by the scrollbars and drop panels).
        /// </summary>
        protected Vector2 _viewOffset;

        /// <summary>
        /// Gets current view offset for all the controls (used by the scroll bars).
        /// </summary>
        [HideInEditor, NoSerialize]
        public Vector2 ViewOffset
        {
            get => _viewOffset;
            set => SetViewOffset(ref value);
        }

        /// <summary>
        /// Sets the view offset.
        /// </summary>
        /// <param name="value">The value.</param>
        protected virtual void SetViewOffset(ref Vector2 value)
        {
            _viewOffset = value;
            OnViewOffsetChanged();
        }

        /// <summary>
        /// Called when view offset gets changed.
        /// </summary>
        protected virtual void OnViewOffsetChanged()
        {
        }

        /// <inheritdoc />
        protected override void DrawChildren()
        {
            // Draw all visible child controls
            bool hasViewOffset = !_viewOffset.IsZero;
            for (int i = 0; i < _children.Count; i++)
            {
                var child = _children[i];

                if (child.Visible)
                {
                    Matrix3x3 transform = child._cachedTransform;
                    if (hasViewOffset && child.IsScrollable)
                    {
                        transform.M31 += _viewOffset.X;
                        transform.M32 += _viewOffset.Y;
                    }

                    Render2D.PushTransform(ref transform);
                    child.Draw();
                    Render2D.PopTransform();
                }
            }
        }

        /// <inheritdoc />
        public override bool IntersectsChildContent(Control child, Vector2 location, out Vector2 childSpaceLocation)
        {
            // Apply offset on scrollable controls
            if (child.IsScrollable)
                location -= _viewOffset;

            return child.IntersectsContent(ref location, out childSpaceLocation);
        }

        /// <inheritdoc />
        public override bool IntersectsContent(ref Vector2 locationParent, out Vector2 location)
        {
            // Little workaround to prevent applying offset when performing intersection test with this scrollable control.
            // Note that overriden PointFromParent applies view offset.
            location = base.PointFromParent(ref locationParent);
            return ContainsPoint(ref location);
        }

        /// <inheritdoc />
        public override Vector2 PointToParent(ref Vector2 location)
        {
            return base.PointToParent(ref location) + _viewOffset;
        }

        /// <inheritdoc />
        public override Vector2 PointFromParent(ref Vector2 location)
        {
            return base.PointFromParent(ref location) - _viewOffset;
        }
    }
}
