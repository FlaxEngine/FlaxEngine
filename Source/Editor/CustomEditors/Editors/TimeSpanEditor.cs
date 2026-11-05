// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;
using FlaxEngine.GUI;
using System;

namespace FlaxEditor.CustomEditors.Editors
{
    /// <summary>
    /// Default implementation of the inspector used to edit TimeSpan value type properties.
    /// </summary>
    [CustomEditor(typeof(TimeSpan)), DefaultEditor]
    class TimeSpanEditor : CustomEditor
    {
        private TextBox _textBox;
        private bool _isRefreshing;

        /// <inheritdoc />
        public override DisplayStyle Style => DisplayStyle.Inline;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            if (HasDifferentTypes)
                return;

            _textBox = layout.Custom<TextBox>().CustomControl;
            _textBox.EditEnd += OnEditEnd;
        }

        private void OnEditEnd()
        {
            if (_isRefreshing)
                return;
            if (TimeSpan.TryParse(_textBox.Text, out var timeSpan))
                SetValue(timeSpan);
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            base.Refresh();

            _isRefreshing = true;
            _textBox.Text = HasDifferentValues ? "Multiple Values" : ((TimeSpan)Values[0]).ToString("G");
            _isRefreshing = false;
        }
    }

    /// <summary>
    /// Default implementation of the inspector used to edit DateTime value type properties.
    /// </summary>
    [CustomEditor(typeof(DateTime)), DefaultEditor]
    class DateTimeEditor : CustomEditor
    {
        private TextBox _textBox;
        private bool _isRefreshing;

        /// <inheritdoc />
        public override DisplayStyle Style => DisplayStyle.Inline;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            if (HasDifferentTypes)
                return;

            _textBox = layout.Custom<TextBox>().CustomControl;
            _textBox.EditEnd += OnEditEnd;
        }

        private void OnEditEnd()
        {
            if (_isRefreshing)
                return;
            if (DateTime.TryParse(_textBox.Text, out var timeSpan))
                SetValue(timeSpan);
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            base.Refresh();

            _isRefreshing = true;
            _textBox.Text = HasDifferentValues ? "Multiple Values" : ((DateTime)Values[0]).ToString("g");
            _isRefreshing = false;
        }
    }
}
