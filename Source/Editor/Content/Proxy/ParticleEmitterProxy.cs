// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.Content.Create;
using FlaxEditor.Content.Thumbnails;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.GUI.Timeline;
using FlaxEditor.GUI.Timeline.Tracks;
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
    [ContentContextMenu("New/Particles/Particle Emitter")]
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
        public override void OnContentWindowContextMenu(ContextMenu menu, ContentItem item)
        {
            base.OnContentWindowContextMenu(menu, item);

            if (item is BinaryAssetItem binaryAssetItem)
            {
                var button = menu.AddButton("Create Particle System", CreateParticleSystemClicked);
                button.Tag = binaryAssetItem;
            }
        }

        private void CreateParticleSystemClicked(ContextMenuButton obj)
        {
            var binaryAssetItem = (BinaryAssetItem)obj.Tag;
            CreateParticleSystem(binaryAssetItem);
        }

        /// <summary>
        /// Creates the particle system from the given particle emitter.
        /// </summary>
        /// <param name="emitterItem">The particle emitter item to use as a base for the particle system.</param>
        public static void CreateParticleSystem(BinaryAssetItem emitterItem)
        {
            var particleSystemName = emitterItem.ShortName + " Particle System";
            var particleSystemProxy = Editor.Instance.ContentDatabase.GetProxy<ParticleSystem>();
            Editor.Instance.Windows.ContentWin.NewItem(particleSystemProxy, null, item => OnParticleSystemCreated(item, emitterItem), particleSystemName);
        }

        private static void OnParticleSystemCreated(ContentItem item, BinaryAssetItem particleItem)
        {
            var assetItem = (AssetItem)item;
            var particleSystem = FlaxEngine.Content.LoadAsync<ParticleSystem>(assetItem.ID);
            if (particleSystem == null || particleSystem.WaitForLoaded())
            {
                Editor.LogError("Failed to load created particle system.");
                return;
            }

            ParticleEmitter emitter = FlaxEngine.Content.LoadAsync<ParticleEmitter>(particleItem.ID);
            if (emitter == null || emitter.WaitForLoaded())
            {
                Editor.LogError("Failed to load base particle emitter.");
            }

            ParticleSystemPreview tempPreview = new ParticleSystemPreview(false);
            ParticleSystemTimeline timeline = new ParticleSystemTimeline(tempPreview);
            timeline.Load(particleSystem);

            var track = (ParticleEmitterTrack)timeline.NewTrack(ParticleEmitterTrack.GetArchetype());
            track.Asset = emitter;
            track.TrackMedia.DurationFrames = timeline.DurationFrames;
            track.Rename(particleItem.ShortName);
            timeline.AddTrack(track);
            timeline.Save(particleSystem);
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
