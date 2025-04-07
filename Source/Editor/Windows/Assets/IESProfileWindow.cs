// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEditor.Content;
using FlaxEditor.Viewport.Previews;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Windows.Assets
{
    /// <summary>
    /// Editor window to view/modify <see cref="IESProfile"/> asset.
    /// </summary>
    /// <seealso cref="FlaxEditor.Windows.Assets.AssetEditorWindow" />
    public sealed class IESProfileWindow : AssetEditorWindowBase<IESProfile>
    {
        private readonly IESProfilePreview _preview;

        /// <inheritdoc />
        public IESProfileWindow(Editor editor, AssetItem item)
        : base(editor, item)
        {
            // IES Profile preview
            _preview = new IESProfilePreview
            {
                Offsets = new Margin(0, 0, _toolstrip.Bottom, 0),
                Parent = this
            };

            // Toolstrip
            _toolstrip.AddButton(editor.Icons.Import64, () => Editor.ContentImporting.Reimport((BinaryAssetItem)Item)).LinkTooltip("Reimport");
            _toolstrip.AddSeparator();
            _toolstrip.AddButton(editor.Icons.CenterView64, _preview.CenterView).LinkTooltip("Center view");
        }

        /// <inheritdoc />
        protected override void UnlinkItem()
        {
            _preview.Asset = null;

            base.UnlinkItem();
        }

        /// <inheritdoc />
        protected override void OnAssetLoaded()
        {
            _preview.Asset = _asset;

            base.OnAssetLoaded();
        }
    }
}
