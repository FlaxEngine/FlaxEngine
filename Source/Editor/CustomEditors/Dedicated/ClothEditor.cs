// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEditor.Gizmo;
using FlaxEditor.Scripting;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEngine.Json;
using FlaxEngine.Tools;

namespace FlaxEditor.CustomEditors.Dedicated
{
    /// <summary>
    /// Custom editor for <see cref="Cloth"/>.
    /// </summary>
    /// <seealso cref="ActorEditor" />
    [CustomEditor(typeof(Cloth)), DefaultEditor]
    class ClothEditor : ActorEditor
    {
        private ClothPaintingGizmoMode _gizmoMode;
        private Viewport.Modes.EditorGizmoMode _prevMode;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            base.Initialize(layout);

            if (Values.Count != 1)
                return;

            // Add gizmo painting mode to the viewport
            var owner = Presenter.Owner;
            if (owner == null)
                return;
            var gizmoOwner = owner as IGizmoOwner ?? owner.PresenterViewport as IGizmoOwner;
            if (gizmoOwner == null)
                return;
            var gizmos = gizmoOwner.Gizmos;
            _gizmoMode = new ClothPaintingGizmoMode();

            var projectCache = Editor.Instance.ProjectCache;
            if (projectCache.TryGetCustomData("ClothGizmoPaintValue", out string cachedPaintValue))
                _gizmoMode.PaintValue = JsonSerializer.Deserialize<float>(cachedPaintValue);
            if (projectCache.TryGetCustomData("ClothGizmoContinuousPaint", out string cachedContinuousPaint))
                _gizmoMode.ContinuousPaint = JsonSerializer.Deserialize<bool>(cachedContinuousPaint);
            if (projectCache.TryGetCustomData("ClothGizmoBrushFalloff", out string cachedBrushFalloff))
                _gizmoMode.BrushFalloff = JsonSerializer.Deserialize<float>(cachedBrushFalloff);
            if (projectCache.TryGetCustomData("ClothGizmoBrushSize", out string cachedBrushSize))
                _gizmoMode.BrushSize = JsonSerializer.Deserialize<float>(cachedBrushSize);
            if (projectCache.TryGetCustomData("ClothGizmoBrushStrength", out string cachedBrushStrength))
                _gizmoMode.BrushStrength = JsonSerializer.Deserialize<float>(cachedBrushStrength);

            gizmos.AddMode(_gizmoMode);
            _prevMode = gizmos.ActiveMode;
            gizmos.ActiveMode = _gizmoMode;
            _gizmoMode.Gizmo.SetPaintCloth((Cloth)Values[0]);

            // Insert gizmo mode options to properties editing
            var paintGroup = layout.Group("Cloth Painting");
            var paintValue = new ReadOnlyValueContainer(new ScriptType(typeof(ClothPaintingGizmoMode)), _gizmoMode);
            paintGroup.Object(paintValue);
            {
                var grid = paintGroup.UniformGrid();
                var gridControl = grid.CustomControl;
                gridControl.ClipChildren = false;
                gridControl.Height = Button.DefaultHeight;
                gridControl.SlotsHorizontally = 2;
                gridControl.SlotsVertically = 1;
                grid.Button("Fill", "Fills the cloth particles with given paint value.").Button.Clicked += _gizmoMode.Gizmo.Fill;
                grid.Button("Reset", "Clears the cloth particles paint.").Button.Clicked += _gizmoMode.Gizmo.Reset;
            }
        }

        /// <inheritdoc />
        protected override void Deinitialize()
        {
            // Cleanup gizmos
            if (_gizmoMode != null)
            {
                var gizmos = _gizmoMode.Owner.Gizmos;
                if (gizmos.ActiveMode == _gizmoMode)
                    gizmos.ActiveMode = _prevMode;
                gizmos.RemoveMode(_gizmoMode);
                var projectCache = Editor.Instance.ProjectCache;
                projectCache.SetCustomData("ClothGizmoPaintValue", JsonSerializer.Serialize(_gizmoMode.PaintValue, typeof(float)));
                projectCache.SetCustomData("ClothGizmoContinuousPaint", JsonSerializer.Serialize(_gizmoMode.ContinuousPaint, typeof(bool)));
                projectCache.SetCustomData("ClothGizmoBrushFalloff", JsonSerializer.Serialize(_gizmoMode.BrushFalloff, typeof(float)));
                projectCache.SetCustomData("ClothGizmoBrushSize", JsonSerializer.Serialize(_gizmoMode.BrushSize, typeof(float)));
                projectCache.SetCustomData("ClothGizmoBrushStrength", JsonSerializer.Serialize(_gizmoMode.BrushStrength, typeof(float)));
                _gizmoMode.Dispose();
                _gizmoMode = null;
            }
            _prevMode = null;

            base.Deinitialize();
        }
    }
}
