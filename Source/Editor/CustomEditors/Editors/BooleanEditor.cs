// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEditor.CustomEditors.Elements;
using FlaxEngine;

namespace FlaxEditor.CustomEditors.Editors
{
    /// <summary>
    /// Default implementation of the inspector used to edit bool value type properties.
    /// </summary>
    [CustomEditor(typeof(bool)), DefaultEditor]
    public sealed class BooleanEditor : CustomEditor
    {
        private CheckBoxElement element;

        /// <inheritdoc />
        public override DisplayStyle Style => DisplayStyle.Inline;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            element = layout.Checkbox();
            element.CheckBox.StateChanged += (box) => SetValue(box.Checked);
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            base.Refresh();

            if (HasDifferentValues)
            {
                element.CheckBox.Intermediate = true;
            }
            else
            {
                var value = (bool?)Values[0];
                if (value != null)
                    element.CheckBox.Checked = value.Value;
            }
        }
    }
}
