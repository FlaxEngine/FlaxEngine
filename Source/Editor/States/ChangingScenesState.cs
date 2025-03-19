// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using FlaxEngine;
using FlaxEngine.Assertions;
using FlaxEngine.Utilities;

namespace FlaxEditor.States
{
    /// <summary>
    /// In this state editor is changing loaded scenes collection.
    /// </summary>
    /// <seealso cref="FlaxEditor.States.EditorState" />
    [HideInEditor]
    public sealed class ChangingScenesState : EditorState
    {
        private readonly List<Guid> _scenesToLoad = new List<Guid>();
        private readonly List<Scene> _scenesToUnload = new List<Scene>();
        private Guid _lastSceneFromRequest;

        internal ChangingScenesState(Editor editor)
        : base(editor)
        {
        }

        /// <inheritdoc />
        public override string Status => "Loading scene...";

        /// <summary>
        /// Loads the scene.
        /// </summary>
        /// <param name="sceneId">The scene asset ID.</param>
        /// <param name="additive">True if don't close opened scenes and just add new scene to the collection, otherwise will release current scenes and load single one.</param>
        public void LoadScene(Guid sceneId, bool additive = false)
        {
            // Clear request
            _scenesToLoad.Clear();
            _scenesToUnload.Clear();

            // Setup request
            _scenesToLoad.Add(sceneId);
            if (!additive)
            {
                _scenesToUnload.AddRange(Level.Scenes);
            }

            TryEnter();
        }

        /// <summary>
        /// Unloads the scene.
        /// </summary>
        /// <param name="scene">The scene to unload.</param>
        public void UnloadScene(Scene scene)
        {
            if (scene == null)
                throw new ArgumentNullException();

            // Clear request
            _scenesToLoad.Clear();
            _scenesToUnload.Clear();

            // Setup request
            _scenesToUnload.Add(scene);

            TryEnter();
        }

        /// <summary>
        /// Unloads the scenes collection.
        /// </summary>
        /// <param name="scenes">The scenes to unload.</param>
        public void UnloadScene(IEnumerable<Scene> scenes)
        {
            if (scenes == null)
                throw new ArgumentNullException();
            if (!scenes.Any())
                return;

            // Clear request
            _scenesToLoad.Clear();
            _scenesToUnload.Clear();

            // Setup request
            _scenesToUnload.AddRange(scenes);

            TryEnter();
        }

        /// <summary>
        /// Changes the scenes.
        /// </summary>
        /// <param name="toLoad">Scenes to load.</param>
        /// <param name="toUnload">Scenes to unload.</param>
        public void ChangeScenes(IEnumerable<Guid> toLoad, IEnumerable<Scene> toUnload)
        {
            // Clear request
            _scenesToLoad.Clear();
            _scenesToUnload.Clear();

            // Setup request
            _scenesToLoad.AddRange(toLoad);
            _scenesToUnload.AddRange(toUnload);

            TryEnter();
        }

        private void TryEnter()
        {
            // Reload existing scene if only 1 scene exists
            bool reloadExistingScene = false;
            if (Level.Scenes.Length == 1 && _scenesToLoad.Count == 1)
            {
                if (Level.FindScene(_scenesToLoad[0]))
                {
                    reloadExistingScene = true;
                }
            }

            if (!reloadExistingScene)
            {
                // Remove redundant scene changes
                for (int i = 0; i < _scenesToUnload.Count; i++)
                {
                    var id = _scenesToUnload[i].ID;
                    for (int j = 0; j < _scenesToLoad.Count; j++)
                    {
                        if (_scenesToLoad[j] == id)
                        {
                            _scenesToLoad.RemoveAt(j--);
                            _scenesToUnload.RemoveAt(i);
                            break;
                        }
                    }
                }

                // Skip already loaded scenes
                for (int j = 0; j < _scenesToLoad.Count; j++)
                {
                    if (Level.FindScene(_scenesToLoad[j]))
                    {
                        _scenesToLoad.RemoveAt(j--);
                        if (_scenesToLoad.Count == 0)
                            break;
                    }
                }
            }

            // Check if won't change anything
            if (_scenesToLoad.Count + _scenesToUnload.Count == 0)
                return;

            // Request state change
            StateMachine.GoToState(this);
        }

        /// <inheritdoc />
        public override bool CanEditContent => false;

        /// <inheritdoc />
        public override void OnEnter()
        {
            Assert.AreEqual(Guid.Empty, _lastSceneFromRequest, "Invalid state.");

            // Bind events
            Level.SceneLoaded += OnSceneEvent;
            Level.SceneLoadError += OnSceneEvent;
            Level.SceneUnloaded += OnSceneEvent;

            // Push scenes changing requests
            for (int i = 0; i < _scenesToUnload.Count; i++)
            {
                _lastSceneFromRequest = _scenesToUnload[i].ID;
                Level.UnloadSceneAsync(_scenesToUnload[i]);
            }
            for (int i = 0; i < _scenesToLoad.Count; i++)
            {
                var id = _scenesToLoad[i];
                bool failed = Level.LoadSceneAsync(id);
                if (!failed)
                    _lastSceneFromRequest = id;
            }

            // Clear request
            _scenesToLoad.Clear();
            _scenesToUnload.Clear();

            // Spacial case when all scenes are unloaded and no scene loaded we won't be able too handle OnSceneEvent so just leave state now
            // It may be caused when scripts are not loaded due to compilation errors or cannot find scene asset or other internal engine.
            if (_lastSceneFromRequest == Guid.Empty)
            {
                Editor.LogWarning("Cannot perform scene change");
                StateMachine.GoToState<EditingSceneState>();
            }
        }

        /// <inheritdoc />
        public override void OnExit(State nextState)
        {
            // Validate (but skip if next state is exit)
            if (!(nextState is ClosingState))
            {
                Assert.AreEqual(Guid.Empty, _lastSceneFromRequest, "Invalid state.");
            }
            else
            {
                _lastSceneFromRequest = Guid.Empty;
            }

            // Unbind events
            Level.SceneLoaded -= OnSceneEvent;
            Level.SceneLoadError -= OnSceneEvent;
            Level.SceneUnloaded -= OnSceneEvent;
        }

        private void OnSceneEvent(Scene scene, Guid sceneId)
        {
            // Check if it's scene from the last request
            if (sceneId == _lastSceneFromRequest)
            {
                _lastSceneFromRequest = Guid.Empty;
                StateMachine.GoToState<EditingSceneState>();
            }
        }
    }
}
