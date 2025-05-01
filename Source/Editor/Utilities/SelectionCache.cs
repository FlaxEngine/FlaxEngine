// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEditor.SceneGraph;

namespace FlaxEditor.Utilities
{
    /// <summary>
    /// Helper utility object that caches the selection of the editor and can restore it later. Works only for objects of <see cref="FlaxEngine.Object"/> type.
    /// </summary>
    public sealed class SelectionCache
    {
        private readonly List<Guid> _objects = new List<Guid>();

        /// <summary>
        /// Caches selection.
        /// </summary>
        public void Cache()
        {
            _objects.Clear();

            var selection = Editor.Instance.SceneEditing.Selection;
            for (int i = 0; i < selection.Count; i++)
            {
                _objects.Add(selection[i].ID);
            }
        }

        /// <summary>
        /// Restores selection.
        /// </summary>
        public void Restore()
        {
            if (_objects.Count == 0)
            {
                Editor.Instance.SceneEditing.Deselect();
                return;
            }

            var selection = new List<SceneGraph.SceneGraphNode>(_objects.Count);

            for (int i = 0; i < _objects.Count; i++)
            {
                var id = _objects[i];
                var node = SceneGraphFactory.FindNode(id);
                if (node != null)
                    selection.Add(node);
            }

            _objects.Clear();

            Editor.Instance.SceneEditing.Select(selection);
        }
    }
}
