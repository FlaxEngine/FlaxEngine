// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEditor.SceneGraph;
using FlaxEngine;

namespace FlaxEditor.Gizmo
{
    /// <summary>
    /// The most import gizmo tool used to move, rotate, scale and select scene objects in editor viewport.
    /// </summary>
    /// <seealso cref="TransformGizmoBase" />
    [HideInEditor]
    public class TransformGizmo : TransformGizmoBase
    {
        /// <summary>
        /// Applies scale to the selected objects pool.
        /// </summary>
        /// <param name="selection">The selected objects pool.</param>
        /// <param name="translationDelta">The translation delta.</param>
        /// <param name="rotationDelta">The rotation delta.</param>
        /// <param name="scaleDelta">The scale delta.</param>
        public delegate void ApplyTransformationDelegate(List<SceneGraphNode> selection, ref Vector3 translationDelta, ref Quaternion rotationDelta, ref Vector3 scaleDelta);

        private readonly List<SceneGraphNode> _selection = new List<SceneGraphNode>();
        private readonly List<SceneGraphNode> _selectionParents = new List<SceneGraphNode>();

        /// <summary>
        /// The event to apply objects transformation.
        /// </summary>
        public ApplyTransformationDelegate ApplyTransformation;

        /// <summary>
        /// The event to duplicate selected objects.
        /// </summary>
        public Action Duplicate;

        /// <summary>
        /// Gets the array of selected parent objects (as actors).
        /// </summary>
        public List<SceneGraphNode> SelectedParents => _selectionParents;

        /// <summary>
        /// Initializes a new instance of the <see cref="TransformGizmo" /> class.
        /// </summary>
        /// <param name="owner">The gizmos owner.</param>
        public TransformGizmo(IGizmoOwner owner)
        : base(owner)
        {
        }

        /// <inheritdoc />
        public override void Pick()
        {
            // Ensure player is not moving objects
            if (ActiveAxis != Axis.None)
                return;

            // Get mouse ray and try to hit any object
            var ray = Owner.MouseRay;
            var view = new Ray(Owner.ViewPosition, Owner.ViewDirection);
            bool selectColliders = (Owner.RenderTask.View.Flags & ViewFlags.PhysicsDebug) == ViewFlags.PhysicsDebug;
            SceneGraphNode.RayCastData.FlagTypes rayCastFlags = SceneGraphNode.RayCastData.FlagTypes.None;
            if (!selectColliders)
                rayCastFlags |= SceneGraphNode.RayCastData.FlagTypes.SkipColliders;
            var hit = Editor.Instance.Scene.Root.RayCast(ref ray, ref view, out _, rayCastFlags);

            // Update selection
            var sceneEditing = Editor.Instance.SceneEditing;
            if (hit != null)
            {
                // For child actor nodes (mesh, link or sth) we need to select it's owning actor node first or any other child node (but not a child actor)
                if (hit is ActorChildNode actorChildNode && !actorChildNode.CanBeSelectedDirectly)
                {
                    var parentNode = actorChildNode.ParentNode;
                    bool canChildBeSelected = sceneEditing.Selection.Contains(parentNode);
                    if (!canChildBeSelected)
                    {
                        for (int i = 0; i < parentNode.ChildNodes.Count; i++)
                        {
                            if (sceneEditing.Selection.Contains(parentNode.ChildNodes[i]))
                            {
                                canChildBeSelected = true;
                                break;
                            }
                        }
                    }

                    if (canChildBeSelected && sceneEditing.Selection.Count > 1)
                    {
                        // Don't select child node if multiple nodes are selected
                        canChildBeSelected = false;
                    }

                    if (!canChildBeSelected)
                    {
                        // Select parent
                        hit = parentNode;
                    }
                }

                bool addRemove = Owner.IsControlDown;
                bool isSelected = sceneEditing.Selection.Contains(hit);

                if (addRemove)
                {
                    if (isSelected)
                        sceneEditing.Deselect(hit);
                    else
                        sceneEditing.Select(hit, true);
                }
                else
                {
                    sceneEditing.Select(hit);
                }
            }
            else
            {
                sceneEditing.Deselect();
            }
        }

        /// <inheritdoc />
        public override void OnSelectionChanged(List<SceneGraphNode> newSelection)
        {
            // End current action
            EndTransforming();

            // Prepare collections
            _selection.Clear();
            _selectionParents.Clear();
            int count = newSelection.Count;
            if (_selection.Capacity < count)
            {
                _selection.Capacity = Mathf.NextPowerOfTwo(count);
                _selectionParents.Capacity = Mathf.NextPowerOfTwo(count);
            }

            // Cache selected objects
            _selection.AddRange(newSelection);

            // Build selected objects parents list.
            // Note: because selection may contain objects and their children we have to split them and get only parents.
            // Later during transformation we apply translation/scale/rotation only on them (children inherit transformations)
            SceneGraphTools.BuildNodesParents(_selection, _selectionParents);
        }

        /// <inheritdoc />
        protected override int SelectionCount => _selectionParents.Count;

        /// <inheritdoc />
        protected override Transform GetSelectedObject(int index)
        {
            return _selectionParents[index].Transform;
        }

        /// <inheritdoc />
        protected override void GetSelectedObjectsBounds(out BoundingBox bounds, out bool navigationDirty)
        {
            bounds = BoundingBox.Empty;
            navigationDirty = false;
            for (int i = 0; i < _selectionParents.Count; i++)
            {
                if (_selectionParents[i] is ActorNode actorNode)
                {
                    bounds = BoundingBox.Merge(bounds, actorNode.Actor.BoxWithChildren);
                    if (actorNode.AffectsNavigationWithChildren)
                    {
                        navigationDirty |= actorNode.Actor.HasStaticFlag(StaticFlags.Navigation);
                    }
                }
            }
        }

        /// <inheritdoc />
        protected override void OnApplyTransformation(ref Vector3 translationDelta, ref Quaternion rotationDelta, ref Vector3 scaleDelta)
        {
            base.OnApplyTransformation(ref translationDelta, ref rotationDelta, ref scaleDelta);

            ApplyTransformation(_selectionParents, ref translationDelta, ref rotationDelta, ref scaleDelta);
        }

        /// <inheritdoc />
        protected override void OnEndTransforming()
        {
            base.OnEndTransforming();

            // Record undo action
            Owner.Undo.AddAction(new TransformObjectsAction(SelectedParents, _startTransforms, ref _startBounds, _navigationDirty));
        }

        /// <inheritdoc />
        protected override void OnDuplicate()
        {
            base.OnDuplicate();

            Duplicate();
        }
    }
}
