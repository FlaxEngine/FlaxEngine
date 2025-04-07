// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
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

        /// <summary>
        /// Called when keyframes selection should be moved.
        /// </summary>
        /// <param name="editor">The source editor.</param>
        /// <param name="control">The source movement control.</param>
        /// <param name="location">The source movement location (in source control local space).</param>
        /// <param name="start">The movement start flag.</param>
        /// <param name="end">The movement end flag.</param>
        void OnKeyframesMove(IKeyframesEditor editor, ContainerControl control, Float2 location, bool start, bool end);

        /// <summary>
        /// Called when keyframes selection should be copied.
        /// </summary>
        /// <param name="editor">The source editor.</param>
        /// <param name="timeOffset">The additional time offset to apply to the copied keyframes (optional).</param>
        /// <param name="data">The result copy data text stream.</param>
        void OnKeyframesCopy(IKeyframesEditor editor, float? timeOffset, System.Text.StringBuilder data);

        /// <summary>
        /// Called when keyframes should be pasted (from clipboard).
        /// </summary>
        /// <param name="editor">The source editor.</param>
        /// <param name="timeOffset">The additional time offset to apply to the pasted keyframes (optional).</param>
        /// <param name="datas">The pasted data text.</param>
        /// <param name="index">The counter for the current data index. Set to -1 until the calling editor starts paste operation.</param>
        void OnKeyframesPaste(IKeyframesEditor editor, float? timeOffset, string[] datas, ref int index);

        /// <summary>
        /// Called when collecting keyframes from the context.
        /// </summary>
        /// <param name="get">The getter function to call for all keyframes. Args are: track name, keyframe time, keyframe object.</param>
        void OnKeyframesGet(Action<string, float, object> get);

        /// <summary>
        /// Called when setting keyframes data (opposite to <see cref="OnKeyframesGet"/>).
        /// </summary>
        /// <param name="keyframes">The list of keyframes, null for empty list. Keyframe data is: time and keyframe object.</param>
        void OnKeyframesSet(List<KeyValuePair<float, object>> keyframes);
    }
}
