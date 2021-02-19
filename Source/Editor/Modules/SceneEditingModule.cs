// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using FlaxEditor.Actions;
using FlaxEditor.SceneGraph;
using FlaxEngine;

namespace FlaxEditor.Modules
{
    /// <summary>
    /// Editing scenes module. Manages scene objects selection and editing modes.
    /// </summary>
    /// <seealso cref="FlaxEditor.Modules.EditorModule" />
    public sealed class SceneEditingModule : EditorModule
    {
        /// <summary>
        /// The selected objects.
        /// </summary>
        public readonly List<SceneGraphNode> Selection = new List<SceneGraphNode>(64);

        /// <summary>
        /// Gets the amount of the selected objects.
        /// </summary>
        public int SelectionCount => Selection.Count;

        /// <summary>
        /// Gets a value indicating whether any object is selected.
        /// </summary>
        public bool HasSthSelected => Selection.Count > 0;

        /// <summary>
        /// Occurs when selected objects collection gets changed.
        /// </summary>
        public event Action SelectionChanged;

        /// <summary>
        /// Occurs before spawning actor to game action.
        /// </summary>
        public event Action SpawnBegin;

        /// <summary>
        /// Occurs after spawning actor to game action.
        /// </summary>
        public event Action SpawnEnd;

        /// <summary>
        /// Occurs before selection delete action.
        /// </summary>
        public event Action SelectionDeleteBegin;

        /// <summary>
        /// Occurs after selection delete action.
        /// </summary>
        public event Action SelectionDeleteEnd;

        internal SceneEditingModule(Editor editor)
        : base(editor)
        {
        }

        /// <summary>
        /// Selects all scenes.
        /// </summary>
        public void SelectAllScenes()
        {
            // Select all scenes (linked to the root node)
            Select(Editor.Scene.Root.ChildNodes);
        }

        /// <summary>
        /// Selects the specified actor (finds it's scene graph node).
        /// </summary>
        /// <param name="actor">The actor.</param>
        public void Select(Actor actor)
        {
            var node = Editor.Scene.GetActorNode(actor);
            if (node != null)
                Select(node);
        }

        /// <summary>
        /// Selects the specified collection of objects.
        /// </summary>
        /// <param name="selection">The selection.</param>
        /// <param name="additive">if set to <c>true</c> will use additive mode, otherwise will clear previous selection.</param>
        public void Select(List<SceneGraphNode> selection, bool additive = false)
        {
            if (selection == null)
                throw new ArgumentNullException();

            // Prevent from selecting null nodes
            selection.RemoveAll(x => x == null);

            // Check if won't change
            if (!additive && Selection.Count == selection.Count && Selection.SequenceEqual(selection))
                return;

            var before = Selection.ToArray();
            if (!additive)
                Selection.Clear();
            Selection.AddRange(selection);

            SelectionChange(before);
        }

        /// <summary>
        /// Selects the specified collection of objects.
        /// </summary>
        /// <param name="selection">The selection.</param>
        /// <param name="additive">if set to <c>true</c> will use additive mode, otherwise will clear previous selection.</param>
        public void Select(SceneGraphNode[] selection, bool additive = false)
        {
            if (selection == null)
                throw new ArgumentNullException();

            Select(selection.ToList(), additive);
        }

        /// <summary>
        /// Selects the specified object.
        /// </summary>
        /// <param name="selection">The selection.</param>
        /// <param name="additive">if set to <c>true</c> will use additive mode, otherwise will clear previous selection.</param>
        public void Select(SceneGraphNode selection, bool additive = false)
        {
            if (selection == null)
                throw new ArgumentNullException();

            // Check if won't change
            if (!additive && Selection.Count == 1 && Selection[0] == selection)
                return;
            if (additive && Selection.Contains(selection))
                return;

            var before = Selection.ToArray();
            if (!additive)
                Selection.Clear();
            Selection.Add(selection);

            SelectionChange(before);
        }

        /// <summary>
        /// Deselects given object.
        /// </summary>
        public void Deselect(SceneGraphNode node)
        {
            if (!Selection.Contains(node))
                return;

            var before = Selection.ToArray();
            Selection.Remove(node);

            SelectionChange(before);
        }

        /// <summary>
        /// Clears selected objects collection.
        /// </summary>
        public void Deselect()
        {
            // Check if won't change
            if (Selection.Count == 0)
                return;

            var before = Selection.ToArray();
            Selection.Clear();

            SelectionChange(before);
        }

        private void SelectionChange(SceneGraphNode[] before)
        {
            Undo.AddAction(new SelectionChangeAction(before, Selection.ToArray(), OnSelectionUndo));

            OnSelectionChanged();
        }

