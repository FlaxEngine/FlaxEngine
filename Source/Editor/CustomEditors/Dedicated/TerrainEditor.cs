// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.CustomEditors.Dedicated
{
    /// <summary>
    /// Custom editor for <see cref="Terrain"/>.
    /// </summary>
    /// <seealso cref="ActorEditor" />
    [CustomEditor(typeof(Terrain)), DefaultEditor]
    public class TerrainEditor : ActorEditor
    {
        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            base.Initialize(layout);

            // Add info box
            if (IsSingleObject && Values[0] is Terrain terrain)
            {
                var patchesCount = terrain.PatchesCount;
                var chunkSize = terrain.ChunkSize;
                var resolution = terrain.Scale;
                var totalSize = terrain.Box.Size;
                string text = string.Format("Patches: {0}\nTotal Chunks: {1}\nChunk Size: {2}\nResolution: {3}m x {4}m\nTotal size: {5}km x {6}km",
                                            patchesCount,
                                            patchesCount * 16,
                                            chunkSize,
                                            1.0f / (resolution.X + 1e-9f),
                                            1.0f / (resolution.Z + 1e-9f),
                                            totalSize.X * 0.00001f,
                                            totalSize.Z * 0.00001f
                );
                var label = layout.Label(text);
                label.Label.AutoHeight = true;
            }
        }
    }
}
