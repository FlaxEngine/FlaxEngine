// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEditor.SceneGraph;
using FlaxEditor.Viewport;
using FlaxEngine;
using System.Collections.Generic;

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
        /// Deletes selected objects.
        /// </summary>
        void DeleteSelection();

        /// <summary>
        /// Focuses selected objects.
        /// </summary>
        void FocusSelection();

        /// <summary>
        /// Spawns the specified actor to the game (with undo).
        /// </summary>
        /// <param name="actor">The actor.</param>
        /// <param name="parent">The parent actor. Set null as default.</param>
        /// <param name="orderInParent">The order under the parent to put the spawned actor.</param>
        /// <param name="autoSelect">True if automatically select the spawned actor, otherwise false.</param>
        void Spawn(Actor actor, Actor parent = null, int orderInParent = -1, bool autoSelect = true);
    }
}
