// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI
{
    /// <summary>
    /// Interface for context for collection of <see cref="IKeyframesEditor"/>.
    /// </summary>
    public interface IKeyframesEditorContext
    {
        /// <summary>
        /// Called when keyframes selection should be cleared for all editors.
        /// </summary>
        /// <param name="editor">The source editor.</param>
        void OnKeyframesDeselect(IKeyframesEditor editor);

        /// <summary>
        /// Called when keyframes selection rectangle gets updated.
        /// </summary>
        /// <param name="editor">The source editor.</param>
        /// <param name="control">The source selection control.</param>
        /// <param name="selection">The source selection rectangle (in source control local space).</param>
        void OnKeyframesSelection(IKeyframesEditor editor, ContainerControl control, Rectangle selection);

        /// <summary>
        /// Called to calculate the total amount of selected keyframes in the editor.
        /// </summary>
        /// <returns>The selected keyframes amount.</returns>
        int OnKeyframesSelectionCount();

        /// <summary>
        /// Called when keyframes selection should be deleted for all editors.
        /// </summary>
        /// <param name="editor">The source editor.</param>
        void OnKeyframesDelete(IKeyframesEditor editor);
    }
}
