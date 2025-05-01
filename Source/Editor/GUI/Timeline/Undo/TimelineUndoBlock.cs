// Copyright (c) Wojciech Figat. All rights reserved.

using System;

namespace FlaxEditor.GUI.Timeline.Undo
{
    class TimelineUndoBlock : IDisposable
    {
        private Timeline _timeline;
        private byte[] _before;
        private IUndoAction _customActionBefore;
        private IUndoAction _customActionAfter;

        public TimelineUndoBlock(Timeline timeline, IUndoAction customActionBefore = null, IUndoAction customActionAfter = null)
        {
            _timeline = timeline;
            if (timeline.Undo != null)
            {
                _customActionBefore = customActionBefore;
                _customActionAfter = customActionAfter;
                _before = EditTimelineAction.CaptureData(timeline);
            }
        }

        public void Dispose()
        {
            if (_timeline.Undo != null)
            {
                var after = EditTimelineAction.CaptureData(_timeline);
                if (!FlaxEngine.Utils.ArraysEqual(_before, after))
                {
                    var action = new EditTimelineAction(_timeline, _before, after);
                    if (_customActionAfter != null && _customActionBefore != null)
                        _timeline.Undo.AddAction(new MultiUndoAction(_customActionBefore, action, _customActionAfter));
                    else if (_customActionAfter != null)
                        _timeline.Undo.AddAction(new MultiUndoAction(action, _customActionAfter));
                    else if (_customActionBefore != null)
                        _timeline.Undo.AddAction(new MultiUndoAction(_customActionBefore, action));
                    else
                        _timeline.Undo.AddAction(action);
                }
                _before = null;
                _customActionBefore = null;
                _customActionAfter = null;
            }
            _timeline = null;
        }
    }
}