        private void OnSelectionUndo(SceneGraphNode[] toSelect)
        {
            Selection.Clear();
            if (toSelect != null)
            {
                for (int i = 0; i < toSelect.Length; i++)
                {
                    if (toSelect[i] != null)
                        Selection.Add(toSelect[i]);
                    else
                        Editor.LogWarning("Null scene graph node to select");
                }
            }

            OnSelectionChanged();
        }

        private void OnDirty(ActorNode node)
        {
            var options = Editor.Options.Options;
            var isPlayMode = Editor.StateMachine.IsPlayMode;
            var actor = node.Actor;

            // Auto CSG mesh rebuild
            if (!isPlayMode && options.General.AutoRebuildCSG)
            {
                if (actor is BoxBrush && actor.Scene)
                    actor.Scene.BuildCSG(options.General.AutoRebuildCSGTimeoutMs);
            }

            // Auto NavMesh rebuild
            if (!isPlayMode && options.General.AutoRebuildNavMesh && actor.Scene && node.AffectsNavigationWithChildren)
            {
                var bounds = actor.BoxWithChildren;
                Navigation.BuildNavMesh(actor.Scene, bounds, options.General.AutoRebuildNavMeshTimeoutMs);
            }
        }

        private void OnDirty(IEnumerable<SceneGraphNode> objects)
        {
            var options = Editor.Options.Options;
            var isPlayMode = Editor.StateMachine.IsPlayMode;

            // Auto CSG mesh rebuild
            if (!isPlayMode && options.General.AutoRebuildCSG)
            {
                foreach (var obj in objects)
                {
                    if (obj is ActorNode node && node.Actor is BoxBrush)
                        node.Actor.Scene.BuildCSG(options.General.AutoRebuildCSGTimeoutMs);
                }
            }

            // Auto NavMesh rebuild
            if (!isPlayMode && options.General.AutoRebuildNavMesh)
            {
                foreach (var obj in objects)
                {
                    if (obj is ActorNode node && node.Actor.Scene && node.AffectsNavigationWithChildren)
                    {
                        var bounds = node.Actor.BoxWithChildren;
                        Navigation.BuildNavMesh(node.Actor.Scene, bounds, options.General.AutoRebuildNavMeshTimeoutMs);
                    }
                }
            }
        }

        /// <summary>
        /// Spawns the specified actor to the game (with undo).
        /// </summary>
        /// <param name="actor">The actor.</param>
        /// <param name="parent">The parent actor. Set null as default.</param>
        /// <param name="autoSelect">True if automatically select the spawned actor, otherwise false.</param>
        public void Spawn(Actor actor, Actor parent = null, bool autoSelect = true)
        {
            bool isPlayMode = Editor.StateMachine.IsPlayMode;

            if (Level.IsAnySceneLoaded == false)
                throw new InvalidOperationException("Cannot spawn actor when no scene is loaded.");

            SpawnBegin?.Invoke();

            // Add it
            Level.SpawnActor(actor, parent);

            // Peek spawned node
            var actorNode = Editor.Instance.Scene.GetActorNode(actor);
            if (actorNode == null)
                throw new InvalidOperationException("Failed to create scene node for the spawned actor.");

            // During play in editor mode spawned actors should be dynamic (user can move them)
            if (isPlayMode)
                actor.StaticFlags = StaticFlags.None;

            // Call post spawn action (can possibly setup custom default values)
            actorNode.PostSpawn();

            // Create undo action
            IUndoAction action = new DeleteActorsAction(new List<SceneGraphNode>(1) { actorNode }, true);
            if (autoSelect)
            {
                var before = Selection.ToArray();
                Selection.Clear();
                Selection.Add(actorNode);
                OnSelectionChanged();
                action = new MultiUndoAction(action, new SelectionChangeAction(before, Selection.ToArray(), OnSelectionUndo));
            }
            Undo.AddAction(action);

            // Mark scene as dirty
            Editor.Scene.MarkSceneEdited(actor.Scene);

            SpawnEnd?.Invoke();

            OnDirty(actorNode);
        }

