// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.IO;
using System.Reflection;
using System.Text;
using FlaxEngine.Utilities;

namespace FlaxEditor.GUI.Timeline.Tracks
{
    /// <summary>
    /// The timeline track for animating <see cref="FlaxEngine.Object"/> reference property via keyframes collection.
    /// </summary>
    /// <seealso cref="MemberTrack" />
    /// <seealso cref="KeyframesPropertyTrack" />
    public sealed class ObjectReferencePropertyTrack : KeyframesPropertyTrack
    {
        /// <summary>
        /// Gets the archetype.
        /// </summary>
        /// <returns>The archetype.</returns>
        public new static TrackArchetype GetArchetype()
        {
            return new TrackArchetype
            {
                TypeId = 12,
                Name = "Property",
                DisableSpawnViaGUI = true,
                Create = options => new ObjectReferencePropertyTrack(ref options),
                Load = LoadTrack,
                Save = SaveTrack,
            };
        }

        private static void LoadTrack(int version, Track track, BinaryReader stream)
        {
            var e = (ObjectReferencePropertyTrack)track;

            e.ValueSize = stream.ReadInt32();
            int propertyNameLength = stream.ReadInt32();
            int propertyTypeNameLength = stream.ReadInt32();
            int keyframesCount = stream.ReadInt32();

            var propertyName = stream.ReadBytes(propertyNameLength);
            e.MemberName = Encoding.UTF8.GetString(propertyName, 0, propertyNameLength);
            if (stream.ReadChar() != 0)
                throw new Exception("Invalid track data.");

            var propertyTypeName = stream.ReadBytes(propertyTypeNameLength);
            e.MemberTypeName = Encoding.UTF8.GetString(propertyTypeName, 0, propertyTypeNameLength);
            if (stream.ReadChar() != 0)
                throw new Exception("Invalid track data.");

            var keyframes = new KeyframesEditor.Keyframe[keyframesCount];
            var propertyType = TypeUtils.GetType(e.MemberTypeName);
            if (!propertyType)
            {
                e.Keyframes.ResetKeyframes();
                stream.ReadBytes(keyframesCount * (sizeof(float) + e.ValueSize));
                if (!string.IsNullOrEmpty(e.MemberTypeName))
                    Editor.LogError("Cannot load track " + e.MemberName + " of type " + e.MemberTypeName + ". Failed to find the value type information.");
                return;
            }

            for (int i = 0; i < keyframesCount; i++)
            {
                var time = stream.ReadSingle();
                var value = new Guid(stream.ReadBytes(16));

                keyframes[i] = new KeyframesEditor.Keyframe
                {
                    Time = time,
                    Value = value,
                };
            }

            e.Keyframes.DefaultValue = Guid.Empty;
            e.Keyframes.SetKeyframes(keyframes);
        }

        private static void SaveTrack(Track track, BinaryWriter stream)
        {
            var e = (ObjectReferencePropertyTrack)track;

            var propertyName = e.MemberName ?? string.Empty;
            var propertyNameData = Encoding.UTF8.GetBytes(propertyName);
            if (propertyNameData.Length != propertyName.Length)
                throw new Exception(string.Format("The object member name bytes data has different size as UTF8 bytes. Type {0}.", propertyName));

            var propertyTypeName = e.MemberTypeName ?? string.Empty;
            var propertyTypeNameData = Encoding.UTF8.GetBytes(propertyTypeName);
            if (propertyTypeNameData.Length != propertyTypeName.Length)
                throw new Exception(string.Format("The object member typename bytes data has different size as UTF8 bytes. Type {0}.", propertyTypeName));

            var keyframes = e.Keyframes.Keyframes;

            stream.Write(e.ValueSize);
            stream.Write(propertyNameData.Length);
            stream.Write(propertyTypeNameData.Length);
            stream.Write(keyframes.Count);

            stream.Write(propertyNameData);
            stream.Write('\0');

            stream.Write(propertyTypeNameData);
            stream.Write('\0');

            for (int i = 0; i < keyframes.Count; i++)
            {
                var keyframe = keyframes[i];
                stream.Write(keyframe.Time);
                stream.Write(((Guid)keyframe.Value).ToByteArray());
            }
        }

        /// <inheritdoc />
        public ObjectReferencePropertyTrack(ref TrackCreateOptions options)
        : base(ref options)
        {
        }

        /// <inheritdoc />
        protected override object GetDefaultValue(Type propertyType)
        {
            return Guid.Empty;
        }

        /// <inheritdoc />
        protected override int GetValueDataSize(Type type)
        {
            // Size of Guid
            return 16;
        }

        /// <inheritdoc />
        protected override bool TryGetValue(out object value)
        {
            if (base.TryGetValue(out value))
            {
                value = (value as FlaxEngine.Object)?.ID ?? Guid.Empty;
                return true;
            }
            return false;
        }
    }
}
