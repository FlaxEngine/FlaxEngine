// Copyright (c) Wojciech Figat. All rights reserved.

using System;

namespace FlaxEditor.Surface.Undo
{
    /// <summary>
    /// Edit Visject Surface node values collection undo action.
    /// </summary>
    /// <seealso cref="FlaxEditor.IUndoAction" />
    class EditNodeValuesAction : IUndoAction
    {
        private VisjectSurface _surface;
        private ContextHandle _context;
        private readonly uint _nodeId;
        private readonly bool _graphEdited;
        private readonly bool _resize;
        private object[] _before;
        private object[] _after;

        public EditNodeValuesAction(SurfaceNode node, object[] before, bool graphEdited)
        {
            if (before == null)
                throw new ArgumentNullException(nameof(before));
            if (node?.Values == null)
                throw new ArgumentNullException(nameof(node));
            _resize = before.Length != node.Values.Length;
            if (_resize && (node.Archetype.Flags & NodeFlags.VariableValuesSize) == 0)
                throw new ArgumentException(nameof(before));

            _surface = node.Surface;
            _context = new ContextHandle(node.Context);
            _nodeId = node.ID;
            _before = before;
            _after = (object[])node.Values.Clone();
            _graphEdited = graphEdited;
        }

        /// <inheritdoc />
        public string ActionString => "Edit node";

        /// <inheritdoc />
        public void Do()
        {
            var context = _context.Get(_surface);
            if (_after == null)
                throw new Exception("Missing values.");
            var node = context.FindNode(_nodeId);
            if (node == null)
                throw new Exception("Missing node.");

            node.SetIsDuringValuesEditing(true);
            if (_resize)
                node.Values = (object[])_after.Clone();
            else
                Array.Copy(_after, node.Values, _after.Length);
            node.OnValuesChanged();
            context.MarkAsModified(_graphEdited);
            node.SetIsDuringValuesEditing(false);
        }

        /// <inheritdoc />
        public void Undo()
        {
            var context = _context.Get(_surface);
            if (_before == null)
                throw new Exception("Missing values.");
            var node = context.FindNode(_nodeId);
            if (node == null)
                throw new Exception("Missing node.");

            node.SetIsDuringValuesEditing(true);
            if (_resize)
                node.Values = (object[])_before.Clone();
            else
                Array.Copy(_before, node.Values, _before.Length);
            node.OnValuesChanged();
            context.MarkAsModified(_graphEdited);
            node.SetIsDuringValuesEditing(false);
        }

        /// <inheritdoc />
        public void Dispose()
        {
            _surface = null;
            _before = null;
            _after = null;
        }
    }
}
