// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using System;
using System.IO;
using System.Runtime;
using FlaxEditor.Content.Thumbnails;
using FlaxEditor.Windows;
using FlaxEditor.Windows.Assets;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Content
{
    /// <summary>
    /// A <see cref="UIBlueprintAsset"/> asset proxy object.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.UIProxy" />
    [ContentContextMenu("New/UI/GameUI")]
    public class UIBlueprintProxy : SpawnableJsonAssetProxy<UIBlueprintAsset>
    {
        /// <inheritdoc/>
        public override string TypeName => typeof(UIBlueprintAsset).FullName;
        /// <inheritdoc/>
        public override string Name => typeof(UIBlueprintAsset).Name;
        /// <inheritdoc/>
        //public override string FileExtension => "";
        /// <inheritdoc/>
        public override Color AccentColor => Color.Aqua;
        /// <inheritdoc/>
        public override AssetItem ConstructItem(string path, string typeName, ref Guid id)
        {
            return new UIBlueprintAssetItem(path, id, typeName);
        }
        /// <inheritdoc/>
        public override bool IsProxyFor(ContentItem item)
        {
            return item is UIBlueprintAssetItem AssetItem && TypeName == AssetItem.TypeName;
        }
        /// <inheritdoc/>
        public override EditorWindow Open(Editor editor, ContentItem item)
        {
            return new UIBlueprintEditorWindow(editor, item as UIBlueprintAssetItem);
        }
        /// <inheritdoc />
        public override bool CanCreate(ContentFolder targetLocation)
        {
            return targetLocation.CanHaveAssets;
        }
        /// <inheritdoc />
        public override void Create(string outputPath, object arg)
        {
            FlaxEditor.Editor.SaveJsonAsset(outputPath, new UIBlueprintAsset());
        }
    }
}
