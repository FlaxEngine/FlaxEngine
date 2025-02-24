// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using FlaxEditor.Actions;
using FlaxEditor.Content;
using FlaxEditor.GUI;
using FlaxEditor.GUI.Drag;
using FlaxEditor.GUI.Tree;
using FlaxEditor.Options;
using FlaxEditor.Scripting;
using FlaxEditor.Utilities;
using FlaxEditor.Windows;
using FlaxEditor.Windows.Assets;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEngine.Utilities;

namespace FlaxEditor.SceneGraph.GUI
{
    /// <summary>
    /// Tree node GUI control used as a proxy object for actors hierarchy.
    /// </summary>
    /// <seealso cref="TreeNode" />
    public class ActorTreeNode : TreeNode
    {
        private int _orderInParent;
        private DragActors _dragActors;
        private DragScripts _dragScripts;
        private DragAssets _dragAssets;
        private DragActorType _dragActorType;
        private DragControlType _dragControlType;
        private DragScriptItems _dragScriptItems;
        private DragHandlers _dragHandlers;
        private List<Rectangle> _highlights;
        private bool _hasSearchFilter;

        /// <summary>
        /// The actor node that owns this node.
        /// </summary>
        protected ActorNode _actorNode;

        /// <summary>
        /// Gets the actor.
        /// </summary>
        public Actor Actor => _actorNode.Actor;

        /// <summary>
        /// Gets the actor node.
        /// </summary>
        public ActorNode ActorNode => _actorNode;

        /// <summary>
        /// Initializes a new instance of the <see cref="ActorTreeNode"/> class.
        /// </summary>
        public ActorTreeNode()
        : base(true)
        {
            ChildrenIndent = 16.0f;
        }

        internal virtual void LinkNode(ActorNode node)
        {
            _actorNode = node;
            var actor = node.Actor;
            if (actor != null)
            {
                _orderInParent = actor.OrderInParent;
                Visible = (actor.HideFlags & HideFlags.HideInHierarchy) == 0;

                // Pick the correct id when inside a prefab window.
                var id = actor.HasPrefabLink && !actor.HasScene ? actor.PrefabObjectID : actor.ID;
                if (Editor.Instance.ProjectCache.IsExpandedActor(ref id))
                {
                    Expand(true);
                }
            }
            else
            {
                _orderInParent = 0;
            }

            UpdateText();
        }

        internal void OnParentChanged(Actor actor, ActorNode parentNode)
        {
            // Update cached value
            _orderInParent = actor.OrderInParent;

            // Update UI (special case if actor is spawned and added to existing scene tree)
            var parentTreeNode = parentNode?.TreeNode;
            if (parentTreeNode != null && !parentTreeNode.IsLayoutLocked)
            {
                parentTreeNode.IsLayoutLocked = true;
                Parent = parentTreeNode;
                IndexInParent = _orderInParent;
                parentTreeNode.IsLayoutLocked = false;

                // Skip UI update if node won't be in a view
                if (parentTreeNode.IsCollapsedInHierarchy)
                {
                    UnlockChildrenRecursive();
                }
                else
                {
                    // Try to perform layout at the level where it makes it the most performant (the least computations)
                    var tree = parentTreeNode.ParentTree;
                    if (tree != null)
                    {
                        if (tree.Parent is Panel treeParent)
                            treeParent.PerformLayout();
                        else
                            tree.PerformLayout();
                    }
                    else
                    {
                        parentTreeNode.PerformLayout();
                    }
                }
            }
            else
            {
                Parent = parentTreeNode;
            }
        }

        internal void OnOrderInParentChanged()
        {
            // Use cached value to check if we need to update UI layout (and update siblings order at once)
            if (Parent is ActorTreeNode parent)
            {
                var anyChanged = false;
                var children = parent.Children;
                for (int i = 0; i < children.Count; i++)
                {
                    if (children[i] is ActorTreeNode child && child.Actor)
                    {
                        var orderInParent = child.Actor.OrderInParent;
                        anyChanged |= child._orderInParent != orderInParent;
                        if (anyChanged)
                            child._orderInParent = orderInParent;
                    }
                }
                if (anyChanged)
                    parent.SortChildren();
            }
            else if (Actor)
            {
                _orderInParent = Actor.OrderInParent;
            }
        }

        /// <summary>
        /// Updates the tree node text.
        /// </summary>
        public virtual void UpdateText()
        {
            Text = _actorNode.Name;
        }

