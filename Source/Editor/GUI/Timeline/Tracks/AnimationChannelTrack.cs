// Copyright (c) Wojciech Figat. All rights reserved.

using System.IO;
using FlaxEngine;

namespace FlaxEditor.GUI.Timeline.Tracks
{
    /// <summary>
    /// The timeline track that represents an <see cref="Animation"/> channel track with sub-tracks such as position/rotation/scale.
    /// </summary>
    /// <seealso cref="FlaxEditor.GUI.Timeline.Track" />
    sealed class AnimationChannelTrack : Track
    {
        /// <summary>
        /// Gets the archetype.
        /// </summary>
        /// <returns>The archetype.</returns>
        public static TrackArchetype GetArchetype()
        {
            return new TrackArchetype
            {
                TypeId = 17,
                Name = "Animation Channel",
                DisableSpawnViaGUI = true,
                Create = options => new AnimationChannelTrack(ref options),
                Load = LoadTrack,
                Save = SaveTrack,
            };
        }

        private static void LoadTrack(int version, Track track, BinaryReader stream)
        {
            var e = (AnimationChannelTrack)track;
        }

        private static void SaveTrack(Track track, BinaryWriter stream)
        {
            var e = (AnimationChannelTrack)track;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="AnimationChannelTrack"/> class.
        /// </summary>
        /// <param name="options">The options.</param>
        public AnimationChannelTrack(ref TrackCreateOptions options)
        : base(ref options)
        {
            _muteCheckbox.Visible = false;
        }
    }

    /// <summary>
    /// The timeline track for animating object property via Linear Curve.
    /// </summary>
    /// <seealso cref="CurvePropertyTrackBase" />
    sealed class AnimationChannelDataTrack : CurvePropertyTrackBase
    {
        /// <summary>
        /// Gets the archetype.
        /// </summary>
        /// <returns>The archetype.</returns>
        public static TrackArchetype GetArchetype()
        {
            return new TrackArchetype
            {
                TypeId = 18,
                Name = "Animation Channel Data",
                DisableSpawnViaGUI = true,
                Create = options => new AnimationChannelDataTrack(ref options),
                Load = LoadTrack,
                Save = SaveTrack,
            };
        }

        private static void LoadTrack(int version, Track track, BinaryReader stream)
        {
            var e = (AnimationChannelDataTrack)track;

            int type = stream.ReadByte();
            int keyframesCount = stream.ReadInt32();
            var propertyType = type == 1 ? typeof(Quaternion) : typeof(Float3);
            e.Title = type == 0 ? "Position" : (type == 1 ? "Rotation" : "Scale");
            e._type = type;
            e.MemberTypeName = propertyType.FullName;
            e.ValueSize = type == 1 ? Quaternion.SizeInBytes : Float3.SizeInBytes;

            var keyframes = new object[keyframesCount];
            for (int i = 0; i < keyframesCount; i++)
            {
                var time = stream.ReadSingle();
                object value;
                if (type == 1)
                    value = stream.ReadQuaternion();
                else
                    value = stream.ReadFloat3();
                keyframes[i] = new LinearCurve<object>.Keyframe
                {
                    Time = time,
                    Value = value,
                };
            }

            if (e.Curve != null && e.Curve.ValueType != propertyType)
            {
                e.Curve.Dispose();
                e.Curve = null;
            }
            if (e.Curve == null)
            {
                e.CreateCurve(propertyType, typeof(LinearCurveEditor<>));
            }
            e.Curve.SetKeyframes(keyframes);
        }

        private static void SaveTrack(Track track, BinaryWriter stream)
        {
            var e = (AnimationChannelDataTrack)track;

            var keyframesCount = e.Curve?.KeyframesCount ?? 0;

            stream.Write((byte)e._type);
            stream.Write(keyframesCount);

            if (keyframesCount == 0)
                return;
            for (int i = 0; i < keyframesCount; i++)
            {
                e.Curve.GetKeyframe(i, out var time, out var value, out _, out _);
                stream.Write(time);
                if (e._type == 1)
                    stream.Write((Quaternion)value);
                else
                    stream.Write((Float3)value);
            }
        }

        private int _type;

        /// <inheritdoc />
        public AnimationChannelDataTrack(ref TrackCreateOptions options)
        : base(ref options)
        {
            _muteCheckbox.Visible = false;
        }

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            base.Update(deltaTime);

            // Disable red tint on missing member
            TitleTintColor = Color.White;
        }
    }
}
