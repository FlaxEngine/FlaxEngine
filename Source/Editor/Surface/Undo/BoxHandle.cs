// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.Surface.Elements;
using FlaxEngine;

namespace FlaxEditor.Surface.Undo
{
    /// <summary>
    /// The helper structure for Surface node box handle.
    /// </summary>
    [HideInEditor]
    public struct BoxHandle : IEquatable<BoxHandle>
    {
        private readonly uint _nodeId;
        private readonly int _boxId;

        /// <summary>
        /// Initializes a new instance of the <see cref="BoxHandle"/> struct.
        /// </summary>
        /// <param name="nodeId">The node identifier.</param>
        /// <param name="boxId">The box identifier.</param>
        public BoxHandle(uint nodeId, int boxId)
        {
            _nodeId = nodeId;
            _boxId = boxId;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="BoxHandle"/> struct.
        /// </summary>
        /// <param name="box">The box.</param>
        public BoxHandle(Box box)
        {
            _nodeId = box.ParentNode.ID;
            _boxId = box.ID;
        }

        /// <summary>
        /// Gets the box.
        /// </summary>
        /// <param name="context">The Surface context.</param>
        /// <returns>The restored box.</returns>
        public Box Get(VisjectSurfaceContext context)
        {
            var node = context.FindNode(_nodeId);
            if (node == null)
                throw new Exception("Missing node.");
            var box = node.GetBox(_boxId);
            if (box == null)
                throw new Exception("Missing box.");
            return box;
        }

        /// <inheritdoc />
        public override bool Equals(object obj)
        {
            return obj is BoxHandle handle && Equals(handle);
        }

        /// <inheritdoc />
        public bool Equals(BoxHandle other)
        {
            return _nodeId == other._nodeId &&
                   _boxId == other._boxId;
        }

        /// <inheritdoc />
        public override int GetHashCode()
        {
            unchecked
            {
                return (_nodeId.GetHashCode() * 397) ^ _boxId.GetHashCode();
            }
        }
    }
}
