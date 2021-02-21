// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using FlaxEditor.SceneGraph;
using FlaxEditor.SceneGraph.Actors;
using FlaxEngine;
using Object = FlaxEngine.Object;

namespace FlaxEditor.Modules
{
    /// <summary>
    /// Scenes and actors management module.
    /// </summary>
    /// <seealso cref="FlaxEditor.Modules.EditorModule" />
    public sealed class SceneModule : EditorModule
    {
        /// <summary>
        /// The root node for the scene graph created for the loaded scenes and actors hierarchy.
        /// </summary>
        /// <seealso cref="FlaxEditor.SceneGraph.RootNode" />
        public class ScenesRootNode : RootNode
        {
            /// <inheritdoc />
            public override void Spawn(Actor actor, Actor parent)
            {
                Editor.Instance.SceneEditing.Spawn(actor, parent);
            }

            /// <inheritdoc />
            public override Undo Undo => Editor.Instance.Undo;
        }

        /// <summary>
        /// The root tree node for the whole scene graph.
        /// </summary>
        public ScenesRootNode Root;

        /// <summary>
        /// Occurs when actor gets removed. Editor and all submodules should remove references to that actor.
        /// </summary>
        public event Action<ActorNode> ActorRemoved;

        internal SceneModule(Editor editor)
        : base(editor)
        {
            // After editor cache but before the windows
            InitOrder = -900;
        }

        /// <summary>
        /// Marks the scene as modified.
        /// </summary>
        /// <param name="scene">The scene.</param>
        public void MarkSceneEdited(Scene scene)
        {
            MarkSceneEdited(GetActorNode(scene) as SceneNode);
        }

        /// <summary>
        /// Marks the scene as modified.
        /// </summary>
        /// <param name="scene">The scene.</param>
        public void MarkSceneEdited(SceneNode scene)
        {
            if (scene != null)
                scene.IsEdited = true;
        }

        /// <summary>
        /// Marks the scenes as modified.
        /// </summary>
        /// <param name="scenes">The scenes.</param>
        public void MarkSceneEdited(IEnumerable<Scene> scenes)
        {
            foreach (var scene in scenes)
                MarkSceneEdited(scene);
        }

        /// <summary>
        /// Marks all the scenes as modified.
        /// </summary>
        public void MarkAllScenesEdited()
        {
            MarkSceneEdited(Level.Scenes);
        }

        /// <summary>
        /// Determines whether the specified scene is edited.
        /// </summary>
        /// <param name="scene">The scene.</param>
        /// <returns>
        ///   <c>true</c> if the specified scene is edited; otherwise, <c>false</c>.
        /// </returns>
        public bool IsEdited(Scene scene)
        {
            var node = GetActorNode(scene) as SceneNode;
            return node?.IsEdited ?? false;
        }

        /// <summary>
        /// Determines whether any scene is edited.
        /// </summary>
        /// <returns>
        ///   <c>true</c> if any scene is edited; otherwise, <c>false</c>.
        /// </returns>
        public bool IsEdited()
        {
            foreach (var scene in Root.ChildNodes)
            {
                if (scene is SceneNode node && node.IsEdited)
                    return true;
            }
            return false;
        }

        /// <summary>
        /// Determines whether every scene is edited.
        /// </summary>
        /// <returns>
        ///   <c>true</c> if every scene is edited; otherwise, <c>false</c>.
        /// </returns>
        public bool IsEverySceneEdited()
        {
            foreach (var scene in Root.ChildNodes)
            {
                if (scene is SceneNode node && !node.IsEdited)
                    return false;
            }
            return true;
        }

