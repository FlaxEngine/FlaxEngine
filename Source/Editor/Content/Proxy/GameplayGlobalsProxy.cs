// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.Windows;
using FlaxEditor.Windows.Assets;
using FlaxEngine;

namespace FlaxEditor.Content
{
    /// <summary>
    /// A <see cref="GameplayGlobals"/> asset proxy object.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.BinaryAssetProxy" />
    [ContentContextMenu("New/Gameplay Globals")]
    public class GameplayGlobalsProxy : BinaryAssetProxy
    {
        /// <inheritdoc />
        public override string Name => "Gameplay Globals";

        /// <inheritdoc />
        public override EditorWindow Open(Editor editor, ContentItem item)
        {
            return new GameplayGlobalsWindow(editor, (AssetItem)item);
        }

        /// <inheritdoc />
        public override Color AccentColor => Color.FromRGB(0xccff33);

        /// <inheritdoc />
        public override Type AssetType => typeof(GameplayGlobals);

        /// <inheritdoc />
        public override bool CanCreate(ContentFolder targetLocation)
        {
            return targetLocation.CanHaveAssets;
        }

        /// <inheritdoc />
        public override void Create(string outputPath, object arg)
        {
            var asset = FlaxEngine.Content.CreateVirtualAsset<GameplayGlobals>();
            if (asset.Save(outputPath))
                throw new Exception("Failed to create new asset.");
            FlaxEngine.Object.Destroy(asset);
        }
    }
}
