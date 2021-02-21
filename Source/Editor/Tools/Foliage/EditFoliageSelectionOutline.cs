// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using FlaxEditor.Gizmo;
using FlaxEngine;

namespace FlaxEditor.Tools.Foliage
{
    /// <summary>
    /// The custom outline for drawing the selected foliage instances outlines.
    /// </summary>
    /// <seealso cref="FlaxEditor.Gizmo.SelectionOutline" />
    [HideInEditor]
    public class EditFoliageSelectionOutline : SelectionOutline
    {
        private StaticModel _staticModel;

        /// <summary>
        /// The parent mode.
        /// </summary>
        public EditFoliageGizmoMode GizmoMode;

        /// <inheritdoc />
        public override bool CanRender
        {
            get
            {
                if (!HasDataReady)
                    return false;

                var foliage = GizmoMode.SelectedFoliage;
                if (!foliage)
                    return false;
                var instanceIndex = GizmoMode.SelectedInstanceIndex;
                if (instanceIndex < 0 || instanceIndex >= foliage.InstancesCount)
                    return false;
                return true;
            }
        }

        /// <inheritdoc />
        protected override void DrawSelectionDepth(GPUContext context, SceneRenderTask task, GPUTexture customDepth)
        {
            var foliage = GizmoMode.SelectedFoliage;
            if (!foliage)
                return;
            var instanceIndex = GizmoMode.SelectedInstanceIndex;
            if (instanceIndex < 0 || instanceIndex >= foliage.InstancesCount)
                return;

            // Draw single instance
            var instance = foliage.GetInstance(instanceIndex);
            var model = foliage.GetFoliageType(instance.Type).Model;
            if (model)
            {
                Transform instanceWorld = foliage.Transform.LocalToWorld(instance.Transform);

                if (!_staticModel)
                {
                    _staticModel = new StaticModel
                    {
                        StaticFlags = StaticFlags.None
                    };
                }

                _staticModel.Model = model;
                _staticModel.Transform = instanceWorld;
                _actors.Add(_staticModel);

                Renderer.DrawSceneDepth(context, task, customDepth, _actors);
            }
        }
    }
}
