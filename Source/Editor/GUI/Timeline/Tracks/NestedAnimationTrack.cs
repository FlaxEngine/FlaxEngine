// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.IO;
using System.Linq;
using FlaxEditor.GUI.Timeline.Undo;
using FlaxEditor.Utilities;
using FlaxEngine;

namespace FlaxEditor.GUI.Timeline.Tracks
{
    /// <summary>
    /// The timeline media that represents a nested animation media event.
    /// </summary>
    /// <seealso cref="FlaxEditor.GUI.Timeline.Media" />
    public class NestedAnimationMedia : Media
    {
        private sealed class Proxy : ProxyBase<NestedAnimationTrack, NestedAnimationMedia>
        {
            /// <summary>
            /// The nested animation to play.
            /// </summary>
            [EditorDisplay("General"), EditorOrder(10)]
            public Animation NestedAnimation
            {
                get => Track.Asset;
                set => Track.Asset = value;
            }

            /// <summary>
            /// If checked, the nested animation will loop when playback exceeds its duration. Otherwise it will stop play.
            /// </summary>
            [EditorDisplay("General"), EditorOrder(20)]
            public bool Loop
            {
                get => Track.Loop;
                set => Track.Loop = value;
            }

            /// <summary>
            /// Animation playback speed.
            /// </summary>
            [EditorDisplay("General"), EditorOrder(30), Limit(0.0f, 100.0f, 0.001f)]
            public float Speed
            {
                get => Track.Speed;
                set => Track.Speed = value;
            }

            /// <summary>
            /// Animation playback start time position (in seconds). Can be used to offset the nested animation start.
            /// </summary>
            [EditorDisplay("General"), EditorOrder(40)]
            public float StartTime
            {
                get => Track.StartTime;
                set => Track.StartTime = value;
            }

            /// <inheritdoc />
            public Proxy(NestedAnimationTrack track, NestedAnimationMedia media)
            : base(track, media)
            {
            }
        }

        /// <inheritdoc />
        public override void OnTimelineChanged(Track track)
        {
            base.OnTimelineChanged(track);

            PropertiesEditObject = track != null ? new Proxy((NestedAnimationTrack)track, this) : null;
        }
    }

    /// <summary>
    /// The timeline track that represents a nested animation playback.
    /// </summary>
    /// <seealso cref="FlaxEditor.GUI.Timeline.Track" />
    public class NestedAnimationTrack : SingleMediaAssetTrack<Animation, NestedAnimationMedia>
    {
        /// <summary>
        /// Gets the archetype.
        /// </summary>
        /// <returns>The archetype.</returns>
        public static TrackArchetype GetArchetype()
        {
            return new TrackArchetype
            {
                TypeId = 20,
                Name = "Nested Animation",
                Create = options => new NestedAnimationTrack(ref options),
                Load = LoadTrack,
                Save = SaveTrack,
            };
        }

        private static void LoadTrack(int version, Track track, BinaryReader stream)
        {
            var e = (NestedAnimationTrack)track;
            Guid id = stream.ReadGuid();
            e.Asset = FlaxEngine.Content.LoadAsync<Animation>(id);
            var m = e.TrackMedia;
            m.StartFrame = (int)stream.ReadSingle();
            m.DurationFrames = (int)stream.ReadSingle();
            e.Speed = stream.ReadSingle();
            e.StartTime = stream.ReadSingle();
        }

        private static void SaveTrack(Track track, BinaryWriter stream)
        {
            var e = (NestedAnimationTrack)track;
            stream.WriteGuid(ref e.AssetID);
            if (e.Media.Count != 0)
            {
                var m = e.TrackMedia;
                stream.Write((float)m.StartFrame);
                stream.Write((float)m.DurationFrames);
            }
            else
            {
                stream.Write((float)0);
                stream.Write((float)track.Timeline.DurationFrames);
            }
            stream.Write(e.Speed);
            stream.Write(e.StartTime);
        }

        /// <summary>
        /// Nested animation playback speed.
        /// </summary>
        public float Speed = 1.0f;

        /// <summary>
        /// Nested animation playback start time position (in seconds). Can be used to offset the nested animation start.
        /// </summary>
        public float StartTime = 0.0f;

        /// <inheritdoc />
        public NestedAnimationTrack(ref TrackCreateOptions options)
        : base(ref options)
        {
            MinMediaCount = 1;
            MaxMediaCount = 1;
        }

        /// <inheritdoc />
        protected override void OnAssetChanged()
        {
            base.OnAssetChanged();

            CheckCyclicReferences();

            if (Timeline != null && Asset && !Asset.WaitForLoaded())
            {
                using (new TrackUndoBlock(this))
                    TrackMedia.Duration = Asset.Length;
            }
        }

        /// <inheritdoc />
        public override void OnTimelineChanged(Timeline timeline)
        {
            base.OnTimelineChanged(timeline);

            CheckCyclicReferences();
        }

        private void CheckCyclicReferences()
        {
            if (Asset && Timeline is AnimationTimeline timeline)
            {
                var refs = Asset.GetReferences();
                var id = timeline._id;
                if (Asset.ID == id || refs.Contains(id))
                {
                    Asset = null;
                    throw new Exception("Cannot use nested scene animation (recursion).");
                }
            }
        }
    }
}
