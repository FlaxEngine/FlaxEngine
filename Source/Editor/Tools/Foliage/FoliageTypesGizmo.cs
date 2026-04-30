// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEditor.Gizmo;

namespace FlaxEditor.Tools.Foliage
{
    /// <summary>
    /// Gizmo for editing foliage types.
    /// </summary>
    public sealed class FoliageTypesGizmo : GizmoBase
    {
        /// <summary>
        /// The parent mode.
        /// </summary>
        public readonly FoliageTypesGizmoMode GizmoMode;

        /// <summary>
        /// Initializes a new instance of the <see cref="EditFoliageGizmo"/> class.
        /// </summary>
        /// <param name="owner">The owner.</param>
        /// <param name="mode">The mode.</param>
        public FoliageTypesGizmo(IGizmoOwner owner, FoliageTypesGizmoMode mode)
        : base(owner)
        {
            GizmoMode = mode;
        }

        /// <inheritdoc />
        public override void Pick()
        {
            // Get mouse ray and try to hit foliage instance
            var foliage = GizmoMode.SelectedFoliage;
            if (!foliage)
                return;
            var ray = Owner.MouseRay;
            if (foliage.Intersects(ref ray, out _, out _, out var instanceIndex))
            {
                // Select hit instance type
                var instance = foliage.GetInstance(instanceIndex);
                GizmoMode.SelectedTypeIndex = instance.Type;
            }
        }
    }
}
