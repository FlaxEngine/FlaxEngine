// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using FlaxEngine;

namespace FlaxEditor.GUI.Timeline.Tracks
{
    /// <summary>
    /// The timeline media that represents a particle miter playback media event.
    /// </summary>
    /// <seealso cref="FlaxEditor.GUI.Timeline.Media" />
    public class ParticleEmitterMedia : SingleMediaAssetMedia
    {
        private sealed class Proxy : ProxyBase<ParticleEmitterTrack, ParticleEmitterMedia>
        {
            /// <summary>
            /// Gets or sets the particle emitter to simulate.
            /// </summary>
            [EditorDisplay("General"), EditorOrder(10), Tooltip("The particle emitter to simulate.")]
            public ParticleEmitter ParticleEmitter
            {
                get => Track.Asset;
                set => Track.Asset = value;
            }

            /// <inheritdoc />
            public Proxy(ParticleEmitterTrack track, ParticleEmitterMedia media)
            : base(track, media)
            {
            }
        }

        /// <inheritdoc />
        public override void OnTimelineChanged(Track track)
        {
            base.OnTimelineChanged(track);

            PropertiesEditObject = new Proxy(Track as ParticleEmitterTrack, this);
        }
    }

    /// <summary>
    /// The timeline track that represents a particle emitter playback.
    /// </summary>
    /// <seealso cref="FlaxEditor.GUI.Timeline.Track" />
    public class ParticleEmitterTrack : SingleMediaAssetTrack<ParticleEmitter, ParticleEmitterMedia>
    {
        /// <summary>
        /// Gets the archetype.
        /// </summary>
        /// <returns>The archetype.</returns>
        public static TrackArchetype GetArchetype()
        {
            return new TrackArchetype
            {
                TypeId = 0,
                Name = "Particle Emitter",
                Create = options => new ParticleEmitterTrack(ref options),
                Load = LoadTrack,
                Save = SaveTrack,
            };
        }

        private static void LoadTrack(int version, Track track, BinaryReader stream)
        {
            var e = (ParticleEmitterTrack)track;
            Guid id = new Guid(stream.ReadBytes(16));
            e.Asset = FlaxEngine.Content.LoadAsync<ParticleEmitter>(id);
            stream.ReadInt32(); // Skip emitterIndex
            var m = e.TrackMedia;
            m.StartFrame = stream.ReadInt32();
            m.DurationFrames = stream.ReadInt32();
        }

        private static void SaveTrack(Track track, BinaryWriter stream)
        {
            var e = (ParticleEmitterTrack)track;
            var emitterId = e.Asset?.ID ?? Guid.Empty;

            stream.Write(emitterId.ToByteArray());
            stream.Write(((ParticleSystemTimeline)track.Timeline).Emitters.IndexOf(e));

            if (e.Media.Count != 0)
            {
                var m = e.TrackMedia;
                stream.Write(m.StartFrame);
                stream.Write(m.DurationFrames);
            }
            else
            {
                stream.Write(0);
                stream.Write(track.Timeline.DurationFrames);
            }
        }

        /// <summary>
        /// The overriden values for the emitters parameters. Key is parameter ID, value is the custom value.
        /// </summary>
        public Dictionary<Guid, object> ParametersOverrides = new Dictionary<Guid, object>();

        /// <summary>
        /// Initializes a new instance of the <see cref="ParticleEmitterTrack"/> class.
        /// </summary>
        /// <param name="options">The options.</param>
        public ParticleEmitterTrack(ref TrackCreateOptions options)
        : base(ref options)
        {
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            ParametersOverrides.Clear();
            ParametersOverrides = null;

            base.OnDestroy();
        }
    }
}
