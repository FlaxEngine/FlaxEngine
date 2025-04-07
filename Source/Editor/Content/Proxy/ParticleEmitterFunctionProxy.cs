// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.Windows;
using FlaxEditor.Windows.Assets;
using FlaxEngine;

namespace FlaxEditor.Content
{
    /// <summary>
    /// A <see cref="ParticleEmitterFunction"/> asset proxy object.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.BinaryAssetProxy" />
    [ContentContextMenu("New/Particles/Particle Emitter Function")]
    public class ParticleEmitterFunctionProxy : BinaryAssetProxy
    {
        /// <inheritdoc />
        public override string Name => "Particle Emitter Function";

        /// <inheritdoc />
        public override EditorWindow Open(Editor editor, ContentItem item)
        {
            return new ParticleEmitterFunctionWindow(editor, item as AssetItem);
        }

        /// <inheritdoc />
        public override Color AccentColor => Color.FromRGB(0x1795a3);

        /// <inheritdoc />
        public override Type AssetType => typeof(ParticleEmitterFunction);

        /// <inheritdoc />
        public override bool CanCreate(ContentFolder targetLocation)
        {
            return targetLocation.CanHaveAssets;
        }

        /// <inheritdoc />
        public override void Create(string outputPath, object arg)
        {
            if (Editor.CreateAsset("ParticleEmitterFunction", outputPath))
                throw new Exception("Failed to create new asset.");
        }
    }
}