        /// <summary>
        /// Converts the selected actor to another type.
        /// </summary>
        /// <param name="to">The type to convert in.</param>
        public void Convert(Type to)
        {
            if (!Editor.SceneEditing.HasSthSelected || !(Editor.SceneEditing.Selection[0] is ActorNode))
                return;

            if (Level.IsAnySceneLoaded == false)
                throw new InvalidOperationException("Cannot spawn actor when no scene is loaded.");

            var actionList = new IUndoAction[4];
            Actor old = ((ActorNode)Editor.SceneEditing.Selection[0]).Actor;
            Actor actor = (Actor)FlaxEngine.Object.New(to);
            var parent = old.Parent;
            var orderInParent = old.OrderInParent;

            SelectionDeleteBegin?.Invoke();

            actionList[0] = new SelectionChangeAction(Selection.ToArray(), new SceneGraphNode[0], OnSelectionUndo);
            actionList[0].Do();

            actionList[1] = new DeleteActorsAction(new List<SceneGraphNode>
            {
                Editor.Instance.Scene.GetActorNode(old)
            });
            actionList[1].Do();

            SelectionDeleteEnd?.Invoke();

            SpawnBegin?.Invoke();

            // Copy properties
            actor.Transform = old.Transform;
            actor.StaticFlags = old.StaticFlags;
            actor.HideFlags = old.HideFlags;
            actor.Layer = old.Layer;
            actor.Tag = old.Tag;
            actor.Name = old.Name;
            actor.IsActive = old.IsActive;

            // Spawn actor
            Level.SpawnActor(actor, parent);
            if (parent != null)
                actor.OrderInParent = orderInParent;
            if (Editor.StateMachine.IsPlayMode)
                actor.StaticFlags = StaticFlags.None;

            // Move children
            for (var i = old.ScriptsCount - 1; i >= 0; i--)
            {
                var script = old.Scripts[i];
                script.Actor = actor;
                Guid newid = Guid.NewGuid();
                FlaxEngine.Object.Internal_ChangeID(FlaxEngine.Object.GetUnmanagedPtr(script), ref newid);
            }
            for (var i = old.Children.Length - 1; i >= 0; i--)
            {
                old.Children[i].Parent = actor;
            }

            var actorNode = Editor.Instance.Scene.GetActorNode(actor);
            if (actorNode == null)
                throw new InvalidOperationException("Failed to create scene node for the spawned actor.");

            actorNode.PostSpawn();
            Editor.Scene.MarkSceneEdited(actor.Scene);

            actionList[2] = new DeleteActorsAction(new List<SceneGraphNode>
            {
                actorNode
            }, true);

            actionList[3] = new SelectionChangeAction(new SceneGraphNode[0], new SceneGraphNode[] { actorNode }, OnSelectionUndo);
            actionList[3].Do();

            var actions = new MultiUndoAction(actionList);
            Undo.AddAction(actions);

            SpawnEnd?.Invoke();

            OnDirty(actorNode);
        }

        /// <summary>
        /// Deletes the selected objects. Supports undo/redo.
        /// </summary>
        public void Delete()
        {
            // Peek things that can be removed
            var objects = Selection.Where(x => x.CanDelete).ToList().BuildAllNodes().Where(x => x.CanDelete).ToList();
            if (objects.Count == 0)
                return;

            SelectionDeleteBegin?.Invoke();

            // Change selection
            var action1 = new SelectionChangeAction(Selection.ToArray(), new SceneGraphNode[0], OnSelectionUndo);

            // Delete objects
            var action2 = new DeleteActorsAction(objects);

            // Merge two actions and perform them
            var action = new MultiUndoAction(new IUndoAction[]
            {
                action1,
                action2
            }, action2.ActionString);
            action.Do();
            Undo.AddAction(action);

            SelectionDeleteEnd?.Invoke();

            OnDirty(objects);
        }

        /// <summary>
        /// Copies the selected objects.
        /// </summary>
        public void Copy()
        {
            // Peek things that can be copied (copy all actors)
            var objects = Selection.Where(x => x.CanCopyPaste).ToList().BuildAllNodes().Where(x => x.CanCopyPaste && x is ActorNode).ToList();
            if (objects.Count == 0)
                return;

            // Serialize actors
            var actors = objects.ConvertAll(x => ((ActorNode)x).Actor);
            var data = Actor.ToBytes(actors.ToArray());
            if (data == null)
            {
                Editor.LogError("Failed to copy actors data.");
                return;
            }

            // Copy data
            Clipboard.RawData = data;
        }


        /// <summary>
        /// Pastes the copied objects. Supports undo/redo.
        /// </summary>
        public void Paste()
        {
            Paste(null);
        }

        /// <summary>
        /// Pastes the copied objects. Supports undo/redo.
        /// </summary>
        /// <param name="pasteTargetActor">The target actor to paste copied data.</param>
        public void Paste(Actor pasteTargetActor)
        {
            // Get clipboard data
            var data = Clipboard.RawData;

            // Set paste target if only one actor is selected and no target provided
            if (pasteTargetActor == null && SelectionCount == 1 && Selection[0] is ActorNode actorNode)
            {
                pasteTargetActor = actorNode.Actor;
            }

            // Create paste action
            var pasteAction = PasteActorsAction.Paste(data, pasteTargetActor?.ID ?? Guid.Empty);
            if (pasteAction != null)
            {
                pasteAction.Do(out _, out var nodeParents);

                // Select spawned objects (parents only)
                var selectAction = new SelectionChangeAction(Selection.ToArray(), nodeParents.Cast<SceneGraphNode>().ToArray(), OnSelectionUndo);
                selectAction.Do();

                // Build single compound undo action that pastes the actors and selects the created objects (parents only)
                Undo.AddAction(new MultiUndoAction(pasteAction, selectAction));
                OnSelectionChanged();
            }
        }