        /// <summary>
        /// Updates the query search filter.
        /// </summary>
        /// <param name="filterText">The filter text.</param>
        public void UpdateFilter(string filterText)
        {
            // SKip hidden actors
            var actor = Actor;
            if (actor != null && (actor.HideFlags & HideFlags.HideInHierarchy) != 0)
                return;

            bool noFilter = string.IsNullOrWhiteSpace(filterText);
            _hasSearchFilter = !noFilter;

            // Update itself
            bool isThisVisible;
            if (noFilter)
            {
                // Clear filter
                _highlights?.Clear();
                isThisVisible = true;
            }
            else
            {
                var splitFilter = filterText.Split(',');
                var hasAllFilters = true;
                foreach (var filter in splitFilter)
                {
                    if (string.IsNullOrEmpty(filter))
                        continue;
                    var trimmedFilter = filter.Trim();
                    var hasFilter = false;
                    
                    // Check if script
                    if (trimmedFilter.Contains("s:", StringComparison.OrdinalIgnoreCase))
                    {
                        // Check for any scripts
                        if (trimmedFilter.Equals("s:", StringComparison.OrdinalIgnoreCase))
                        {
                            if (Actor != null)
                            {
                                if (Actor.ScriptsCount > 0)
                                {
                                    hasFilter = true;
                                }
                            }
                        }
                        else
                        {
                            var scriptText = trimmedFilter.Replace("s:", "", StringComparison.OrdinalIgnoreCase).Trim();
                            var scriptFound = false;
                            if (Actor != null)
                            {
                                foreach (var script in Actor.Scripts)
                                {
                                    var name = TypeUtils.GetTypeDisplayName(script.GetType());
                                    var nameNoSpaces = name.Replace(" ", "");
                                    if (name.Contains(scriptText, StringComparison.OrdinalIgnoreCase) || nameNoSpaces.Contains(scriptText, StringComparison.OrdinalIgnoreCase))
                                    {
                                        scriptFound = true;
                                        break;
                                    }
                                }
                            }

                            hasFilter = scriptFound;
                        }
                    }
                    // Check for actor type
                    else if (trimmedFilter.Contains("a:", StringComparison.OrdinalIgnoreCase))
                    {
                        if (trimmedFilter.Equals("a:", StringComparison.OrdinalIgnoreCase))
                        {
                            if (Actor != null)
                                hasFilter = true;
                        }
                        else
                        {
                            if (Actor !=null)
                            {
                                var actorTypeText = trimmedFilter.Replace("a:", "", StringComparison.OrdinalIgnoreCase).Trim();
                                var name = TypeUtils.GetTypeDisplayName(Actor.GetType());
                                var nameNoSpaces = name.Replace(" ", "");
                                if (name.Contains(actorTypeText, StringComparison.OrdinalIgnoreCase) || nameNoSpaces.Contains(actorTypeText, StringComparison.OrdinalIgnoreCase))
                                    hasFilter = true;
                            }
                        }
                    }
                    // Match text
                    else
                    {
                        var text = Text;
                        if (QueryFilterHelper.Match(trimmedFilter, text, out QueryFilterHelper.Range[] ranges))
                        {
                            // Update highlights
                            if (_highlights == null)
                                _highlights = new List<Rectangle>(ranges.Length);
                            else
                                _highlights.Clear();
                            var font = Style.Current.FontSmall;
                            var textRect = TextRect;
                            for (int i = 0; i < ranges.Length; i++)
                            {
                                var start = font.GetCharPosition(text, ranges[i].StartIndex);
                                var end = font.GetCharPosition(text, ranges[i].EndIndex);
                                _highlights.Add(new Rectangle(start.X + textRect.X, textRect.Y, end.X - start.X, textRect.Height));
                            }
                            hasFilter = true;
                        }
                    }
                    
                    if (!hasFilter)
                    {
                        hasAllFilters = false;
                        break;
                    }
                }

                isThisVisible = hasAllFilters;
                if (!hasAllFilters)
                    _highlights?.Clear();
            }

            // Update children
            bool isAnyChildVisible = false;
            for (int i = 0; i < _children.Count; i++)
            {
                if (_children[i] is ActorTreeNode child)
                {
                    child.UpdateFilter(filterText);
                    isAnyChildVisible |= child.Visible;
                }
            }

            bool isExpanded = isAnyChildVisible;

            // Restore cached state on query filter clear
            if (noFilter && actor != null)
            {
                // Pick the correct id when inside a prefab window.
                var id = actor.HasPrefabLink && actor.Scene.Scene == null ? actor.PrefabObjectID : actor.ID;
                isExpanded = Editor.Instance.ProjectCache.IsExpandedActor(ref id);
            }

            if (isExpanded)
            {
                Expand(true);
            }
            else
            {
                Collapse(true);
            }

            Visible = isThisVisible | isAnyChildVisible;
        }

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            // Update hidden state
            var actor = Actor;
            if (actor && !_hasSearchFilter)
            {
                Visible = (actor.HideFlags & HideFlags.HideInHierarchy) == 0;
            }

