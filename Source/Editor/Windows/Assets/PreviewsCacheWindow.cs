// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEditor.Content;
using FlaxEditor.Viewport.Previews;
using FlaxEngine.GUI;

namespace FlaxEditor.Windows.Assets
{
    /// <summary>
    /// Editor window that allows to view <see cref="PreviewsCache"/> asset.
    /// </summary>
    /// <seealso cref="PreviewsCache" />
    /// <seealso cref="FlaxEditor.Windows.Assets.AssetEditorWindow" />
    public sealed class PreviewsCacheWindow : AssetEditorWindowBase<PreviewsCache>
    {
        private readonly TexturePreview _preview;

        /// <inheritdoc />
        public PreviewsCacheWindow(Editor editor, AssetItem item)
        : base(editor, item)
        {
            // Texture preview
            _preview = new TexturePreview(true)
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = new Margin(0, 0, _toolstrip.Bottom, 0),
                Parent = this
            };

            // Toolstrip
            _toolstrip.AddButton(editor.Icons.CenterView64, _preview.CenterView).LinkTooltip("Center view");
        }

        /// <inheritdoc />
        protected override void UnlinkItem()
        {
            _preview.Asset = null;

            base.UnlinkItem();
        }

        /// <inheritdoc />
        protected override void OnAssetLinked()
        {
            _preview.Asset = _asset;

            base.OnAssetLinked();
        }
    }
}
