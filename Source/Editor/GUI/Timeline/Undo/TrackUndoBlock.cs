// Copyright (c) Wojciech Figat. All rights reserved.

using System;

namespace FlaxEditor.GUI.Timeline.Undo
{
    class TrackUndoBlock : IDisposable
    {
        private Track _track;
        private byte[] _before;
        private IUndoAction _customActionBefore;
        private IUndoAction _customActionAfter;

        public TrackUndoBlock(Track track, IUndoAction customActionBefore = null, IUndoAction customActionAfter = null)
        {
            _track = track;
            if (_track.Timeline.Undo != null)
            {
                _customActionBefore = customActionBefore;
                _customActionAfter = customActionAfter;
                _before = EditTrackAction.CaptureData(track);
            }
        }

        public void Dispose()
        {
            if (_track.Timeline.Undo != null)
            {
                var after = EditTrackAction.CaptureData(_track);
                if (!FlaxEngine.Utils.ArraysEqual(_before, after))
                {
                    var action = new EditTrackAction(_track.Timeline, _track, _before, after);
                    if (_customActionAfter != null && _customActionBefore != null)
                        _track.Timeline.Undo.AddAction(new MultiUndoAction(_customActionBefore, action, _customActionAfter));
                    else if (_customActionAfter != null)
                        _track.Timeline.Undo.AddAction(new MultiUndoAction(action, _customActionAfter));
                    else if (_customActionBefore != null)
                        _track.Timeline.Undo.AddAction(new MultiUndoAction(_customActionBefore, action));
                    else
                        _track.Timeline.Undo.AddAction(action);
                }
                _before = null;
                _customActionBefore = null;
                _customActionAfter = null;
            }
            _track = null;
        }
    }
}
