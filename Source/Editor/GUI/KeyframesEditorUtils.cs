// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Text;
using FlaxEngine;

namespace FlaxEditor.GUI
{
    /// <summary>
    /// Utilities for <see cref="IKeyframesEditor"/> and <see cref="IKeyframesEditorContext"/>
    /// </summary>
    internal static class KeyframesEditorUtils
    {
        public const string CopyPrefix = "KEYFRAMES!?";

        public static void Copy(IKeyframesEditor editor, float? timeOffset = null)
        {
            var data = new StringBuilder();
            if (editor.KeyframesEditorContext != null)
                editor.KeyframesEditorContext.OnKeyframesCopy(editor, timeOffset, data);
            else
                editor.OnKeyframesCopy(editor, timeOffset, data);
            Clipboard.Text = data.ToString();
        }

        public static bool CanPaste()
        {
            return Clipboard.Text.Contains(CopyPrefix);
        }

        public static void Paste(IKeyframesEditor editor, float? timeOffset = null)
        {
            var data = Clipboard.Text;
            if (string.IsNullOrEmpty(data))
                return;
            var datas = data.Split(new[] { CopyPrefix }, StringSplitOptions.RemoveEmptyEntries);
            if (datas.Length == 0)
                return;
            if (editor.KeyframesEditorContext != null)
                editor.KeyframesEditorContext.OnKeyframesDeselect(editor);
            else
                editor.OnKeyframesDeselect(editor);
            int index = -1;
            if (editor.KeyframesEditorContext != null)
                editor.KeyframesEditorContext.OnKeyframesPaste(editor, timeOffset, datas, ref index);
            else
                editor.OnKeyframesPaste(editor, timeOffset, datas, ref index);
        }
    }
}
