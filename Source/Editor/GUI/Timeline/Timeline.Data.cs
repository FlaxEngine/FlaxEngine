// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.IO;
using FlaxEngine;

namespace FlaxEditor.GUI.Timeline
{
    partial class Timeline
    {
        /// <summary>
        /// Loads the timeline data.
        /// </summary>
        /// <param name="version">The version.</param>
        /// <param name="stream">The input stream.</param>
        protected virtual void LoadTimelineData(int version, BinaryReader stream)
        {
        }

        /// <summary>
        /// Saves the timeline data.
        /// </summary>
        /// <param name="stream">The output stream.</param>
        protected virtual void SaveTimelineData(BinaryWriter stream)
        {
        }

        /// <summary>
        /// Loads the timeline data after reading the timeline tracks.
        /// </summary>
        /// <param name="version">The version.</param>
        /// <param name="stream">The input stream.</param>
        protected virtual void LoadTimelineCustomData(int version, BinaryReader stream)
        {
        }

        /// <summary>
        /// Saves the timeline data after saving the timeline tracks.
        /// </summary>
        /// <param name="stream">The output stream.</param>
        protected virtual void SaveTimelineCustomData(BinaryWriter stream)
        {
        }

        /// <summary>
        /// Loads the timeline from the specified data.
        /// </summary>
        /// <param name="data">The data.</param>
        public virtual void Load(byte[] data)
        {
            Profiler.BeginEvent("Clear");
            Clear();
            LockChildrenRecursive();
            Profiler.EndEvent();

            using (var memory = new MemoryStream(data))
            using (var stream = new BinaryReader(memory))
            {
                Profiler.BeginEvent("LoadData");
                int version = stream.ReadInt32();
                switch (version)
                {
                case 1:
                {
                    // [Deprecated on 23.07.2019, expires on 27.04.2021]

                    // Load properties
                    FramesPerSecond = stream.ReadSingle();
                    DurationFrames = stream.ReadInt32();
                    LoadTimelineData(version, stream);

                    // Load tracks
                    int tracksCount = stream.ReadInt32();
                    _tracks.Capacity = Math.Max(_tracks.Capacity, tracksCount);
                    for (int i = 0; i < tracksCount; i++)
                    {
                        var type = stream.ReadByte();
                        var flag = stream.ReadByte();
                        Track track = null;
                        var mute = (flag & 1) == 1;
                        for (int j = 0; j < TrackArchetypes.Count; j++)
                        {
                            if (TrackArchetypes[j].TypeId == type)
                            {
                                var options = new TrackCreateOptions
                                {
                                    Archetype = TrackArchetypes[j],
                                };
                                if (mute)
                                    options.Flags |= TrackFlags.Mute;
                                track = TrackArchetypes[j].Create(options);
                                break;
                            }
                        }
                        if (track == null)
                            throw new Exception("Unknown timeline track type " + type);
                        int parentIndex = stream.ReadInt32();
                        int childrenCount = stream.ReadInt32();
                        track.Name = Utilities.Utils.ReadStr(stream, -13);
                        track.Tag = parentIndex;

                        track.Archetype.Load(version, track, stream);

                        AddLoadedTrack(track);
                    }
                    break;
                }
                case 2: // [Deprecated in 2020 expires on 03.09.2023]
                case 3: // [Deprecated on 03.09.2021 expires on 03.09.2023]
                case 4:
                {
                    // Load properties
                    FramesPerSecond = stream.ReadSingle();
                    DurationFrames = stream.ReadInt32();
                    LoadTimelineData(version, stream);

                    // Load tracks
                    int tracksCount = stream.ReadInt32();
                    _tracks.Capacity = Math.Max(_tracks.Capacity, tracksCount);
                    for (int i = 0; i < tracksCount; i++)
                    {
                        var type = stream.ReadByte();
                        var flag = (TrackFlags)stream.ReadByte();
                        Track track = null;
                        for (int j = 0; j < TrackArchetypes.Count; j++)
                        {
                            if (TrackArchetypes[j].TypeId == type)
                            {
                                var options = new TrackCreateOptions
                                {
                                    Archetype = TrackArchetypes[j],
                                    Flags = flag,
                                };
                                track = TrackArchetypes[j].Create(options);
                                break;
                            }
                        }
                        if (track == null)
                            throw new Exception("Unknown timeline track type " + type);
                        int parentIndex = stream.ReadInt32();
                        int childrenCount = stream.ReadInt32();
                        track.Name = Utilities.Utils.ReadStr(stream, -13);
                        track.Tag = parentIndex;
                        track.Color = stream.ReadColor32();

                        Profiler.BeginEvent("LoadTack");
                        track.Archetype.Load(version, track, stream);
                        Profiler.EndEvent();

                        AddLoadedTrack(track);
                    }
                    break;
                }
                default: throw new Exception("Unknown timeline version " + version);
                }
                LoadTimelineCustomData(version, stream);
                Profiler.EndEvent();

                Profiler.BeginEvent("ParentTracks");
                for (int i = 0; i < _tracks.Count; i++)
                {
                    var parentIndex = (int)_tracks[i].Tag;
                    _tracks[i].Tag = null;
                    if (parentIndex != -1)
                        _tracks[i].ParentTrack = _tracks[parentIndex];
                }
                Profiler.EndEvent();
                Profiler.BeginEvent("SetupTracks");
                for (int i = 0; i < _tracks.Count; i++)
                {
                    _tracks[i].OnLoaded();
                }
                Profiler.EndEvent();
            }

            Profiler.BeginEvent("ArrangeTracks");
            ArrangeTracks();
            PerformLayout(true);
            UnlockChildrenRecursive();
            PerformLayout(true);
            Profiler.EndEvent();

            ClearEditedFlag();
        }

        /// <summary>
        /// Saves the timeline data.
        /// </summary>
        /// <returns>The saved timeline data.</returns>
        public virtual byte[] Save()
        {
            // Serialize timeline to stream
            using (var memory = new MemoryStream(512))
            using (var stream = new BinaryWriter(memory))
            {
                // Save properties
                stream.Write(FormatVersion);
                stream.Write(FramesPerSecond);
                stream.Write(DurationFrames);
                SaveTimelineData(stream);

                // Save tracks
                int tracksCount = Tracks.Count;
                stream.Write(tracksCount);
                for (int i = 0; i < tracksCount; i++)
                {
                    var track = Tracks[i];

                    stream.Write((byte)track.Archetype.TypeId);
                    stream.Write((byte)track.Flags);
                    stream.Write(_tracks.IndexOf(track.ParentTrack));
                    stream.Write(track.SubTracks.Count);
                    Utilities.Utils.WriteStr(stream, track.Name, -13);
                    stream.Write((Color32)track.Color);
                    track.Archetype.Save(track, stream);
                }

                SaveTimelineCustomData(stream);

                return memory.ToArray();
            }
        }
    }
}
