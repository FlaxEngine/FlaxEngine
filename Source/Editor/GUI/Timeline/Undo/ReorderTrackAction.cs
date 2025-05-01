// Copyright (c) Wojciech Figat. All rights reserved.

namespace FlaxEditor.GUI.Timeline.Undo
{
    class ReorderTrackAction : IUndoAction
    {
        private Timeline _timeline;
        private string _name, _beforeParent, _afterParent;
        private int _beforeOrder;
        private int _afterOrder;

        public ReorderTrackAction(Timeline timeline, Track track, int beforeOrder, int afterOrder, string beforeParent, string afterParent)
        {
            _timeline = timeline;
            _name = track.Name;
            _beforeOrder = beforeOrder;
            _afterOrder = afterOrder;
            _beforeParent = beforeParent;
            _afterParent = afterParent;
        }

        private void Set(int order, string parent)
        {
            var track = _timeline.FindTrack(_name);
            track.ParentTrack = _timeline.FindTrack(parent);
            track.TrackIndex = order;
            _timeline.OnTracksOrderChanged();
            _timeline.MarkAsEdited();
        }

        public string ActionString => "Reorder track";

        public void Do()
        {
            Set(_afterOrder, _afterParent);
        }

        public void Undo()
        {
            Set(_beforeOrder, _beforeParent);
        }

        public void Dispose()
        {
            _timeline = null;
            _name = null;
            _beforeParent = null;
        }
    }
}
