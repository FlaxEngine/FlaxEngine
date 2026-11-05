// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEditor.Gizmo;
using FlaxEngine;

namespace FlaxEditor.Tools.Foliage
{
    /// <summary>
    /// The custom outline for drawing the selected foliage type.
    /// </summary>
    /// <seealso cref="FlaxEditor.Gizmo.SelectionOutline" />
    [HideInEditor]
    public class FoliageTypesOutline : SelectionOutline
    {
        /// <summary>
        /// The parent mode.
        /// </summary>
        public FoliageTypesGizmoMode GizmoMode;

        /// <inheritdoc />
        public override bool CanRender()
        {
            if (!HasDataReady)
                return false;

            var foliage = GizmoMode.SelectedFoliage;
            if (!foliage)
                return false;
            var typeIndex = GizmoMode.SelectedTypeIndex;
            if (typeIndex < 0 || typeIndex >= foliage.FoliageTypesCount)
                return false;
            return true;
        }

        /// <inheritdoc />
        public override void Render(GPUContext context, ref RenderContext renderContext, GPUTexture input, GPUTexture output)
        {
            base.Render(context, ref renderContext, input, output);

            // Restore debug option
            var foliage = GizmoMode.SelectedFoliage;
            if (foliage)
                foliage._drawFoliageType = -1;
        }

        /// <inheritdoc />
        protected override void DrawSelectionDepth(GPUContext context, SceneRenderTask task, GPUTexture customDepth)
        {
            var foliage = GizmoMode.SelectedFoliage;
            if (!foliage)
                return;
            var typeIndex = GizmoMode.SelectedTypeIndex;
            if (typeIndex < 0 || typeIndex >= foliage.FoliageTypesCount)
                return;

            // Draw instances of the given type
            foliage._drawFoliageType = typeIndex;
            _actors.Add(foliage);
            Renderer.DrawSceneDepth(context, task, customDepth, _actors);
        }
    }
}
