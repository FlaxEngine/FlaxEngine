// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using FlaxEditor.Surface.Elements;
using FlaxEngine.Utilities;

namespace FlaxEditor.Surface.Undo
{
    /// <summary>
    /// Edit Visject Surface node boxes connection undo action.
    /// </summary>
    /// <seealso cref="FlaxEditor.IUndoAction" />
    class ConnectBoxesAction : IUndoAction
    {
        private VisjectSurface _surface;
        private ContextHandle _context;
        private bool _connect;
        private BoxHandle _input;
        private BoxHandle _output;
        private BoxHandle[] _inputBefore;
        private BoxHandle[] _inputAfter;
        private BoxHandle[] _outputBefore;
        private BoxHandle[] _outputAfter;

        public ConnectBoxesAction(InputBox iB, OutputBox oB, bool connect)
        {
            if (iB == null || oB == null || iB.ParentNode == null || oB.ParentNode == null)
                throw new System.ArgumentNullException();
            _surface = iB.Surface;
            _context = new ContextHandle(iB.ParentNode.Context);
            _connect = connect;

            _input = new BoxHandle(iB);
            _output = new BoxHandle(oB);

            CaptureConnections(iB, out _inputBefore);
            CaptureConnections(oB, out _outputBefore);

#if BUILD_DEBUG
            // Validate handles
            if (_context.Get(_surface) != iB.ParentNode.Context)
                throw new System.Exception("Invalid ContextHandle");
            if (_input.Get(iB.ParentNode.Context) != iB || _output.Get(oB.ParentNode.Context) != oB)
                throw new System.Exception("Invalid BoxHandle");
#endif
        }

        public void End()
        {
            var context = _context.Get(_surface);
            var iB = _input.Get(context);
            var oB = _output.Get(context);

            CaptureConnections(iB, out _inputAfter);
            CaptureConnections(oB, out _outputAfter);
        }

        private void CaptureConnections(Box box, out BoxHandle[] connections)
        {
            connections = new BoxHandle[box.Connections.Count];
            for (int i = 0; i < connections.Length; i++)
            {
                var other = box.Connections[i];
                connections[i] = new BoxHandle(other);
            }
        }

        /// <inheritdoc />
        public string ActionString => _connect ? "Connect boxes" : "Disconnect boxes";

        /// <inheritdoc />
        public void Do()
        {
            Execute(_inputAfter, _outputAfter);
        }

        /// <inheritdoc />
        public void Undo()
        {
            Execute(_inputBefore, _outputBefore);
        }

        private void Execute(BoxHandle[] input, BoxHandle[] output)
        {
            var context = _context.Get(_surface);
            var iB = _input.Get(context);
            var oB = _output.Get(context);

            var toUpdate = new HashSet<Box>
            {
                iB,
                oB
            };
            toUpdate.AddRange(iB.Connections);
            toUpdate.AddRange(oB.Connections);

            for (int i = 0; i < iB.Connections.Count; i++)
            {
                var box = iB.Connections[i];
                box.Connections.Remove(iB);
            }
            for (int i = 0; i < oB.Connections.Count; i++)
            {
                var box = oB.Connections[i];
                box.Connections.Remove(oB);
            }

            iB.Connections.Clear();
            oB.Connections.Clear();

            for (int i = 0; i < input.Length; i++)
            {
                var box = input[i].Get(context);
                iB.Connections.Add(box);
                box.Connections.Add(iB);
            }
            for (int i = 0; i < output.Length; i++)
            {
                if (!output[i].Equals(_input))
                {
                    var box = output[i].Get(context);
                    oB.Connections.Add(box);
                    box.Connections.Add(oB);
                }
            }

            toUpdate.AddRange(iB.Connections);
            toUpdate.AddRange(oB.Connections);

            foreach (var box in toUpdate)
            {
                box.OnConnectionsChanged();
                box.ConnectionTick();
            }

            context.MarkAsModified();
        }

        /// <inheritdoc />
        public void Dispose()
        {
            _surface = null;
            _inputBefore = null;
            _inputAfter = null;
            _outputBefore = null;
            _outputAfter = null;
        }
    }
}
