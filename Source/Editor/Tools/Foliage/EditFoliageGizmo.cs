// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.Gizmo;
using FlaxEditor.SceneGraph;
using FlaxEditor.SceneGraph.Actors;
using FlaxEditor.Tools.Foliage.Undo;
using FlaxEngine;

namespace FlaxEditor.Tools.Foliage
{
    /// <summary>
    /// Gizmo for editing foliage instances. Managed by the <see cref="EditFoliageGizmoMode"/>.
    /// </summary>
    /// <seealso cref="FlaxEditor.Gizmo.TransformGizmoBase" />
    public sealed class EditFoliageGizmo : TransformGizmoBase
    {
        private MaterialBase _highlightMaterial;
        private bool _needSync = true;
        private EditInstanceAction _action;

        /// <summary>
        /// The parent mode.
        /// </summary>
        public readonly EditFoliageGizmoMode GizmoMode;

        /// <summary>
        /// Initializes a new instance of the <see cref="EditFoliageGizmo"/> class.
        /// </summary>
        /// <param name="owner">The owner.</param>
        /// <param name="mode">The mode.</param>
        public EditFoliageGizmo(IGizmoOwner owner, EditFoliageGizmoMode mode)
        : base(owner)
        {
            GizmoMode = mode;
        }

        /// <inheritdoc />
        public override BoundingSphere FocusBounds
        {
            get
            {
                var foliage = GizmoMode.SelectedFoliage;
                if (foliage)
                {
                    var instanceIndex = GizmoMode.SelectedInstanceIndex;
                    if (instanceIndex >= 0 && instanceIndex < foliage.InstancesCount)
                    {
                        var instance = foliage.GetInstance(instanceIndex);
                        return instance.Bounds;
                    }
                }
                return base.FocusBounds;
            }
        }

        /// <inheritdoc />
        protected override int SelectionCount
        {
            get
            {
                var foliage = GizmoMode.SelectedFoliage;
                if (!foliage)
                    return 0;
                var instanceIndex = GizmoMode.SelectedInstanceIndex;
                if (instanceIndex < 0 || instanceIndex >= foliage.InstancesCount)
                    return 0;
                return 1;
            }
        }

        /// <inheritdoc />
        protected override SceneGraphNode GetSelectedObject(int index)
        {
            var foliage = GizmoMode.SelectedFoliage;
            if (!foliage)
                throw new InvalidOperationException("No foliage selected.");
            var instanceIndex = GizmoMode.SelectedInstanceIndex;
            if (instanceIndex < 0 || instanceIndex >= foliage.InstancesCount)
                throw new InvalidOperationException("No foliage instance selected.");
            return new FoliageInstanceNode(foliage, instanceIndex);
        }

        /// <inheritdoc />
        protected override Transform GetSelectedTransform(int index)
        {
            var foliage = GizmoMode.SelectedFoliage;
            if (!foliage)
                throw new InvalidOperationException("No foliage selected.");
            var instanceIndex = GizmoMode.SelectedInstanceIndex;
            if (instanceIndex < 0 || instanceIndex >= foliage.InstancesCount)
                throw new InvalidOperationException("No foliage instance selected.");
            var instance = foliage.GetInstance(instanceIndex);
            return foliage.Transform.LocalToWorld(instance.Transform);
        }

        /// <inheritdoc />
        protected override void GetSelectedObjectsBounds(out BoundingBox bounds, out bool navigationDirty)
        {
            bounds = BoundingBox.Empty;
            navigationDirty = false;
            var foliage = GizmoMode.SelectedFoliage;
            var instanceIndex = GizmoMode.SelectedInstanceIndex;
            if (foliage && instanceIndex >= 0 && instanceIndex < foliage.InstancesCount)
            {
                var instance = foliage.GetInstance(instanceIndex);
                BoundingBox.FromSphere(ref instance.Bounds, out bounds);
            }
        }

        /// <inheritdoc />
        protected override bool IsSelected(SceneGraphNode obj)
        {
            return false;
        }

        /// <inheritdoc />
        protected override void OnStartTransforming()
        {
            base.OnStartTransforming();

            // Start undo
            var foliage = GizmoMode.SelectedFoliage;
            if (!foliage)
                throw new InvalidOperationException("No foliage selected.");
            _action = new EditInstanceAction(foliage, GizmoMode.SelectedInstanceIndex);
        }

