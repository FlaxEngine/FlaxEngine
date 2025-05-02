// Copyright (c) Wojciech Figat. All rights reserved.

#if USE_LARGE_WORLDS
using Real = System.Double;
#else
using Real = System.Single;
#endif

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
        /// Gets the array of selected objects.
        /// </summary>
        public List<SceneGraphNode> Selection => _selection;

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

        /// <summary>
        /// Helper function, recursively finds the Prefab Root of node or null.
        /// </summary>
        /// <param name="node">The node from which to start.</param>
        /// <returns>The prefab root or null.</returns>
        public ActorNode GetPrefabRootInParent(ActorNode node)
        {
            if (!node.HasPrefabLink)
                return null;
            if (node.Actor.IsPrefabRoot)
                return node;
            if (node.ParentNode is ActorNode parAct)
                return GetPrefabRootInParent(parAct);
            return null;
        }

        /// <summary>
        /// Recursively walks up from the node up to ceiling node(inclusive) or selection(exclusive).
        /// </summary>
        /// <param name="node">The node from which to start</param>
        /// <param name="ceiling">The ceiling(inclusive)</param>
        /// <returns>The node to select.</returns>
        public ActorNode WalkUpAndFindActorNodeBeforeSelection(ActorNode node, ActorNode ceiling)
        {
            if (node == ceiling || _selection.Contains(node))
                return node;
            if (node.ParentNode is ActorNode parentNode)
            {
                if (_selection.Contains(node.ParentNode))
                    return node;
                return WalkUpAndFindActorNodeBeforeSelection(parentNode, ceiling);
            }
            return node;
        }

        /// <inheritdoc />
        public override void SnapToGround()
        {
            if (Owner.SceneGraphRoot == null)
                return;
            var ray = new Ray(Position, Vector3.Down);
            while (true)
            {
                var view = new Ray(Owner.ViewPosition, Owner.ViewDirection);
                var rayCastFlags = SceneGraphNode.RayCastData.FlagTypes.SkipEditorPrimitives | SceneGraphNode.RayCastData.FlagTypes.SkipTriggers;
                var hit = Owner.SceneGraphRoot.RayCast(ref ray, ref view, out var distance, out _, rayCastFlags);
                if (hit != null)
                {
                    // Skip snapping selection to itself
                    bool isSelected = false;
                    for (var e = hit; e != null && !isSelected; e = e.ParentNode)
                        isSelected |= IsSelected(e);
                    if (isSelected)
                    {
                        GetSelectedObjectsBounds(out var selectionBounds, out _);
                        var offset = Mathf.Max(selectionBounds.Size.Y * 0.5f, 1.0f);
                        ray.Position = ray.GetPoint(offset);
                        continue;
                    }

                    // Include objects bounds into target snap location
                    var editorBounds = BoundingBox.Empty;
                    Real bottomToCenter = 100000.0f;
                    for (int i = 0; i < _selectionParents.Count; i++)
                    {
                        if (_selectionParents[i] is ActorNode actorNode)
                        {
                            var b = actorNode.Actor.EditorBoxChildren;
                            BoundingBox.Merge(ref editorBounds, ref b, out editorBounds);
                            bottomToCenter = Mathf.Min(bottomToCenter, actorNode.Actor.Position.Y - editorBounds.Minimum.Y);
                        }
                    }
                    var newPosition = ray.GetPoint(distance) + new Vector3(0, bottomToCenter, 0);

                    // Snap
                    var translationDelta = newPosition - Position;
                    var rotationDelta = Quaternion.Identity;
                    var scaleDelta = Vector3.Zero;
                    if (translationDelta.IsZero)
                        break;
                    StartTransforming();
                    OnApplyTransformation(ref translationDelta, ref rotationDelta, ref scaleDelta);
                    EndTransforming();
                }
                break;
            }
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
            var renderView = Owner.RenderTask.View;
            bool selectColliders = (renderView.Flags & ViewFlags.PhysicsDebug) == ViewFlags.PhysicsDebug || renderView.Mode == ViewMode.PhysicsColliders;
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

                // Select prefab root and then go down until you find the actual item in which case select the prefab root again
                if (hit is ActorNode actorNode)
                {
                    ActorNode prefabRoot = GetPrefabRootInParent(actorNode);
                    if (prefabRoot != null && actorNode != prefabRoot)
                    {
                        bool isPrefabInSelection = false;
                        foreach (var e in sceneEditing.Selection)
                        {
                            if (e is ActorNode ae && GetPrefabRootInParent(ae) == prefabRoot)
                            {
                                isPrefabInSelection = true;
                                break;
                            }
                        }

                        // Skip selecting prefab root if we already had object from that prefab selected
                        if (!isPrefabInSelection)
                        {
                            hit = WalkUpAndFindActorNodeBeforeSelection(actorNode, prefabRoot);
                        }
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

            base.OnSelectionChanged(newSelection);
        }

        /// <inheritdoc />
        protected override int SelectionCount => _selectionParents.Count;

        /// <inheritdoc />
        protected override SceneGraphNode GetSelectedObject(int index)
        {
            return _selectionParents[index];
        }

        /// <inheritdoc />
        protected override Transform GetSelectedTransform(int index)
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
                    navigationDirty |= actorNode.AffectsNavigationWithChildren;
                }
            }
        }

        /// <inheritdoc />
        protected override bool IsSelected(SceneGraphNode obj)
        {
            return _selection.Contains(obj);
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
