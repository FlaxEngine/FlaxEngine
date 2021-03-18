// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.Windows;
using FlaxEngine;

namespace FlaxEditor.Content
{
    /// <summary>
    /// A <see cref="Shader"/> asset proxy object.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.BinaryAssetProxy" />
    public sealed class ShaderProxy : BinaryAssetProxy
    {
        /// <inheritdoc />
        public override string Name => "Shader";

        /// <inheritdoc />
        public override bool CanReimport(ContentItem item)
        {
            return true;
        }

        /// <inheritdoc />
        public override EditorWindow Open(Editor editor, ContentItem item)
        {
            var assetItem = (BinaryAssetItem)item;
            var asset = FlaxEngine.Content.Load<Shader>(assetItem.ID);
            if (asset)
            {
                var source = Editor.GetShaderSourceCode(asset);
                Utilities.Utils.ShowSourceCodeWindow(source, "Shader Source", item.RootWindow.Window);
            }
            return null;
        }

        /// <inheritdoc />
        public override Color AccentColor => Color.FromRGB(0x7542f5);

        /// <inheritdoc />
        public override Type AssetType => typeof(Shader);
    }
}
