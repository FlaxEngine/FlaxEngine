// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.Windows;
using FlaxEditor.Windows.Assets;
using FlaxEngine;

namespace FlaxEditor.Content
{
    /// <summary>
    /// Implementation of <see cref="BinaryAssetItem"/> for <see cref="SceneAnimation"/> assets.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.BinaryAssetItem" />
    class SceneAnimationItem : BinaryAssetItem
    {
        /// <inheritdoc />
        public SceneAnimationItem(string path, ref Guid id, string typeName, Type type)
        : base(path, ref id, typeName, type, ContentItemSearchFilter.Other)
        {
        }

        /// <inheritdoc />
        public override bool OnEditorDrag(object context)
        {
            return true;
        }

        /// <inheritdoc />
        public override Actor OnEditorDrop(object context)
        {
            return new SceneAnimationPlayer { Animation = FlaxEngine.Content.LoadAsync<SceneAnimation>(ID) };
        }
    }

    /// <summary>
    /// A <see cref="SceneAnimation"/> asset proxy object.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.BinaryAssetProxy" />
    [ContentContextMenu("New/Animation/Scene Animation")]
    public class SceneAnimationProxy : BinaryAssetProxy
    {
        /// <inheritdoc />
        public override string Name => "Scene Animation";

        /// <inheritdoc />
        public override EditorWindow Open(Editor editor, ContentItem item)
        {
            return new SceneAnimationWindow(editor, item as AssetItem);
        }

        /// <inheritdoc />
        public override AssetItem ConstructItem(string path, string typeName, ref Guid id)
        {
            return new SceneAnimationItem(path, ref id, typeName, AssetType);
        }

        /// <inheritdoc />
        public override Color AccentColor => Color.FromRGB(0xff5c4a87);

        /// <inheritdoc />
        public override Type AssetType => typeof(SceneAnimation);

        /// <inheritdoc />
        public override bool CanCreate(ContentFolder targetLocation)
        {
            return targetLocation.CanHaveAssets;
        }

        /// <inheritdoc />
        public override void Create(string outputPath, object arg)
        {
            if (Editor.CreateAsset("SceneAnimation", outputPath))
                throw new Exception("Failed to create new asset.");
        }
    }
}