        /// <summary>
        /// Creates the new scene file. The default scene contains set of simple actors.
        /// </summary>
        /// <param name="path">The path.</param>
        public void CreateSceneFile(string path)
        {
            // Create a sample scene
            var scene = new Scene
            {
                StaticFlags = StaticFlags.FullyStatic
            };

            //
            var sun = scene.AddChild<DirectionalLight>();
            sun.Name = "Sun";
            sun.LocalPosition = new Vector3(40, 160, 0);
            sun.LocalEulerAngles = new Vector3(45, 0, 0);
            sun.StaticFlags = StaticFlags.FullyStatic;
            //
            var sky = scene.AddChild<Sky>();
            sky.Name = "Sky";
            sky.LocalPosition = new Vector3(40, 150, 0);
            sky.SunLight = sun;
            sky.StaticFlags = StaticFlags.FullyStatic;
            //
            var skyLight = scene.AddChild<SkyLight>();
            skyLight.Mode = SkyLight.Modes.CustomTexture;
            skyLight.Brightness = 2.5f;
            skyLight.CustomTexture = FlaxEngine.Content.LoadAsyncInternal<CubeTexture>(EditorAssets.DefaultSkyCubeTexture);
            skyLight.StaticFlags = StaticFlags.FullyStatic;
            //
            var floor = scene.AddChild<StaticModel>();
            floor.Name = "Floor";
            floor.Scale = new Vector3(4, 0.5f, 4);
            floor.Model = FlaxEngine.Content.LoadAsync<Model>(StringUtils.CombinePaths(Globals.EngineContentFolder, "Editor/Primitives/Cube.flax"));
            if (floor.Model)
            {
                floor.Model.WaitForLoaded();
                floor.SetMaterial(0, FlaxEngine.Content.LoadAsync<MaterialBase>(StringUtils.CombinePaths(Globals.EngineContentFolder, "Engine/WhiteMaterial.flax")));
            }
            floor.StaticFlags = StaticFlags.FullyStatic;
            //
            var cam = scene.AddChild<Camera>();
            cam.Name = "Camera";
            cam.Position = new Vector3(0, 150, -300);

            // Serialize
            var bytes = Level.SaveSceneToBytes(scene);

            // Cleanup
            Object.Destroy(ref scene);

            if (bytes == null || bytes.Length == 0)
                throw new Exception("Failed to serialize scene.");

            // Write to file
            using (var fileStream = new FileStream(path, FileMode.Create, FileAccess.Write, FileShare.Read))
                fileStream.Write(bytes, 0, bytes.Length);
        }

        /// <summary>
        /// Saves scene (async).
        /// </summary>
        /// <param name="scene">Scene to save.</param>
        public void SaveScene(Scene scene)
        {
            SaveScene(GetActorNode(scene) as SceneNode);
        }

        /// <summary>
        /// Saves scene (async).
        /// </summary>
        /// <param name="scene">Scene to save.</param>
        public void SaveScene(SceneNode scene)
        {
            if (!scene.IsEdited)
                return;

            scene.IsEdited = false;
            Level.SaveSceneAsync(scene.Scene);
        }

        /// <summary>
        /// Saves all open scenes (async).
        /// </summary>
        public void SaveScenes()
        {
            if (!IsEdited())
                return;

            foreach (var scene in Root.ChildNodes)
            {
                if (scene is SceneNode node)
                    node.IsEdited = false;
            }
            Level.SaveAllScenesAsync();
        }

        /// <summary>
        /// Opens scene (async).
        /// </summary>
        /// <param name="sceneId">Scene ID</param>
        /// <param name="additive">True if don't close opened scenes and just add new scene to them, otherwise will release current scenes and load single one.</param>
        public void OpenScene(Guid sceneId, bool additive = false)
        {
            // Check if cannot change scene now
            if (!Editor.StateMachine.CurrentState.CanChangeScene)
                return;

            if (!additive)
            {
                // Ensure to save all pending changes
                if (CheckSaveBeforeClose())
                    return;
            }

            // Load scene
            Editor.StateMachine.ChangingScenesState.LoadScene(sceneId, additive);
        }

        /// <summary>
        /// Closes scene (async).
        /// </summary>
        /// <param name="scene">The scene.</param>
        public void CloseScene(Scene scene)
        {
            // Check if cannot change scene now
            if (!Editor.StateMachine.CurrentState.CanChangeScene)
                return;

            // Ensure to save all pending changes
            if (CheckSaveBeforeClose())
                return;

            // Unload scene
            Editor.StateMachine.ChangingScenesState.UnloadScene(scene);
        }

        /// <summary>
        /// Closes all opened scene (async).
        /// </summary>
        public void CloseAllScenes()
        {
            // Check if cannot change scene now
            if (!Editor.StateMachine.CurrentState.CanChangeScene)
                return;

            // Ensure to save all pending changes
            if (CheckSaveBeforeClose())
                return;

            // Unload scenes
            Editor.StateMachine.ChangingScenesState.UnloadScene(Level.Scenes);
        }