            base.Update(deltaTime);
        }


        /// <inheritdoc />
        protected override bool ShowTooltip => true;

        /// <inheritdoc />
        public override bool OnShowTooltip(out string text, out Float2 location, out Rectangle area)
        {
            // Evaluate tooltip text once it's actually needed
            var actor = _actorNode.Actor;
            if (string.IsNullOrEmpty(TooltipText) && actor)
                TooltipText = Surface.SurfaceUtils.GetVisualScriptTypeDescription(TypeUtils.GetObjectType(actor));

            return base.OnShowTooltip(out text, out location, out area);
        }

        /// <inheritdoc />
        protected override Color CacheTextColor()
        {
            // Update node text color (based on ActorNode.IsActiveInHierarchy but with optimized logic a little)
            if (Parent is ActorTreeNode)
            {
                Color color = Style.Current.Foreground;
                var actor = Actor;
                if (actor)
                {
                    if (actor.HasPrefabLink)
                    {
                        // Prefab
                        color = Style.Current.ProgressNormal;
                    }

                    if (!actor.IsActiveInHierarchy)
                    {
                        // Inactive
                        return Style.Current.ForegroundGrey;
                    }

                    if (actor.HasScene && Editor.Instance.StateMachine.IsPlayMode && actor.IsStatic)
                    {
                        // Static
                        return color * 0.85f;
                    }
                }

                // Default
                return color;
            }

            return base.CacheTextColor();
        }

        /// <inheritdoc />
        public override int Compare(Control other)
        {
            if (other is ActorTreeNode node)
            {
                return _orderInParent - node._orderInParent;
            }
            return base.Compare(other);
        }

        /// <summary>
        /// Starts the actor renaming action.
        /// </summary>
        public void StartRenaming(EditorWindow window = null, Panel treePanel = null)
        {
            // Block renaming during scripts reload
            if (Editor.Instance.ProgressReporting.CompileScripts.IsActive)
                return;

            Select();

            // Disable scrolling of view
            if (window is SceneTreeWindow)
                (window as SceneTreeWindow).ScrollingOnSceneTreeView(false);
            else if (window is PrefabWindow)
                (window as PrefabWindow).ScrollingOnTreeView(false);

            // Start renaming the actor
            var rect = TextRect;
            if (treePanel != null)
            {
                treePanel.ScrollViewTo(this, true);
                rect.Size = new Float2(treePanel.Width - TextRect.Location.X, TextRect.Height);
            }
            var dialog = RenamePopup.Show(this, rect, _actorNode.Name, false);
            dialog.Renamed += OnRenamed;
            dialog.Closed += popup =>
            {
                // Enable scrolling of view
                if (window is SceneTreeWindow)
                    (window as SceneTreeWindow).ScrollingOnSceneTreeView(true);
                else if (window is PrefabWindow)
                    (window as PrefabWindow).ScrollingOnTreeView(true);
            };
        }

        private void OnRenamed(RenamePopup renamePopup)
        {
            using (new UndoBlock(ActorNode.Root.Undo, Actor, "Rename"))
                Actor.Name = renamePopup.Text.Trim();
        }

        /// <inheritdoc />
        protected override void OnExpandedChanged()
        {
            base.OnExpandedChanged();
            var actor = Actor;

            if (!IsLayoutLocked && actor)
            {
                // Pick the correct id when inside a prefab window.
                var id = actor.HasPrefabLink && !actor.HasScene ? actor.PrefabObjectID : actor.ID;
                Editor.Instance.ProjectCache.SetExpandedActor(ref id, IsExpanded);
            }
        }

        /// <inheritdoc />
        public override void Draw()
        {
            base.Draw();

            // Draw all highlights
            if (_highlights != null)
            {
                var style = Style.Current;
                var color = style.ProgressNormal * 0.6f;
                for (int i = 0; i < _highlights.Count; i++)
                    Render2D.FillRectangle(_highlights[i], color);
            }
        }

