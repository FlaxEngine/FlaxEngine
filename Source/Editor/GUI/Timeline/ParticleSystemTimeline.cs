// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using FlaxEditor.Content;
using FlaxEditor.GUI.Drag;
using FlaxEditor.GUI.Timeline.Tracks;
using FlaxEditor.Utilities;
using FlaxEditor.Viewport.Previews;
using FlaxEngine;
using FlaxEngine.Utilities;

namespace FlaxEditor.GUI.Timeline
{
    /// <summary>
    /// The timeline editor for particle system asset.
    /// </summary>
    /// <seealso cref="FlaxEditor.GUI.Timeline.Timeline" />
    public sealed class ParticleSystemTimeline : Timeline
    {
        private sealed class Proxy : ProxyBase<ParticleSystemTimeline>
        {
            /// <inheritdoc />
            public Proxy(ParticleSystemTimeline timeline)
            : base(timeline)
            {
            }
        }

        private ParticleSystemPreview _preview;

        internal List<ParticleEmitterTrack> Emitters;

        /// <summary>
        /// Initializes a new instance of the <see cref="ParticleSystemTimeline"/> class.
        /// </summary>
        /// <param name="preview">The particle system preview.</param>
        /// <param name="undo">The undo/redo to use for the history actions recording. Optional, can be null to disable undo support.</param>
        public ParticleSystemTimeline(ParticleSystemPreview preview, FlaxEditor.Undo undo = null)
        : base(PlaybackButtons.Play | PlaybackButtons.Stop, undo)
        {
            PlaybackState = PlaybackStates.Playing;
            PropertiesEditObject = new Proxy(this);

            _preview = preview;

            // Setup track types
            TrackArchetypes.Add(ParticleEmitterTrack.GetArchetype());
            TrackArchetypes.Add(FolderTrack.GetArchetype());
        }

        /// <summary>
        /// Called when emitters parameters overrides collection gets edited.
        /// </summary>
        public void OnEmittersParametersOverridesEdited()
        {
            _isModified = true;
        }

        /// <inheritdoc />
        protected override void SetupDragDrop()
        {
            base.SetupDragDrop();

            DragHandlers.Add(new DragHandler(new DragAssets(IsValidAsset), OnDragAsset));
        }

        private static bool IsValidAsset(AssetItem assetItem)
        {
            if (assetItem is BinaryAssetItem binaryAssetItem)
            {
                if (typeof(ParticleEmitter).IsAssignableFrom(binaryAssetItem.Type))
                {
                    var emitter = FlaxEngine.Content.LoadAsync<ParticleEmitter>(binaryAssetItem.ID);
                    if (emitter)
                        return true;
                }
            }

            return false;
        }

        private static void OnDragAsset(Timeline timeline, DragHelper drag)
        {
            foreach (var assetItem in ((DragAssets)drag).Objects)
            {
                if (assetItem is BinaryAssetItem binaryAssetItem)
                {
                    if (typeof(ParticleEmitter).IsAssignableFrom(binaryAssetItem.Type))
                    {
                        var emitter = FlaxEngine.Content.LoadAsync<ParticleEmitter>(binaryAssetItem.ID);
                        if (emitter)
                        {
                            var track = (ParticleEmitterTrack)timeline.NewTrack(ParticleEmitterTrack.GetArchetype());
                            track.Asset = emitter;
                            track.TrackMedia.DurationFrames = timeline.DurationFrames;
                            track.Rename(assetItem.ShortName);
                            timeline.AddTrack(track);
                        }
                    }
                }
            }
        }

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            CurrentFrame = (int)(_preview.PreviewActor.Time * _preview.System.FramesPerSecond);

            base.Update(deltaTime);
        }

        /// <inheritdoc />
        public override void OnPlay()
        {
            _preview.PlaySimulation = true;

            base.OnPlay();
        }

        /// <inheritdoc />
        public override void OnPause()
        {
            _preview.PlaySimulation = false;
            _preview.PreviewActor.LastUpdateTime = -1.0f;

            base.OnPause();
        }

