// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.IO;
using System.Linq;
using FlaxEditor.Utilities;
using FlaxEngine;

namespace FlaxEditor.GUI.Timeline.Tracks
{
    /// <summary>
    /// The timeline media that represents a post-process material media event.
    /// </summary>
    /// <seealso cref="FlaxEditor.GUI.Timeline.Media" />
    public class PostProcessMaterialMedia : Media
    {
        private sealed class Proxy : ProxyBase<PostProcessMaterialTrack, PostProcessMaterialMedia>
        {
            [EditorDisplay("General"), EditorOrder(10), Tooltip("The post process material to show.")]
            public MaterialBase PostProcessMaterial
            {
                get => Track.Asset;
                set => Track.Asset = value;
            }

            /// <inheritdoc />
            public Proxy(PostProcessMaterialTrack track, PostProcessMaterialMedia media)
            : base(track, media)
            {
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="PostProcessMaterialMedia"/> class.
        /// </summary>
        public PostProcessMaterialMedia()
        {
            CanSplit = true;
            CanDelete = true;
        }

        /// <inheritdoc />
        public override void OnTimelineChanged(Track track)
        {
            base.OnTimelineChanged(track);

            PropertiesEditObject = track != null ? new Proxy((PostProcessMaterialTrack)track, this) : null;
        }
    }

    /// <summary>
    /// The timeline track that represents a post-process material playback.
    /// </summary>
    /// <seealso cref="FlaxEditor.GUI.Timeline.Track" />
    public class PostProcessMaterialTrack : SingleMediaAssetTrack<MaterialBase, PostProcessMaterialMedia>
    {
        /// <summary>
        /// Gets the archetype.
        /// </summary>
        /// <returns>The archetype.</returns>
        public static TrackArchetype GetArchetype()
        {
            return new TrackArchetype
            {
                TypeId = 2,
                Name = "Post Process Material",
                Create = options => new PostProcessMaterialTrack(ref options),
                Load = LoadTrack,
                Save = SaveTrack,
            };
        }

        private static void LoadTrack(int version, Track track, BinaryReader stream)
        {
            var e = (PostProcessMaterialTrack)track;
            Guid id = stream.ReadGuid();
            e.Asset = FlaxEngine.Content.LoadAsync<MaterialBase>(id);
            if (version <= 3)
            {
                // [Deprecated on 03.09.2021 expires on 03.09.2023]
                var m = e.TrackMedia;
                m.StartFrame = stream.ReadInt32();
                m.DurationFrames = stream.ReadInt32();
            }
            else
            {
                var count = stream.ReadInt32();
                while (e.Media.Count > count)
                    e.RemoveMedia(e.Media.Last());
                while (e.Media.Count < count)
                    e.AddMedia(new PostProcessMaterialMedia());
                for (int i = 0; i < count; i++)
                {
                    var m = e.Media[i];
                    m.StartFrame = stream.ReadInt32();
                    m.DurationFrames = stream.ReadInt32();
                }
            }
        }

        private static void SaveTrack(Track track, BinaryWriter stream)
        {
            var e = (PostProcessMaterialTrack)track;
            stream.WriteGuid(ref e.AssetID);
            var count = e.Media.Count;
            stream.Write(count);
            for (int i = 0; i < count; i++)
            {
                var m = e.Media[i];
                stream.Write(m.StartFrame);
                stream.Write(m.DurationFrames);
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="PostProcessMaterialTrack"/> class.
        /// </summary>
        /// <param name="options">The options.</param>
        public PostProcessMaterialTrack(ref TrackCreateOptions options)
        : base(ref options)
        {
            MinMediaCount = 1;
        }
    }
}
