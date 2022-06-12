// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

using System;
using FlaxEngine;

namespace FlaxEditor.Gizmo
{
    /// <summary>
    /// Draws a grid to feel better world origin position and the world units.
    /// </summary>
    /// <seealso cref="FlaxEditor.Gizmo.GizmoBase" />
    [HideInEditor]
    public class GridGizmo : GizmoBase
    {
        private bool _enabled = true;

        /// <summary>
        /// Gets or sets a value indicating whether this <see cref="GridGizmo"/> is enabled.
        /// </summary>
        public bool Enabled
        {
            get => _enabled;
            set
            {
                if (_enabled != value)
                {
                    _enabled = value;
                    EnabledChanged?.Invoke(this);
                }
            }
        }

        /// <summary>
        /// Occurs when enabled state gets changed.
        /// </summary>
        public event Action<GridGizmo> EnabledChanged;

        /// <summary>
        /// Initializes a new instance of the <see cref="GridGizmo"/> class.
        /// </summary>
        /// <param name="owner">The gizmos owner.</param>
        public GridGizmo(IGizmoOwner owner)
        : base(owner)
        {
        }

        /// <inheritdoc />
        public override void Draw(ref RenderContext renderContext)
        {
            if (!Enabled)
                return;

            var viewPos = Owner.ViewPosition;
            var plane = new Plane(Vector3.Zero, Vector3.UnitY);
            var dst = CollisionsHelper.DistancePlanePoint(ref plane, ref viewPos);

            float space, size;
            if (dst <= 500.0f)
            {
                space = 50;
                size = 8000;
            }
            else if (dst <= 2000.0f)
            {
                space = 100;
                size = 8000;
            }
            else
            {
                space = 1000;
                size = 100000;
            }

            Color color = Color.Gray * 0.7f;
            int count = (int)(size / space);

            Vector3 start = new Vector3(0, 0, size * -0.5f);
            Vector3 end = new Vector3(0, 0, size * 0.5f);

            for (int i = 0; i <= count; i++)
            {
                start.X = end.X = i * space + start.Z;
                DebugDraw.DrawLine(start, end, color);
            }

            start = new Vector3(size * -0.5f, 0, 0);
            end = new Vector3(size * 0.5f, 0, 0);

            for (int i = 0; i <= count; i++)
            {
                start.Z = end.Z = i * space + start.X;
                DebugDraw.DrawLine(start, end, color);
            }
        }
    }
}