        /// <inheritdoc />
        public override void OnStop()
        {
            _preview.PlaySimulation = false;
            _preview.PreviewActor.ResetSimulation();
            _preview.PreviewActor.UpdateSimulation();

            base.OnStop();
        }

        /// <summary>
        /// Loads the timeline from the specified <see cref="FlaxEngine.ParticleSystem"/> asset.
        /// </summary>
        /// <param name="asset">The asset.</param>
        public void Load(ParticleSystem asset)
        {
            Profiler.BeginEvent("Asset.LoadTimeline");
            var data = asset.LoadTimeline();
            Profiler.EndEvent();

            Profiler.BeginEvent("Timeline.Load");
            Load(data);
            Profiler.EndEvent();
        }

        /// <summary>
        /// Saves the timeline data to the <see cref="FlaxEngine.ParticleSystem"/> asset.
        /// </summary>
        /// <param name="asset">The asset.</param>
        public void Save(ParticleSystem asset)
        {
            var data = Save();
            asset.SaveTimeline(data);
            asset.Reload();
        }

        /// <inheritdoc />
        protected override void LoadTimelineData(int version, BinaryReader stream)
        {
            base.LoadTimelineData(version, stream);

            // Load emitters
            int emittersCount = stream.ReadInt32();
        }

        /// <inheritdoc />
        protected override void SaveTimelineData(BinaryWriter stream)
        {
            base.SaveTimelineData(stream);

            // Save emitters
            Emitters = Tracks.Where(track => track is ParticleEmitterTrack).Cast<ParticleEmitterTrack>().ToList();
            int emittersCount = Emitters.Count;
            stream.Write(emittersCount);
        }

        /// <inheritdoc />
        protected override void LoadTimelineCustomData(int version, BinaryReader stream)
        {
            base.LoadTimelineCustomData(version, stream);

            Emitters = Tracks.Where(track => track is ParticleEmitterTrack).Cast<ParticleEmitterTrack>().ToList();

            // Load parameters overrides
            int overridesCount = stream.BaseStream.Position != stream.BaseStream.Length ? stream.ReadInt32() : 0;
            if (overridesCount != 0)
            {
                for (int i = 0; i < overridesCount; i++)
                {
                    var idx = stream.ReadInt32();
                    var id = stream.ReadGuid();
                    object value = null;
                    if (version == 2)
                        stream.ReadCommonValue(ref value);
                    else
                        value = stream.ReadVariant();

                    Emitters[idx].ParametersOverrides.Add(id, value);
                }
            }
        }

        /// <inheritdoc />
        protected override void SaveTimelineCustomData(BinaryWriter stream)
        {
            base.SaveTimelineCustomData(stream);

            // Save parameters overrides
            var emittersParametersOverrides = new Dictionary<KeyValuePair<int, Guid>, object>();
            for (var i = 0; i < Emitters.Count; i++)
            {
                var emitterTrack = Emitters[i];
                if (emitterTrack.ParametersOverrides.Count != 0)
                {
                    foreach (var e in emitterTrack.ParametersOverrides)
                    {
                        emittersParametersOverrides.Add(new KeyValuePair<int, Guid>(i, e.Key), e.Value);
                    }
                }
            }
            if (emittersParametersOverrides.Count != 0)
            {
                stream.Write(emittersParametersOverrides.Count);
                foreach (var e in emittersParametersOverrides)
                {
                    var id = e.Key.Value;
                    stream.Write(e.Key.Key);
                    stream.WriteGuid(ref id);
                    stream.WriteVariant(e.Value);
                }
            }
        }

        /// <inheritdoc />
        public override void Load(byte[] data)
        {
            base.Load(data);

            OnPlay();
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            _preview = null;
            if (Emitters != null)
            {
                Emitters.Clear();
                Emitters = null;
            }

            base.OnDestroy();
        }
    }
}
