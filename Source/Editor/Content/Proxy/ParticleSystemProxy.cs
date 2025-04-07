// Copyright (c) Wojciech Figat. All rights reserved.

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
    /// Implementation of <see cref="BinaryAssetItem"/> for <see cref="ParticleSystem"/> assets.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.BinaryAssetItem" />
    class ParticleSystemItem : BinaryAssetItem
    {
        /// <inheritdoc />
        public ParticleSystemItem(string path, ref Guid id, string typeName, Type type)
        : base(path, ref id, typeName, type, ContentItemSearchFilter.Particles)
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
            return new ParticleEffect { ParticleSystem = FlaxEngine.Content.LoadAsync<ParticleSystem>(ID) };
        }
    }

    /// <summary>
    /// A <see cref="ParticleSystem"/> asset proxy object.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.BinaryAssetProxy" />
    [ContentContextMenu("New/Particles/Particle System")]
    public class ParticleSystemProxy : BinaryAssetProxy
    {
        private ParticleSystemPreview _preview;
        private ThumbnailRequest _warmupRequest;

        /// <inheritdoc />
        public override string Name => "Particle System";

        /// <inheritdoc />
        public override EditorWindow Open(Editor editor, ContentItem item)
        {
            return new ParticleSystemWindow(editor, item as AssetItem);
        }

        /// <inheritdoc />
        public override AssetItem ConstructItem(string path, string typeName, ref Guid id)
        {
            return new ParticleSystemItem(path, ref id, typeName, AssetType);
        }

        /// <inheritdoc />
        public override Color AccentColor => Color.FromRGB(0xFF790200);

        /// <inheritdoc />
        public override Type AssetType => typeof(ParticleSystem);

        /// <inheritdoc />
        public override bool CanCreate(ContentFolder targetLocation)
        {
            return targetLocation.CanHaveAssets;
        }

        /// <inheritdoc />
        public override void Create(string outputPath, object arg)
        {
            if (Editor.CreateAsset("ParticleSystem", outputPath))
                throw new Exception("Failed to create new asset.");
        }

        /// <inheritdoc />
        public override void OnThumbnailDrawPrepare(ThumbnailRequest request)
        {
            if (_preview == null)
            {
                _preview = new ParticleEmitterPreview(false);
                InitAssetPreview(_preview);
            }

            // Mark for initial warmup
            request.Tag = 0;
        }

        /// <inheritdoc />
        public override bool CanDrawThumbnail(ThumbnailRequest request)
        {
            var state = (int)request.Tag;
            if (state == 2)
                return true;

            // Allow only one request at once during warmup time
            if (_warmupRequest != null && _warmupRequest != request)
                return false;

            // Ensure assets are ready to be used
            if (!_preview.HasLoadedAssets)
                return false;
            var asset = (ParticleSystem)request.Asset;
            if (!asset.IsLoaded)
                return false;

            if (state == 0)
            {
                // Start the warmup
                _warmupRequest = request;
                request.Tag = 1;
                _preview.System = asset;
            }
            else if (_preview.PreviewActor.Time >= Mathf.Min(0.2f * asset.Duration, 0.6f))
            {
                // End the warmup
                request.Tag = 2;
                _preview.FitIntoView();
                return true;
            }

            // Handle warmup time for the preview
            _preview.PreviewActor.UpdateSimulation();
            return false;
        }

        /// <inheritdoc />
        public override void OnThumbnailDrawBegin(ThumbnailRequest request, ContainerControl guiRoot, GPUContext context)
        {
            _preview.Parent = guiRoot;
            _preview.SyncBackbufferSize();

            _preview.Task.OnDraw();
        }

        /// <inheritdoc />
        public override void OnThumbnailDrawEnd(ThumbnailRequest request, ContainerControl guiRoot)
        {
            if (_warmupRequest == request)
            {
                _warmupRequest = null;
            }

            _preview.System = null;
            _preview.Parent = null;
        }

        /// <inheritdoc />
        public override void OnThumbnailDrawCleanup(ThumbnailRequest request)
        {
            if (_warmupRequest == request)
            {
                _warmupRequest = null;
            }
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