        /// <summary>
        /// Show save before scene load/unload action.
        /// </summary>
        /// <param name="scene">The scene that will be closed.</param>
        /// <returns>True if action has been canceled, otherwise false</returns>
        public bool CheckSaveBeforeClose(SceneNode scene)
        {
            // Check if scene was edited after last saving
            if (scene.IsEdited)
            {
                // Ask user for further action
                var result = MessageBox.Show(
                                             string.Format("Scene \'{0}\' has been edited. Save before closing?", scene.Name),
                                             "Close without saving?",
                                             MessageBoxButtons.YesNoCancel,
                                             MessageBoxIcon.Question
                                            );
                if (result == DialogResult.OK || result == DialogResult.Yes)
                {
                    // Save and close
                    SaveScene(scene);
                }
                else if (result == DialogResult.Cancel || result == DialogResult.Abort)
                {
                    // Cancel closing
                    return true;
                }
            }

            ClearRefsToSceneObjects();

            return false;
        }

        /// <summary>
        /// Show save before scene load/unload action.
        /// </summary>
        /// <returns>True if action has been canceled, otherwise false</returns>
        public bool CheckSaveBeforeClose()
        {
            // Check if scene was edited after last saving
            if (IsEdited())
            {
                // Ask user for further action
                var scenes = Level.Scenes;
                var result = MessageBox.Show(
                                             scenes.Length == 1 ? string.Format("Scene \'{0}\' has been edited. Save before closing?", scenes[0].Name) : string.Format("{0} scenes have been edited. Save before closing?", scenes.Length),
                                             "Close without saving?",
                                             MessageBoxButtons.YesNoCancel,
                                             MessageBoxIcon.Question
                                            );
                if (result == DialogResult.OK || result == DialogResult.Yes)
                {
                    // Save and close
                    SaveScenes();
                }
                else if (result == DialogResult.Cancel || result == DialogResult.Abort)
                {
                    // Cancel closing
                    return true;
                }
            }

            ClearRefsToSceneObjects();

            return false;
        }

        /// <summary>
        /// Clears references to the scene objects by the editor. Deselects objects.
        /// </summary>
        /// <param name="fullCleanup">True if cleanup all data (including serialized and cached data). Otherwise will just clear living references to the scene objects.</param>
        public void ClearRefsToSceneObjects(bool fullCleanup = false)
        {
            Editor.SceneEditing.Deselect();

            if (fullCleanup)
            {
                Undo.Clear();
            }
        }

        private void OnSceneLoaded(Scene scene, Guid sceneId)
        {
            var startTime = DateTime.UtcNow;

            // Build scene tree
            var sceneNode = SceneGraphFactory.BuildSceneTree(scene);
            var treeNode = sceneNode.TreeNode;
            treeNode.IsLayoutLocked = true;
            treeNode.Expand(true);

            // Add to the tree
            var rootNode = Root.TreeNode;
            rootNode.IsLayoutLocked = true;
            //
            sceneNode.ParentNode = Root;
            rootNode.SortChildren();
            //
            treeNode.UnlockChildrenRecursive();
            rootNode.IsLayoutLocked = false;
            rootNode.Parent.PerformLayout();

            var endTime = DateTime.UtcNow;
            var milliseconds = (int)(endTime - startTime).TotalMilliseconds;
            Editor.Log($"Created graph for scene \'{scene.Name}\' in {milliseconds} ms");
        }

        private void OnSceneUnloading(Scene scene, Guid sceneId)
        {
            // Find scene tree node
            var node = Root.FindChildActor(scene);
            if (node != null)
            {
                Editor.Log($"Cleanup graph for scene \'{scene.Name}\'");

                // Cleanup
                node.Dispose();
            }
        }

        private void OnActorSpawned(Actor actor)
        {
            // Skip for not loaded scenes (spawning actors during scene loading in script Start function)
            var sceneNode = GetActorNode(actor.Scene);
            if (sceneNode == null)
                return;

            // Skip for missing parent
            var parent = actor.Parent;
            if (parent == null)
                return;

            var parentNode = GetActorNode(parent);
            if (parentNode == null)
            {
                // Missing parent node when adding child actor to not spawned or unlinked actor
                return;
            }

            var node = SceneGraphFactory.BuildActorNode(actor);
            if (node != null)
            {
                node.TreeNode.UnlockChildrenRecursive();
                node.ParentNode = parentNode;
            }
        }

        private void OnActorDeleted(Actor actor)
        {
            var node = GetActorNode(actor);
            if (node != null)
            {
                OnActorDeleted(node);
            }
        }

        private void OnActorDeleted(ActorNode node)
        {
            for (int i = 0; i < node.ChildNodes.Count; i++)
            {
                if (node.ChildNodes[i] is ActorNode child)
                {
                    i--;
                    OnActorDeleted(child);
                }
            }

            ActorRemoved?.Invoke(node);

            // Cleanup part of the graph
            node.Dispose();
        }

