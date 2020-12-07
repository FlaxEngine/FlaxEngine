// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using FlaxEditor.GUI.Timeline.Tracks;
using FlaxEngine;

namespace FlaxEditor.GUI.Timeline
{
    /// <summary>
    /// The timeline editor for animation asset.
    /// </summary>
    /// <seealso cref="FlaxEditor.GUI.Timeline.Timeline" />
    public sealed class AnimationTimeline : Timeline
    {
        private sealed class Proxy : ProxyBase<AnimationTimeline>
        {
            /// <inheritdoc />
            public Proxy(AnimationTimeline timeline)
            : base(timeline)
            {
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="AnimationTimeline"/> class.
        /// </summary>
        /// <param name="undo">The undo/redo to use for the history actions recording. Optional, can be null to disable undo support.</param>
        public AnimationTimeline(FlaxEditor.Undo undo)
        : base(PlaybackButtons.None, undo, false, false)
        {
            PlaybackState = PlaybackStates.Seeking;
            ShowPreviewValues = false;
            PropertiesEditObject = new Proxy(this);

            // Setup track types
            TrackArchetypes.Add(AnimationChannelTrack.GetArchetype());
            TrackArchetypes.Add(AnimationChannelDataTrack.GetArchetype());
        }

        /// <summary>
        /// Loads the timeline from the specified <see cref="FlaxEngine.Animation"/> asset.
        /// </summary>
        /// <param name="asset">The asset.</param>
        public void Load(Animation asset)
        {
            Profiler.BeginEvent("Asset.LoadTimeline");
            asset.LoadTimeline(out var data);
            Profiler.EndEvent();

            Profiler.BeginEvent("Timeline.Load");
            Load(data);
            Profiler.EndEvent();
        }

        /// <summary>
        /// Saves the timeline data to the <see cref="FlaxEngine.Animation"/> asset.
        /// </summary>
        /// <param name="asset">The asset.</param>
        public void Save(Animation asset)
        {
            var data = Save();
            asset.SaveTimeline(data);
            asset.Reload();
        }

        /// <inheritdoc />
        public override void OnSeek(int frame)
        {
            CurrentFrame = frame;

            base.OnSeek(frame);
        }
    }
}
