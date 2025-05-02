// Copyright (c) Wojciech Figat. All rights reserved.

namespace FlaxEditor.GUI.Timeline.Undo
{
    sealed class EditFpsAction : IUndoAction
    {
        private Timeline _timeline;
        private float _before, _after;

        public EditFpsAction(Timeline timeline, float before, float after)
        {
            _timeline = timeline;
            _before = before;
            _after = after;
        }

        public string ActionString => "Edit FPS";

        public void Do()
        {
            _timeline.SetFPS(_after);
        }

        public void Undo()
        {
            _timeline.SetFPS(_before);
        }

        public void Dispose()
        {
            _timeline = null;
        }
    }
}
