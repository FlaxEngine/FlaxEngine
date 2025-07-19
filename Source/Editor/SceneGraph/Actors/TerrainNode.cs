// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using FlaxEngine;

namespace FlaxEditor.SceneGraph.Actors
{
    /// <summary>
    /// Scene tree node for <see cref="Terrain"/> actor type.
    /// </summary>
    [HideInEditor]
    public sealed class TerrainNode : ActorNode
    {
        private struct RemovedTerrain
        {
            public Guid SceneId;
            public Guid TerrainId;
            public string[] Files;
        }

        private static List<RemovedTerrain> _cleanupFiles;

        /// <inheritdoc />
        public TerrainNode(Actor actor)
        : base(actor)
        {
        }

        /// <inheritdoc />
        public override bool AffectsNavigation => true;

        /// <inheritdoc />
        public override void Delete()
        {
            // Schedule terrain data files for automatic cleanup
            if (_cleanupFiles == null)
            {
                _cleanupFiles = new List<RemovedTerrain>();
                Engine.RequestingExit += OnRequestingExit;
            }
            if (Actor is Terrain terrain)
            {
                var removed = new RemovedTerrain
                {
                    SceneId = terrain.Scene?.ID ?? Guid.Empty,
                    TerrainId = terrain.ID,
                    Files = new string[4],
                };
                for (int i = 0; i < terrain.PatchesCount; i++)
                {
                    var patch = terrain.GetPatch(i);
                    if (patch.Heightmap)
                        removed.Files[0] = patch.Heightmap.Path;
                    if (patch.Heightfield)
                        removed.Files[1] = patch.Heightfield.Path;
                    if (patch.GetSplatmap(0))
                        removed.Files[2] = patch.GetSplatmap(0).Path;
                    if (patch.GetSplatmap(1))
                        removed.Files[3] = patch.GetSplatmap(1).Path;
                }
                _cleanupFiles.Add(removed);
            }

            base.Delete();
        }

        void OnRequestingExit()
        {
            foreach (var e in _cleanupFiles)
            {
                try
                {
                    // Skip removing this terrain file sif it's still referenced
                    var sceneReferences = Editor.GetAssetReferences(e.SceneId);
                    if (sceneReferences != null && sceneReferences.Contains(e.TerrainId))
                    {
                        Debug.Log($"Skip removing files used by terrain {e.TerrainId} on scene {e.SceneId} as it's still in use");
                        continue;
                    }

                    // Delete files
                    Debug.Log($"Removing files used by removed terrain {e.TerrainId} on scene {e.SceneId}");
                    foreach (var file in e.Files)
                    {
                        if (file != null && File.Exists(file))
                            File.Delete(file);
                    }
                }
                catch (Exception ex)
                {
                    Editor.LogWarning(ex);
                }
            }
            _cleanupFiles.Clear();
            _cleanupFiles = null;
            Engine.RequestingExit -= OnRequestingExit;
        }
    }
}
