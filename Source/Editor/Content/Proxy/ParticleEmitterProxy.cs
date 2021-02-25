// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.Content.Create;
using FlaxEditor.Content.Thumbnails;
using FlaxEditor.Viewport.Previews;
using FlaxEditor.Windows;
using FlaxEditor.Windows.Assets;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Content
{
    /// <summary>
    /// A <see cref="ParticleEmitter"/> asset proxy object.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.BinaryAssetProxy" />
    public class ParticleEmitterProxy : BinaryAssetProxy
    {
        private ParticleEmitterPreview _preview;
        private ThumbnailRequest _warmupRequest;

        /// <inheritdoc />
        public override string Name => "Particle Emitter";

        /// <inheritdoc />
        public override EditorWindow Open(Editor editor, ContentItem item)
        {
            return new ParticleEmitterWindow(editor, item as AssetItem);
        }

        /// <inheritdoc />
        public override Color AccentColor => Color.FromRGB(0xFF79D2B0);

        /// <inheritdoc />
        public override Type AssetType => typeof(ParticleEmitter);

        /// <inheritdoc />
        public override bool CanCreate(ContentFolder targetLocation)
        {
            return targetLocation.CanHaveAssets;
        }

        /// <inheritdoc />
        public override void Create(string outputPath, object arg)
        {
            Editor.Instance.ContentImporting.Create(new ParticleEmitterCreateEntry(outputPath));
        }

        /// <inheritdoc />
        public override void OnThumbnailDrawPrepare(ThumbnailRequest request)
        {
            if (_preview == null)
            {
                _preview = new ParticleEmitterPreview(false)
                {
                    RenderOnlyWithWindow = false,
                    UseAutomaticTaskManagement = false,
                    AnchorPreset = AnchorPresets.StretchAll,
                    Offsets = Margin.Zero,
                };
                _preview.Task.Enabled = false;

                var eyeAdaptation = _preview.PostFxVolume.EyeAdaptation;
                eyeAdaptation.Mode = EyeAdaptationMode.None;
                eyeAdaptation.OverrideFlags |= EyeAdaptationSettingsOverride.Mode;
                _preview.PostFxVolume.EyeAdaptation = eyeAdaptation;
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
            var asset = (ParticleEmitter)request.Asset;
            if (!asset.IsLoaded)
                return false;

            if (state == 0)
            {
                // Start the warmup
                _warmupRequest = request;
                request.Tag = 1;
                _preview.Emitter = asset;
            }
            else if (_preview.PreviewActor.Time >= 0.6f)
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

            _preview.Emitter = null;
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
