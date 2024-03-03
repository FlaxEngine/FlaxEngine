// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.Content.Thumbnails;
using FlaxEditor.Viewport.Previews;
using FlaxEditor.Windows;
using FlaxEditor.Windows.Assets;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Content
{
    /// <summary>
    /// A <see cref="SkinnedModel"/> asset proxy object.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.BinaryAssetProxy" />
    public class SkinnedModelProxy : BinaryAssetProxy
    {
        private AnimatedModelPreview _preview;

        /// <inheritdoc />
        public override string Name => "Skinned Model";

        /// <inheritdoc />
        public override bool CanReimport(ContentItem item)
        {
            return true;
        }

        /// <inheritdoc />
        public override EditorWindow Open(Editor editor, ContentItem item)
        {
            return new SkinnedModelWindow(editor, item as AssetItem);
        }

        /// <inheritdoc />
        public override Color AccentColor => Color.FromRGB(0xB30031);

        /// <inheritdoc />
        public override Type AssetType => typeof(SkinnedModel);

        /// <inheritdoc />
        public override void OnThumbnailDrawPrepare(ThumbnailRequest request)
        {
            if (_preview == null)
            {
                _preview = new AnimatedModelPreview(false);
                InitAssetPreview(_preview);
            }

            // TODO: disable streaming for asset during thumbnail rendering (and restore it after)
        }

        /// <inheritdoc />
        public override bool CanDrawThumbnail(ThumbnailRequest request)
        {
            return _preview.HasLoadedAssets && ThumbnailsModule.HasMinimumQuality((SkinnedModel)request.Asset);
        }

        /// <inheritdoc />
        public override void OnThumbnailDrawBegin(ThumbnailRequest request, ContainerControl guiRoot, GPUContext context)
        {
            _preview.SkinnedModel = (SkinnedModel)request.Asset;
            _preview.Parent = guiRoot;
            _preview.SyncBackbufferSize();

            _preview.Task.OnDraw();
        }

        /// <inheritdoc />
        public override void OnThumbnailDrawEnd(ThumbnailRequest request, ContainerControl guiRoot)
        {
            _preview.SkinnedModel = null;
            _preview.Parent = null;
        }

        /// <inheritdoc />
        public override void Dispose()
        {
            if (_preview != null)
            {
                _preview.Dispose();
                _preview = null;
            }

            base.Dispose();
        }
    }
}