        private void OnActorParentChanged(Actor actor, Actor prevParent)
        {
            ActorNode node = null;
            var parentNode = GetActorNode(actor.Parent);

            // Try use previous parent actor to find actor node
            var prevParentNode = GetActorNode(prevParent);
            if (prevParentNode != null)
            {
                // If should be one of the children
                node = prevParentNode.FindChildActor(actor);

                // Search whole tree if node was not found
                if (node == null)
                {
                    node = GetActorNode(actor);
                }
            }
            else if (parentNode != null)
            {
                // Create new node for that actor (user may unlink it from the scene before and now link it)
                node = SceneGraphFactory.BuildActorNode(actor);
            }
            if (node == null)
                return;

            // Get the new parent node (may be missing)
            if (parentNode != null)
            {
                // Change parent
                node.TreeNode.UnlockChildrenRecursive();
                node.ParentNode = parentNode;
            }
            else
            {
                // Check if actor is selected in editor
                if (Editor.SceneEditing.Selection.Contains(node))
                    Editor.SceneEditing.Deselect();

                // Remove node (user may unlink actor from the scene but not destroy the actor)
                node.Dispose();
            }
        }

        private void OnActorOrderInParentChanged(Actor actor)
        {
            ActorNode node = GetActorNode(actor);
            node?.TreeNode.OnOrderInParentChanged();
        }

        private void OnActorNameChanged(Actor actor)
        {
            ActorNode node = GetActorNode(actor);
            node?.TreeNode.OnNameChanged();
        }

        private void OnActorActiveChanged(Actor actor)
        {
            //ActorNode node = GetActorNode(actor);
            //node?.TreeNode.OnActiveChanged();
        }

        /// <summary>
        /// Gets the actor node.
        /// </summary>
        /// <param name="actor">The actor.</param>
        /// <returns>Found actor node or null if missing. Actor may not be linked to the scene tree so node won't be found in that case.</returns>
        public ActorNode GetActorNode(Actor actor)
        {
            if (actor == null)
                return null;

            // ActorNode has the same ID as actor does
            return SceneGraphFactory.FindNode(actor.ID) as ActorNode;
        }

        /// <summary>
        /// Gets the actor node.
        /// </summary>
        /// <param name="actorId">The actor id.</param>
        /// <returns>Found actor node or null if missing. Actor may not be linked to the scene tree so node won't be found in that case.</returns>
        public ActorNode GetActorNode(Guid actorId)
        {
            // ActorNode has the same ID as actor does
            return SceneGraphFactory.FindNode(actorId) as ActorNode;
        }

        /// <summary>
        /// Executes the custom action on the graph nodes.
        /// </summary>
        /// <param name="callback">The callback.</param>
        public void ExecuteOnGraph(SceneGraphTools.GraphExecuteCallbackDelegate callback)
        {
            Root.ExecuteOnGraph(callback);
        }

        /// <inheritdoc />
        public override void OnInit()
        {
            Root = new ScenesRootNode();

            // Bind events
            Level.SceneLoaded += OnSceneLoaded;
            Level.SceneUnloading += OnSceneUnloading;
            Level.ActorSpawned += OnActorSpawned;
            Level.ActorDeleted += OnActorDeleted;
            Level.ActorParentChanged += OnActorParentChanged;
            Level.ActorOrderInParentChanged += OnActorOrderInParentChanged;
            Level.ActorNameChanged += OnActorNameChanged;
            Level.ActorActiveChanged += OnActorActiveChanged;
        }

        /// <inheritdoc />
        public override void OnExit()
        {
            // Unbind events
            Level.SceneLoaded -= OnSceneLoaded;
            Level.SceneUnloading -= OnSceneUnloading;
            Level.ActorSpawned -= OnActorSpawned;
            Level.ActorDeleted -= OnActorDeleted;
            Level.ActorParentChanged -= OnActorParentChanged;
            Level.ActorOrderInParentChanged -= OnActorOrderInParentChanged;
            Level.ActorNameChanged -= OnActorNameChanged;
            Level.ActorActiveChanged -= OnActorActiveChanged;

            // Cleanup graph
            Root.Dispose();

            if (SceneGraphFactory.Nodes.Count > 0)
            {
                Editor.LogWarning("Not all scene graph nodes has been disposed!");
            }
        }
    }
}
