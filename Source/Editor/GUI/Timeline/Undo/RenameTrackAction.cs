// Copyright (c) Wojciech Figat. All rights reserved.

namespace FlaxEditor.GUI.Timeline.Undo
{
    class RenameTrackAction : IUndoAction
    {
        private Timeline _timeline;
        private string _beforeName;
        private string _afterName;

        public RenameTrackAction(Timeline timeline, Track track, string oldName, string newName)
        {
            _timeline = timeline;
            _beforeName = oldName;
            _afterName = newName;
        }

        private void Set(string oldName, string newName)
        {
            var track = _timeline.FindTrack(oldName);
            track.Rename(newName);
            _timeline.MarkAsEdited();
        }

        public string ActionString => "Rename track";

        public void Do()
        {
            Set(_beforeName, _afterName);
        }

        public void Undo()
        {
            Set(_afterName, _beforeName);
        }

        public void Dispose()
        {
            _timeline = null;
            _beforeName = null;
            _afterName = null;
        }
    }
}
