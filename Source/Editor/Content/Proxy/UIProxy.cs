// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using System;
using System.IO;
using FlaxEditor.Content.Thumbnails;
using FlaxEditor.Windows;
using FlaxEditor.Windows.Assets;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Content
{
    /// <summary>
    /// A <see cref="Widget"/> asset proxy object.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.UIProxy" />
    [ContentContextMenu("New/UI/GameUI")]
    public class UIProxy : AssetProxy
    {
        /// <inheritdoc/>
        public override string TypeName => typeof(Widget).FullName;
        /// <inheritdoc/>
        public override string Name => typeof(Widget).Name;
        /// <inheritdoc/>
        public override string FileExtension => "widget";
        /// <inheritdoc/>
        public override Color AccentColor => Color.Aqua;
        /// <inheritdoc/>
        public override AssetItem ConstructItem(string path, string typeName, ref Guid id)
        {
            return new WidgetItem(path, typeName, ref id);
        }
        /// <inheritdoc/>
        public override bool IsProxyFor(ContentItem item)
        {
            return item is WidgetItem AssetItem && TypeName == AssetItem.TypeName;
        }
        /// <inheritdoc/>
        public override EditorWindow Open(Editor editor, ContentItem item)
        {
            return new UIEditorWindow(editor, item as WidgetItem);
        }
        /// <inheritdoc />
        public override bool CanCreate(ContentFolder targetLocation)
        {
            return targetLocation.CanHaveAssets;
        }
        /// <inheritdoc />
        public override void Create(string outputPath, object arg)
        {
            Widget data = FlaxEngine.Content.CreateVirtualAsset<Widget>();
            data.Save(outputPath);
        }
    }
}
