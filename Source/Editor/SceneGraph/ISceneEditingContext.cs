// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using FlaxEditor.SceneGraph;
using FlaxEditor.Viewport;

namespace FlaxEditor
{
    /// <summary>
    /// Shared interface for scene editing utilities.
    /// </summary>
    public interface ISceneEditingContext
    {
        /// <summary>
        /// Gets the main or last editor viewport used for scene editing within this context.
        /// </summary>
        EditorViewport Viewport { get; }

        /// <summary>
        /// Gets the list of selected scene graph nodes in the editor context.
        /// </summary>
        List<SceneGraphNode> Selection { get; }

        /// <summary>
        /// Selects the specified node.
        /// </summary>
        /// <param name="node">The node.</param>
        /// <param name="additive">if set to <c>true</c> will use additive mode, otherwise will clear previous selection.</param>
        void Select(SceneGraphNode node, bool additive = false);

        /// <summary>
        /// Deselects node.
        /// </summary>
        /// <param name="node">The node to deselect.</param>
        void Deselect(SceneGraphNode node);

        /// <summary>
        /// Opends popup for renaming selected objects.
        /// </summary>
        void RenameSelection();

        /// <summary>
        /// Focuses selected objects.
        /// </summary>
        void FocusSelection();
    }
}
