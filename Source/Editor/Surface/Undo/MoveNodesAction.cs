// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using System;
using FlaxEngine;

namespace FlaxEditor.Surface.Undo
{
    /// <summary>
    /// Move Visject Surface nodes undo action.
    /// </summary>
    /// <seealso cref="FlaxEditor.IUndoAction" />
    class MoveNodesAction : IUndoAction
    {
        private VisjectSurface _surface;
        private ContextHandle _context;
        private uint[] _nodeIds;
        private readonly Vector2 _locationDelta;

        public MoveNodesAction(VisjectSurfaceContext context, uint[] nodeIds, Vector2 locationDelta)
        {
            _surface = context.Surface;
            _context = new ContextHandle(context);
            _nodeIds = nodeIds;
            _locationDelta = locationDelta;
        }

        /// <inheritdoc />
        public string ActionString => "Move nodes";

        /// <inheritdoc />
        public void Do()
        {
            Apply(_locationDelta);
        }

        /// <inheritdoc />
        public void Undo()
        {
            Apply(-_locationDelta);
        }

        private void Apply(Vector2 delta)
        {
            var context = _context.Get(_surface);
            foreach (var nodeId in _nodeIds)
            {
                var node = context.FindNode(nodeId);
                if (node == null)
                    throw new Exception("Missing node.");

                node.Location += delta;
            }

            context.MarkAsModified(false);
        }

        /// <inheritdoc />
        public void Dispose()
        {
            _surface = null;
            _nodeIds = null;
        }
    }
}