        /// <inheritdoc />
        protected override bool OnMouseDoubleClickHeader(ref Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left)
            {
                var sceneContext = this.GetSceneContext();
                switch (Editor.Instance.Options.Options.Input.DoubleClickSceneNode)
                {
                case SceneNodeDoubleClick.RenameActor:
                    sceneContext.RenameSelection();
                    return true;
                case SceneNodeDoubleClick.FocusActor:
                    sceneContext.FocusSelection();
                    return true;
                case SceneNodeDoubleClick.OpenPrefab:
                    Editor.Instance.Prefabs.OpenPrefab(ActorNode);
                    return true;
                case SceneNodeDoubleClick.Expand:
                default: break;
                }
            }
            return base.OnMouseDoubleClickHeader(ref location, button);
        }

        /// <inheritdoc />
        protected override DragDropEffect OnDragEnterHeader(DragData data)
        {
            // Check if cannot edit scene or there is no scene loaded (handle case for actors in prefab editor)
            if (_actorNode?.ParentScene != null)
            {
                if (!Editor.Instance.StateMachine.CurrentState.CanEditScene || !Level.IsAnySceneLoaded)
                    return DragDropEffect.None;
            }
            else
            {
                if (!Editor.Instance.StateMachine.CurrentState.CanEditContent)
                    return DragDropEffect.None;
            }

            if (_dragHandlers == null)
                _dragHandlers = new DragHandlers();

            // Check if drop actors
            if (_dragActors == null)
            {
                _dragActors = new DragActors(ValidateDragActor);
                _dragHandlers.Add(_dragActors);
            }
            if (_dragActors.OnDragEnter(data))
                return _dragActors.Effect;

            // Check if drop scripts
            if (_dragScripts == null)
            {
                _dragScripts = new DragScripts(ValidateDragScript);
                _dragHandlers.Add(_dragScripts);
            }
            if (_dragScripts.OnDragEnter(data))
                return _dragScripts.Effect;

            // Check if drag assets
            if (_dragAssets == null)
            {
                _dragAssets = new DragAssets(ValidateDragAsset);
                _dragHandlers.Add(_dragAssets);
            }
            if (_dragAssets.OnDragEnter(data))
                return _dragAssets.Effect;

            // Check if drag actor type
            if (_dragActorType == null)
            {
                _dragActorType = new DragActorType(ValidateDragActorType);
                _dragHandlers.Add(_dragActorType);
            }
            if (_dragActorType.OnDragEnter(data))
                return _dragActorType.Effect;

            // Check if drag control type
            if (_dragControlType == null)
            {
                _dragControlType = new DragControlType(ValidateDragControlType);
                _dragHandlers.Add(_dragControlType);
            }
            if (_dragControlType.OnDragEnter(data))
                return _dragControlType.Effect;

            // Check if drag script item
            if (_dragScriptItems == null)
            {
                _dragScriptItems = new DragScriptItems(ValidateDragScriptItem);
                _dragHandlers.Add(_dragScriptItems);
            }
            if (_dragScriptItems.OnDragEnter(data))
                return _dragScriptItems.Effect;

            return DragDropEffect.None;
        }

        /// <inheritdoc />
        protected override DragDropEffect OnDragMoveHeader(DragData data)
        {
            return _dragHandlers.Effect;
        }

        /// <inheritdoc />
        protected override void OnDragLeaveHeader()
        {
            _dragHandlers.OnDragLeave();
        }

        /// <inheritdoc />
        protected override DragDropEffect OnDragDropHeader(DragData data)
        {
            var result = DragDropEffect.None;

            Actor myActor = Actor;
            Actor newParent;
            int newOrder = -1;

            // Check if has no actor (only for Root Actor)
            if (myActor == null)
            {
                // Append to the last scene
                var scenes = Level.Scenes;
                if (scenes == null || scenes.Length == 0)
                    throw new InvalidOperationException("No scene loaded.");
                newParent = scenes[scenes.Length - 1];
            }
            else
            {
                newParent = myActor;

                // Use drag positioning to change target parent and index
                if (DragOverMode == DragItemPositioning.Above)
                {
                    if (myActor.HasParent)
                    {
                        newParent = myActor.Parent;
                        newOrder = myActor.OrderInParent;
                    }
                }
                else if (DragOverMode == DragItemPositioning.Below)
                {
                    if (myActor.HasParent)
                    {
                        newParent = myActor.Parent;
                        newOrder = myActor.OrderInParent + 1;
                    }
                }
            }
            if (newParent == null)
                throw new InvalidOperationException("Missing parent actor.");

            // Drag actors
            if (_dragActors != null && _dragActors.HasValidDrag)
            {
                bool worldPositionsStays = Root.GetKey(KeyboardKeys.Control) == false;
                var objects = new SceneObject[_dragActors.Objects.Count];
                var treeNodes = new TreeNode[_dragActors.Objects.Count];
                for (int i = 0; i < objects.Length; i++)
                {
                    objects[i] = _dragActors.Objects[i].Actor;
                    treeNodes[i] = _dragActors.Objects[i].TreeNode;
                }
                var action = new ParentActorsAction(objects, newParent, newOrder, worldPositionsStays);
                ActorNode.Root.Undo?.AddAction(action);
                action.Do();
                ParentTree.Focus();
                ParentTree.Select(treeNodes.ToList());
                result = DragDropEffect.Move;
            }
            // Drag scripts
            else if (_dragScripts != null && _dragScripts.HasValidDrag)
            {
                var objects = new SceneObject[_dragScripts.Objects.Count];
                for (int i = 0; i < objects.Length; i++)
                    objects[i] = _dragScripts.Objects[i];
                var action = new ParentActorsAction(objects, newParent, newOrder);
                ActorNode.Root.Undo?.AddAction(action);
                action.Do();
                Select();
                result = DragDropEffect.Move;
            }
            // Drag assets
            else if (_dragAssets != null && _dragAssets.HasValidDrag)
            {
                var spawnParent = myActor;
                if (DragOverMode == DragItemPositioning.Above || DragOverMode == DragItemPositioning.Below)
                    spawnParent = newParent;

                for (int i = 0; i < _dragAssets.Objects.Count; i++)
                {
                    var item = _dragAssets.Objects[i];
                    var actor = item.OnEditorDrop(this);
                    if (spawnParent.GetType() != typeof(Scene))
                    {
                        // Set all Actors static flags to match parents
                        List<Actor> childActors = new List<Actor>();
                        Utilities.Utils.GetActorsTree(childActors, actor);
                        foreach (var child in childActors)
                        {
                            child.StaticFlags = spawnParent.StaticFlags;
                        }
                    }
                    actor.Name = item.ShortName;
                    if (_dragAssets.Objects[i] is not PrefabItem)
                        actor.Transform = Transform.Identity;
                    var previousTrans = actor.Transform;
                    ActorNode.Root.Spawn(actor, spawnParent);
                    actor.LocalTransform = previousTrans;
                    actor.OrderInParent = newOrder;
                }
                result = DragDropEffect.Move;
            }
            // Drag actor type
            else if (_dragActorType != null && _dragActorType.HasValidDrag)
            {
                for (int i = 0; i < _dragActorType.Objects.Count; i++)
                {
                    var item = _dragActorType.Objects[i];
                    var actor = item.CreateInstance() as Actor;
                    if (actor == null)
                    {
                        Editor.LogWarning("Failed to spawn actor of type " + item.TypeName);
                        continue;
                    }
                    actor.StaticFlags = newParent.StaticFlags;
                    actor.Name = item.Name;
                    ActorNode.Root.Spawn(actor, newParent);
                    actor.OrderInParent = newOrder;
                }
                result = DragDropEffect.Move;
            }
            // Drag control type
            else if (_dragControlType != null && _dragControlType.HasValidDrag)
            {
                for (int i = 0; i < _dragControlType.Objects.Count; i++)
                {
                    var item = _dragControlType.Objects[i];
                    var control = item.CreateInstance() as Control;
                    if (control == null)
                    {
                        Editor.LogWarning("Failed to spawn UIControl with control type " + item.TypeName);
                        continue;
                    }
                    var uiControl = new UIControl
                    {
                        Control = control,
                        StaticFlags = newParent.StaticFlags,
                        Name = item.Name,
                    };
                    ActorNode.Root.Spawn(uiControl, newParent);
                    uiControl.OrderInParent = newOrder;
                }
                result = DragDropEffect.Move;
            }
            // Drag script item
            else if (_dragScriptItems != null && _dragScriptItems.HasValidDrag)
            {
                var spawnParent = myActor;
                if (DragOverMode == DragItemPositioning.Above || DragOverMode == DragItemPositioning.Below)
                    spawnParent = newParent;

                for (int i = 0; i < _dragScriptItems.Objects.Count; i++)
                {
                    var item = _dragScriptItems.Objects[i];
                    var actorType = Editor.Instance.CodeEditing.Actors.Get(item);
                    var scriptType = Editor.Instance.CodeEditing.Scripts.Get(item);
                    if (actorType != ScriptType.Null)
                    {
                        var actor = actorType.CreateInstance() as Actor;
                        if (actor == null)
                        {
                            Editor.LogWarning("Failed to spawn actor of type " + actorType.TypeName);
                            continue;
                        }
                        actor.StaticFlags = spawnParent.StaticFlags;
                        actor.Name = actorType.Name;
                        actor.Transform = spawnParent.Transform;
                        ActorNode.Root.Spawn(actor, spawnParent);
                        actor.OrderInParent = newOrder;
                    }
                    else if (scriptType != ScriptType.Null)
                    {
                        if (DragOverMode == DragItemPositioning.Above || DragOverMode == DragItemPositioning.Below)
                        {
                            Editor.LogWarning("Failed to spawn script of type " + actorType.TypeName);
                            continue;
                        }
                        IUndoAction action = new AddRemoveScript(true, newParent, scriptType);
                        Select();
                        ActorNode.Root.Undo?.AddAction(action);
                        action.Do();
                    }
                }
                result = DragDropEffect.Move;
            }

            // Clear cache
            _dragHandlers.OnDragDrop(null);

            // Check if scene has been modified
            if (result != DragDropEffect.None)
            {
                var node = SceneGraphFactory.FindNode(newParent.ID) as ActorNode;
                node?.TreeNode.Expand();
            }

            return result;
        }

        private bool ValidateDragActor(ActorNode actorNode)
        {
            // Reject dragging actors not linked to scene (eg. from prefab) or in the opposite way
            var thisHasScene = ActorNode.ParentScene != null;
            var otherHasScene = actorNode.ParentScene != null;
            if (thisHasScene != otherHasScene)
                return false;

            // Reject dragging actors between prefab windows (different roots)
            if (!thisHasScene && ActorNode.Root != actorNode.Root)
                return false;

            // Reject dragging parents and itself
            return actorNode.Actor != null && actorNode != ActorNode && actorNode.Find(Actor) == null;
        }

        private bool ValidateDragScript(Script script)
        {
            // Reject dragging scripts not linked to scene (eg. from prefab) or in the opposite way
            var thisHasScene = Actor.HasScene;
            var otherHasScene = script.HasScene;
            if (thisHasScene != otherHasScene)
                return false;

            // Reject dragging parents and itself
            return script.Actor != null && script.Parent != Actor;
        }

        private bool ValidateDragAsset(AssetItem assetItem)
        {
            return assetItem.OnEditorDrag(this);
        }

        private static bool ValidateDragActorType(ScriptType actorType)
        {
            return Editor.Instance.CodeEditing.Actors.Get().Contains(actorType);
        }

        private static bool ValidateDragControlType(ScriptType controlType)
        {
            return Editor.Instance.CodeEditing.Controls.Get().Contains(controlType);
        }

        private bool ValidateDragScriptItem(ScriptItem script)
        {
            return Editor.Instance.CodeEditing.Actors.Get(script) != ScriptType.Null || Editor.Instance.CodeEditing.Scripts.Get(script) != ScriptType.Null;
        }

        /// <inheritdoc />
        protected override void DoDragDrop()
        {
            DragData data;
            var tree = ParentTree;

            // Check if this node is selected
            if (tree.Selection.Contains(this))
            {
                // Get selected actors
                var actors = new List<ActorNode>();
                for (var i = 0; i < tree.Selection.Count; i++)
                {
                    var e = tree.Selection[i];

                    // Skip if parent is already selected to keep correct parenting
                    if (tree.Selection.Contains(e.Parent))
                        continue;

                    if (e is ActorTreeNode node && node.ActorNode.CanDrag)
                        actors.Add(node.ActorNode);
                }
                data = DragActors.GetDragData(actors);
            }
            else
            {
                data = DragActors.GetDragData(ActorNode);
            }

            // Start drag operation
            DoDragDrop(data);
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            _dragActors = null;
            _dragScripts = null;
            _dragAssets = null;
            _dragActorType = null;
            _dragControlType = null;
            _dragScriptItems = null;
            _dragHandlers?.Clear();
            _dragHandlers = null;
            _highlights = null;

            base.OnDestroy();
        }
    }
}
