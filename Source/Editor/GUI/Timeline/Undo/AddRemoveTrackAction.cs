// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System.IO;
using FlaxEngine;

namespace FlaxEditor.GUI.Timeline.Undo
{
    class AddRemoveTrackAction : IUndoAction
    {
        private Timeline _timeline;
        private bool _isAdd;
        private TrackCreateOptions _options;
        private Color _color;
        private byte[] _data;
        private string _name;
        private string _parentName;
        private int _order;
        private bool _expanded;

        public AddRemoveTrackAction(Timeline timeline, Track track, bool isAdd)
        {
            _timeline = timeline;
            _isAdd = isAdd;
            _options = new TrackCreateOptions
            {
                Archetype = track.Archetype,
                Loop = track.Loop,
                Mute = track.Mute,
            };
            _color = track.Color;
            _name = track.Name;
            _parentName = track.ParentTrack?.Name;
            _order = track.TrackIndex;
            _expanded = track.IsExpanded;
            using (var memory = new MemoryStream(512))
            using (var stream = new BinaryWriter(memory))
            {
                _options.Archetype.Save(track, stream);
                _data = memory.ToArray();
            }
        }

        private void Add()
        {
            var track = _timeline.FindTrack(_name);
            if (track != null)
                return;
            track = _options.Archetype.Create(_options);
            track.Name = _name;
            track.Color = _color;
            track.IsExpanded = _expanded;
            using (var memory = new MemoryStream(_data))
            using (var stream = new BinaryReader(memory))
            {
                track.Archetype.Load(Timeline.FormatVersion, track, stream);
            }
            if (_parentName != null)
                track.ParentTrack = _timeline.FindTrack(_parentName);
            _timeline.AddTrack(track, false);
            track.TrackIndex = _order;
            _timeline.OnTracksOrderChanged();
        }

        private void Remove()
        {
            var track = _timeline.FindTrack(_name);
            _timeline.Delete(track, false);
        }

        public string ActionString => _isAdd ? "Add track" : "Remove track";

        public void Do()
        {
            if (_isAdd)
                Add();
            else
                Remove();
        }

        public void Undo()
        {
            if (_isAdd)
                Remove();
            else
                Add();
        }

        public void Dispose()
        {
            _options = new TrackCreateOptions();
            _timeline = null;
            _data = null;
            _name = null;
            _parentName = null;
        }
    }
}
