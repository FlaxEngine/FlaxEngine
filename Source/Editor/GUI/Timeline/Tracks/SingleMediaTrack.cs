// Copyright (c) Wojciech Figat. All rights reserved.

namespace FlaxEditor.GUI.Timeline.Tracks
{
    /// <summary>
    /// The base class for timeline tracks that use single media.
    /// </summary>
    /// <typeparam name="TMedia">The type of the media.</typeparam>
    /// <seealso cref="FlaxEditor.GUI.Timeline.Track" />
    public abstract class SingleMediaTrack<TMedia> : Track
    where TMedia : Media, new()
    {
        /// <summary>
        /// Gets the track media.
        /// </summary>
        public TMedia TrackMedia
        {
            get
            {
                TMedia media;
                if (Media.Count == 0)
                {
                    media = new TMedia
                    {
                        StartFrame = 0,
                        DurationFrames = Timeline?.DurationFrames ?? 60,
                    };
                    AddMedia(media);
                }
                else
                {
                    media = (TMedia)Media[0];
                }
                return media;
            }
        }

        /// <inheritdoc />
        protected SingleMediaTrack(ref TrackCreateOptions options)
        : base(ref options)
        {
        }

        /// <inheritdoc />
        public override void OnSpawned()
        {
            // Ensure to have minimum valid media count
            for (int i = Media.Count; i < MinMediaCount; i++)
            {
                var m = new TMedia
                {
                    StartFrame = 0,
                    DurationFrames = Timeline?.DurationFrames ?? 60,
                };
                AddMedia(m);
            }

            base.OnSpawned();
        }
    }
}
