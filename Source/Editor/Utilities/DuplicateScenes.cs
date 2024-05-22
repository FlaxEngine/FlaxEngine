// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using FlaxEngine;

namespace FlaxEditor.Utilities
{
    /// <summary>
    /// Tool helper class used to duplicate loaded scenes (backup them) and restore later. Used for play in-editor functionality.
    /// </summary>
    public class DuplicateScenes
    {
        private struct SceneData
        {
            public bool IsDirty;
            public byte[] Bytes;
        }

        private readonly List<SceneData> _scenesData = new List<SceneData>(16);

        /// <summary>
        /// Checks if scene data has been gathered.
        /// </summary>
        public bool HasData => _scenesData.Count > 0;

        /// <summary>
        /// Gets a value indicating whether any scene was dirty before gathering.
        /// </summary>
        public bool WasDirty => _scenesData.Any(x => x.IsDirty);

        /// <summary>
        /// Collect loaded scenes data.
        /// </summary>
        public void GatherSceneData()
        {
            if (HasData)
                throw new InvalidOperationException("DuplicateScenes has already gathered scene data.");
            Profiler.BeginEvent("DuplicateScenes.GatherSceneData");

            Editor.Log("Collecting scene data");

            // Get loaded scenes
            var scenes = Level.Scenes;
            int scenesCount = scenes.Length;
            if (scenesCount == 0)
            {
                Profiler.EndEvent();
                throw new InvalidOperationException("Cannot gather scene data. No scene loaded.");
            }
            var sceneIds = new Guid[scenesCount];
            for (int i = 0; i < scenesCount; i++)
            {
                sceneIds[i] = scenes[i].ID;
            }

            // Serialize scenes
            _scenesData.Capacity = scenesCount;
            for (int i = 0; i < scenesCount; i++)
            {
                var scene = scenes[i];
                var data = new SceneData
                {
                    IsDirty = Editor.Instance.Scene.IsEdited(scene),
                    Bytes = Level.SaveSceneToBytes(scene, false),
                };
                _scenesData.Add(data);
            }

            // Delete old scenes
            if (Level.UnloadAllScenes())
            {
                Profiler.EndEvent();
                throw new Exception("Failed to unload scenes.");
            }
            FlaxEngine.Scripting.FlushRemovedObjects();

            // Ensure that old scenes has been unregistered
            {
                var noScenes = Level.Scenes;
                if (noScenes != null && noScenes.Length != 0)
                {
                    Profiler.EndEvent();
                    throw new Exception("Failed to unload scenes.");
                }
            }

            Editor.Log(string.Format("Gathered {0} scene(s)!", scenesCount));
            Profiler.EndEvent();
        }

        /// <summary>
        /// Deserialize captured scenes.
        /// </summary>
        public void CreateScenes()
        {
            if (!HasData)
                throw new InvalidOperationException("DuplicateScenes has not gathered scene data yet.");
            Profiler.BeginEvent("DuplicateScenes.CreateScenes");

            Editor.Log("Creating scenes");

            // Deserialize new scenes
            int scenesCount = _scenesData.Count;
            for (int i = 0; i < scenesCount; i++)
            {
                var data = _scenesData[i];
                var scene = Level.LoadSceneFromBytes(data.Bytes);
                if (scene == null)
                {
                    Profiler.EndEvent();
                    throw new Exception("Failed to deserialize scene");
                }
            }

            Profiler.EndEvent();
        }

        /// <summary>
        /// Deletes the creates scenes for the simulation.
        /// </summary>
        public void UnloadScenes()
        {
            Profiler.BeginEvent("DuplicateScenes.UnloadScenes");
            Editor.Log("Restoring scene data");

            // TODO: here we can keep changes for actors marked to keep their state after simulation

            // Delete new scenes
            if (Level.UnloadAllScenes())
            {
                Profiler.EndEvent();
                throw new Exception("Failed to unload scenes.");
            }
            FlaxEngine.Scripting.FlushRemovedObjects();
            Editor.WipeOutLeftoverSceneObjects();

            Profiler.EndEvent();
        }

        /// <summary>
        /// Restore captured scene data.
        /// </summary>
        public void RestoreSceneData()
        {
            if (!HasData)
                throw new InvalidOperationException("DuplicateScenes has not gathered scene data yet.");
            Profiler.BeginEvent("DuplicateScenes.RestoreSceneData");

            // Deserialize old scenes
            for (int i = 0; i < _scenesData.Count; i++)
            {
                var data = _scenesData[i];
                var scene = Level.LoadSceneFromBytes(data.Bytes);
                if (scene == null)
                {
                    Editor.LogError("Failed to restore scene");
                    continue;
                }

                // Restore `dirty` state
                if (data.IsDirty)
                    Editor.Instance.Scene.MarkSceneEdited(scene);
            }
            _scenesData.Clear();

            Editor.Log("Restored previous scenes");
            Profiler.EndEvent();
        }
    }
}