        /// <inheritdoc />
        protected override void OnApplyTransformation(ref Vector3 translationDelta, ref Quaternion rotationDelta, ref Vector3 scaleDelta)
        {
            base.OnApplyTransformation(ref translationDelta, ref rotationDelta, ref scaleDelta);

            bool applyRotation = !rotationDelta.IsIdentity;
            bool useObjCenter = ActivePivot == PivotType.ObjectCenter;
            Vector3 gizmoPosition = Position;

            // Get instance transform
            var foliage = GizmoMode.SelectedFoliage;
            if (!foliage)
                throw new InvalidOperationException("No foliage selected.");
            var instanceIndex = GizmoMode.SelectedInstanceIndex;
            if (instanceIndex < 0 || instanceIndex >= foliage.InstancesCount)
                throw new InvalidOperationException("No foliage instance selected.");
            var instance = foliage.GetInstance(instanceIndex);
            var trans = foliage.Transform.LocalToWorld(instance.Transform);

            // Apply rotation
            if (applyRotation)
            {
                Vector3 pivotOffset = trans.Translation - gizmoPosition;
                if (useObjCenter || pivotOffset.IsZero)
                {
                    trans.Orientation *= Quaternion.Invert(trans.Orientation) * rotationDelta * trans.Orientation;
                }
                else
                {
                    Matrix.RotationQuaternion(ref trans.Orientation, out var transWorld);
                    Matrix.RotationQuaternion(ref rotationDelta, out var deltaWorld);
                    Matrix world = transWorld * Matrix.Translation(pivotOffset) * deltaWorld * Matrix.Translation(-pivotOffset);
                    trans.SetRotation(ref world);
                    trans.Translation += world.TranslationVector;
                }
            }

            // Apply scale
            const float scaleLimit = 99_999_999.0f;
            trans.Scale = Float3.Clamp(trans.Scale + scaleDelta, new Float3(-scaleLimit), new Float3(scaleLimit));

            // Apply translation
            trans.Translation += translationDelta;

            // Transform foliage instance
            instance.Transform = foliage.Transform.WorldToLocal(trans);
            foliage.SetInstanceTransform(instanceIndex, ref instance.Transform);
            foliage.RebuildClusters();
        }

        /// <inheritdoc />
        protected override void OnEndTransforming()
        {
            base.OnEndTransforming();

            // End undo
            _action.RecordEnd();
            Owner.Undo?.AddAction(_action);
            _action = null;
        }

        /// <inheritdoc />
        protected override void OnDuplicate()
        {
            base.OnDuplicate();

            // Get selected instance
            var foliage = GizmoMode.SelectedFoliage;
            if (!foliage)
                throw new InvalidOperationException("No foliage selected.");
            var instanceIndex = GizmoMode.SelectedInstanceIndex;
            if (instanceIndex < 0 || instanceIndex >= foliage.InstancesCount)
                throw new InvalidOperationException("No foliage instance selected.");
            var instance = foliage.GetInstance(instanceIndex);
            var action = new EditFoliageAction(foliage);

            // Duplicate instance and select it
            var newIndex = foliage.InstancesCount;
            foliage.AddInstance(ref instance);
            action.RecordEnd();
            Owner.Undo?.AddAction(new MultiUndoAction(action, new EditSelectedInstanceIndexAction(GizmoMode.SelectedInstanceIndex, newIndex)));
            GizmoMode.SelectedInstanceIndex = newIndex;
        }

        /// <inheritdoc />
        public override void Draw(ref RenderContext renderContext)
        {
            base.Draw(ref renderContext);

            if (!IsActive || !_highlightMaterial)
                return;

            var foliage = GizmoMode.SelectedFoliage;
            if (!foliage)
                return;
            var instanceIndex = GizmoMode.SelectedInstanceIndex;
            if (instanceIndex < 0 || instanceIndex >= foliage.InstancesCount)
                return;

            var instance = foliage.GetInstance(instanceIndex);
            var model = foliage.GetFoliageType(instance.Type).Model;
            if (model)
            {
                var transform = foliage.Transform.LocalToWorld(instance.Transform);
                renderContext.View.GetWorldMatrix(ref transform, out var world);
                model.Draw(ref renderContext, _highlightMaterial, ref world, StaticFlags.None, false);
            }
        }

        /// <inheritdoc />
        public override void Pick()
        {
            // Ensure player is not moving objects
            if (ActiveAxis != Axis.None)
                return;

            // Get mouse ray and try to hit foliage instance
            var foliage = GizmoMode.SelectedFoliage;
            if (!foliage)
                return;
            var ray = Owner.MouseRay;
            foliage.Intersects(ref ray, out _, out _, out var instanceIndex);

            // Change the selection (with undo)
            if (GizmoMode.SelectedInstanceIndex == instanceIndex)
                return;
            var action = new EditSelectedInstanceIndexAction(GizmoMode.SelectedInstanceIndex, instanceIndex);
            action.Do();
            Owner.Undo?.AddAction(action);
        }

        /// <inheritdoc />
        public override void SnapToGround()
        {
            if (Physics.RayCast(Position, Vector3.Down, out var hit, float.MaxValue, uint.MaxValue, false))
            {
                // Snap
                var translationDelta = hit.Point - Position;
                var rotationDelta = Quaternion.Identity;
                var scaleDelta = Vector3.Zero;
                if (translationDelta.IsZero)
                    return;
                StartTransforming();
                OnApplyTransformation(ref translationDelta, ref rotationDelta, ref scaleDelta);
                EndTransforming();
            }
        }

        /// <inheritdoc />
        public override void OnActivated()
        {
            base.OnActivated();

            _highlightMaterial = EditorAssets.Cache.HighlightMaterialInstance;

            if (_needSync)
            {
                _needSync = false;

                // Sync with main transform gizmo
                var mainTransformGizmo = Editor.Instance.MainTransformGizmo;
                ActiveMode = mainTransformGizmo.ActiveMode;
                ActiveTransformSpace = mainTransformGizmo.ActiveTransformSpace;
                mainTransformGizmo.ModeChanged += () => ActiveMode = mainTransformGizmo.ActiveMode;
                mainTransformGizmo.TransformSpaceChanged += () => ActiveTransformSpace = mainTransformGizmo.ActiveTransformSpace;
            }
        }

        /// <inheritdoc />
        public override void OnDeactivated()
        {
            _highlightMaterial = null;

            base.OnDeactivated();
        }
    }
}
