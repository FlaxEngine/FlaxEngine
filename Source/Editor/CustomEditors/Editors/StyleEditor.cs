// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEditor.CustomEditors.Elements;
using FlaxEditor.GUI;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.CustomEditors.Editors
{
    /// <summary>
    /// Default implementation of the inspector used to edit styles.
    /// </summary>
    [CustomEditor(typeof(Style)), DefaultEditor]
    public class StyleEditor : CustomEditor
    {
        private CustomElement<StyleValueEditor> _element;

        /// <inheritdoc />
        public override DisplayStyle Style => DisplayStyle.Inline;

        /// <summary>
        /// Initializes this editor.
        /// </summary>
        /// <param name="layout">The layout builder.</param>
        public override void Initialize(LayoutElementsContainer layout)
        {
            var style = (Style)Values[0];

            _element = layout.Custom<StyleValueEditor>();
            _element.CustomControl.Value = style;
            _element.CustomControl.ValueChanged += OnValueChanged;
        }

        private void OnValueChanged()
        {
            SetValue(_element.CustomControl.Value);
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            base.Refresh();

            if (HasDifferentValues)
            {
            }
            else
            {
                _element.CustomControl.Value = (Style)Values[0];
            }
        }
    }
}
