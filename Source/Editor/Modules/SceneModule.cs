// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using FlaxEditor.SceneGraph;
using FlaxEditor.SceneGraph.Actors;
using FlaxEngine;
using FlaxEngine.GUI;
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
            private readonly Editor _editor;

            /// <inheritdoc />
            public ScenesRootNode()
            {
                _editor = Editor.Instance;
            }

            /// <inheritdoc />
            public override void Spawn(Actor actor, Actor parent, int orderInParent = -1)
            {
                _editor.SceneEditing.Spawn(actor, parent, orderInParent);
            }

            /// <inheritdoc />
            public override Undo Undo => Editor.Instance.Undo;

            /// <inheritdoc />
            public override ISceneEditingContext SceneContext => _editor.Windows.EditWin;
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
            InitOrder = -800;
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
        /// <returns><c>true</c> if the specified scene is edited; otherwise, <c>false</c>.</returns>
        public bool IsEdited(Scene scene)
        {
            var node = GetActorNode(scene) as SceneNode;
            return node?.IsEdited ?? false;
        }

        /// <summary>
        /// Determines whether any scene is edited.
        /// </summary>
        /// <returns><c>true</c> if any scene is edited; otherwise, <c>false</c>.</returns>
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
        /// <returns><c>true</c> if every scene is edited; otherwise, <c>false</c>.</returns>
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
            floor.Scale = new Float3(4, 0.5f, 4);
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
            //
            var audioListener = cam.AddChild<AudioListener>();
            audioListener.Name = "Audio Listener";

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
            Editor.UI.AddStatusMessage("Saved!");
        }

        /// <summary>
        /// Opens scene (async).
        /// </summary>
        /// <param name="sceneId">Scene ID</param>
        /// <param name="additive">True if don't close opened scenes and just add new scene to them, otherwise will release current scenes and load single one.</param>
        public void OpenScene(Guid sceneId, bool additive = false)
        {
            if (!Editor.StateMachine.CurrentState.CanChangeScene)
                return;

            // In play-mode Editor mocks the level streaming script
            if (Editor.IsPlayMode)
            {
                if (!additive)
                    Level.UnloadAllScenesAsync();
                Level.LoadSceneAsync(sceneId);
                return;
            }

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
        /// Reload all loaded scenes.
        /// </summary>
        public void ReloadScenes()
        {
            if (!Editor.StateMachine.CurrentState.CanChangeScene)
                return;

            if (!Editor.IsPlayMode)
            {
                if (CheckSaveBeforeClose())
                    return;
            }

            // Reload scenes
            foreach (var scene in Level.Scenes)
            {
                var sceneId = scene.ID;
                Level.UnloadScene(scene);
                Level.LoadScene(sceneId);
            }
        }

        /// <summary>
        /// Closes scene (async).
        /// </summary>
        /// <param name="scene">The scene.</param>
        public void CloseScene(Scene scene)
        {
            if (!Editor.StateMachine.CurrentState.CanChangeScene)
                return;

            // In play-mode Editor mocks the level streaming script
            if (Editor.IsPlayMode)
            {
                Level.UnloadSceneAsync(scene);
                return;
            }

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
            if (!Editor.StateMachine.CurrentState.CanChangeScene)
                return;

            // In play-mode Editor mocks the level streaming script
            if (Editor.IsPlayMode)
            {
                Level.UnloadAllScenesAsync();
                return;
            }

            // Ensure to save all pending changes
            if (CheckSaveBeforeClose())
                return;

            // Unload scenes
            Editor.StateMachine.ChangingScenesState.UnloadScene(Level.Scenes);
        }

        /// <summary>
        /// Closes all of the scenes except for the specified scene (async).
        /// </summary>
        /// <param name="scene">The scene to not close.</param>
        public void CloseAllScenesExcept(Scene scene)
        {
            if (!Editor.StateMachine.CurrentState.CanChangeScene)
                return;

            var scenes = new List<Scene>();
            foreach (var s in Level.Scenes)
            {
                if (s == scene)
                    continue;
                scenes.Add(s);
            }

            // In play-mode Editor mocks the level streaming script
            if (Editor.IsPlayMode)
            {
                foreach (var s in scenes)
                {
                    Level.UnloadSceneAsync(s);
                }
                return;
            }

            // Ensure to save all pending changes
            if (CheckSaveBeforeClose())
                return;

            // Unload scenes
            Editor.StateMachine.ChangingScenesState.UnloadScene(scenes);
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
            Profiler.BeginEvent("SceneModule.ClearRefsToSceneObjects");
            Editor.SceneEditing.Deselect();

            if (fullCleanup)
            {
                Undo.Clear();
            }
            Profiler.EndEvent();
        }

        private Dictionary<ContainerControl, Float2> _uiRootSizes;

        internal void OnSaveStart(ContainerControl uiRoot)
        {
            // Force viewport UI to have fixed size during scene/prefabs saving to result in stable data (less mess in version control diffs)
            if (_uiRootSizes == null)
                _uiRootSizes = new Dictionary<ContainerControl, Float2>();
            _uiRootSizes[uiRoot] = uiRoot.Size;
            uiRoot.Size = new Float2(1920, 1080);
        }

        internal void OnSaveEnd(ContainerControl uiRoot)
        {
            // Restore cached size of the UI root container
            if (_uiRootSizes != null && _uiRootSizes.Remove(uiRoot, out var size))
            {
                uiRoot.Size = size;
            }
        }

        private void OnSceneSaving(Scene scene, Guid sceneId)
        {
            OnSaveStart(RootControl.GameRoot);
        }
        
        private void OnSceneSaved(Scene scene, Guid sceneId)
        {
            OnSaveEnd(RootControl.GameRoot);
        }
        
        private void OnSceneSaveError(Scene scene, Guid sceneId)
        {
            OnSaveEnd(RootControl.GameRoot);
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
            sceneNode.ParentNode = Root;
            rootNode.SortChildren();
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
                var selection = Editor.SceneEditing.Selection;
                var hasSceneSelection = false;
                for (int i = 0; i < selection.Count; i++)
                {
                    if (selection[i].ParentScene == node)
                    {
                        hasSceneSelection = true;
                        break;
                    }
                }
                if (hasSceneSelection)
                {
                    var newSelection = new List<SceneGraphNode>();
                    for (int i = 0; i < selection.Count; i++)
                    {
                        if (selection[i].ParentScene != node)
                            newSelection.Add(selection[i]);
                    }
                    Editor.SceneEditing.Select(newSelection);
                }
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
            node.ParentNode = parentNode;
            if (parentNode == null)
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
            node?.TreeNode.UpdateText();
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
            Level.SceneSaving += OnSceneSaving;
            Level.SceneSaved += OnSceneSaved;
            Level.SceneSaveError += OnSceneSaveError;
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
            Level.SceneSaving -= OnSceneSaving;
            Level.SceneSaved -= OnSceneSaved;
            Level.SceneSaveError -= OnSceneSaveError;
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
