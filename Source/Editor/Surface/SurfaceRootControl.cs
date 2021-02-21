// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Surface
{
    /// <summary>
    /// The surface root control used to navigate around the view (scale and move it).
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.ContainerControl" />
    [HideInEditor]
    public class SurfaceRootControl : ContainerControl
    {
        /// <inheritdoc />
        public SurfaceRootControl()
        {
            AutoFocus = false;
            ClipChildren = false;
            Pivot = Vector2.Zero;
        }

        /// <inheritdoc />
        public override bool IntersectsContent(ref Vector2 locationParent, out Vector2 location)
        {
            location = PointFromParent(ref locationParent);
            return true;
        }

        /// <summary>
        /// Draws the comments. Render them before other controls to prevent foreground override.
        /// </summary>
        public virtual void DrawComments()
        {
            Render2D.PushTransform(ref _cachedTransform);

            // Push clipping mask
            if (ClipChildren)
            {
                GetDesireClientArea(out Rectangle clientArea);
                Render2D.PushClip(ref clientArea);
            }

            // Draw all visible child comments
            for (int i = 0; i < _children.Count; i++)
            {
                var child = _children[i];

                if (child is SurfaceComment && child.Visible)
                {
                    Render2D.PushTransform(ref child._cachedTransform);
                    child.Draw();
                    Render2D.PopTransform();
                }
            }

            // Pop clipping mask
            if (ClipChildren)
            {
                Render2D.PopClip();
            }

            Render2D.PopTransform();
        }

        /// <inheritdoc />
        protected override void DrawChildren()
        {
            // Skip comments (render them to as background manually by Visject Surface)
            Render2D.PeekClip(out var globalClipping);
            Render2D.PeekTransform(out var globalTransform);
            for (int i = 0; i < _children.Count; i++)
            {
                var child = _children[i];
                if (!(child is SurfaceComment) && child.Visible)
                {
                    Matrix3x3.Multiply(ref child._cachedTransform, ref globalTransform, out var globalChildTransform);
                    var childGlobalRect = new Rectangle(globalChildTransform.M31, globalChildTransform.M32, child.Width * globalChildTransform.M11, child.Height * globalChildTransform.M22);
                    if (globalClipping.Intersects(ref childGlobalRect))
                    {
                        Render2D.PushTransform(ref child._cachedTransform);
                        child.Draw();
                        Render2D.PopTransform();
                    }
                }
            }
        }
    }
}
