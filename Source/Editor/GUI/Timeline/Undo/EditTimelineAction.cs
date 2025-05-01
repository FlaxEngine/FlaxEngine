// Copyright (c) Wojciech Figat. All rights reserved.

using System.IO;

namespace FlaxEditor.GUI.Timeline.Undo
{
    class EditTimelineAction : IUndoAction
    {
        private Timeline _timeline;
        private byte[] _beforeData;
        private byte[] _afterData;

        public EditTimelineAction(Timeline timeline, byte[] beforeData, byte[] afterData)
        {
            _timeline = timeline;
            _beforeData = beforeData;
            _afterData = afterData;
        }

        public static byte[] CaptureData(Timeline timeline)
        {
            using (var memory = new MemoryStream(512))
            using (var stream = new BinaryWriter(memory))
            {
                stream.Write(timeline.DurationFrames);
                return memory.ToArray();
            }
        }

        private void Set(byte[] data)
        {
            using (var memory = new MemoryStream(data))
            using (var stream = new BinaryReader(memory))
            {
                _timeline.DurationFrames = stream.ReadInt32();
            }
            _timeline.ArrangeTracks();
            _timeline.MarkAsEdited();
        }

        public string ActionString => "Edit timeline";

        public void Do()
        {
            Set(_afterData);
        }

        public void Undo()
        {
            Set(_beforeData);
        }

        public void Dispose()
        {
            _timeline = null;
            _beforeData = null;
            _afterData = null;
        }
    }
}