        /// <summary>
        /// Cuts the selected objects. Supports undo/redo.
        /// </summary>
        public void Cut()
        {
            Copy();
            Delete();
        }

        /// <summary>
        /// Duplicates the selected objects. Supports undo/redo.
        /// </summary>
        public void Duplicate()
        {
            // Peek things that can be copied (copy all actors)
            var nodes = Selection.Where(x => x.CanDuplicate).ToList().BuildAllNodes();
            if (nodes.Count == 0)
                return;
            var actors = new List<Actor>();
            var newSelection = new List<SceneGraphNode>();
            List<IUndoAction> customUndoActions = null;
            foreach (var node in nodes)
            {
                if (node.CanDuplicate)
                {
                    if (node is ActorNode actorNode)
                    {
                        actors.Add(actorNode.Actor);
                    }
                    else
                    {
                        var customDuplicatedObject = node.Duplicate(out var customUndoAction);
                        if (customDuplicatedObject != null)
                            newSelection.Add(customDuplicatedObject);
                        if (customUndoAction != null)
                        {
                            if (customUndoActions == null)
                                customUndoActions = new List<IUndoAction>();
                            customUndoActions.Add(customUndoAction);
                        }
                    }
                }
            }
            if (actors.Count == 0)
            {
                // Duplicate custom scene graph nodes only without actors
                if (newSelection.Count != 0)
                {
                    // Select spawned objects (parents only)
                    var selectAction = new SelectionChangeAction(Selection.ToArray(), newSelection.ToArray(), OnSelectionUndo);
                    selectAction.Do();

                    // Build a single compound undo action that pastes the actors, pastes custom stuff (scene graph extension) and selects the created objects (parents only)
                    var customUndoActionsCount = customUndoActions?.Count ?? 0;
                    var undoActions = new IUndoAction[1 + customUndoActionsCount];
                    for (int i = 0; i < customUndoActionsCount; i++)
                        undoActions[i] = customUndoActions[i];
                    undoActions[undoActions.Length - 1] = selectAction;

                    Undo.AddAction(new MultiUndoAction(undoActions));
                    OnSelectionChanged();
                }
                return;
            }

            // Serialize actors
            var data = Actor.ToBytes(actors.ToArray());
            if (data == null)
            {
                Editor.LogError("Failed to copy actors data.");
                return;
            }

            // Create paste action (with selecting spawned objects)
            var pasteAction = PasteActorsAction.Duplicate(data, Guid.Empty);
            if (pasteAction != null)
            {
                pasteAction.Do(out _, out var nodeParents);

                // Select spawned objects (parents only)
                newSelection.AddRange(nodeParents);
                var selectAction = new SelectionChangeAction(Selection.ToArray(), newSelection.ToArray(), OnSelectionUndo);
                selectAction.Do();

                // Build a single compound undo action that pastes the actors, pastes custom stuff (scene graph extension) and selects the created objects (parents only)
                var customUndoActionsCount = customUndoActions?.Count ?? 0;
                var undoActions = new IUndoAction[2 + customUndoActionsCount];
                undoActions[0] = pasteAction;
                for (int i = 0; i < customUndoActionsCount; i++)
                    undoActions[i + 1] = customUndoActions[i];
                undoActions[undoActions.Length - 1] = selectAction;

                Undo.AddAction(new MultiUndoAction(undoActions));
                OnSelectionChanged();
            }
        }

        /// <summary>
        /// Called when selection gets changed. Invokes the other events and updates editor. Call it when you manually modify selected objects collection.
        /// </summary>
        public void OnSelectionChanged()
        {
            SelectionChanged?.Invoke();
        }

        /// <inheritdoc />
        public override void OnInit()
        {
            // Deselect actors on remove (and actor child nodes)
            Editor.Scene.ActorRemoved += Deselect;
            Editor.Scene.Root.ActorChildNodesDispose += OnActorChildNodesDispose;
        }

        private void OnActorChildNodesDispose(ActorNode node)
        {
            // TODO: cache if selection contains any actor child node and skip this loop if no need to iterate

            // Deselect child nodes
            for (int i = 0; i < node.ChildNodes.Count; i++)
            {
                if (Selection.Contains(node.ChildNodes[i]))
                {
                    Deselect();
                    return;
                }
            }
        }
    }
}
