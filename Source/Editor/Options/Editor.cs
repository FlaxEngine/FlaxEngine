// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using FlaxEditor.CustomEditors;
using FlaxEditor.CustomEditors.Editors;
using FlaxEngine.GUI;
using FlaxEngine.Json;

namespace FlaxEditor.Options
{
    /// <summary>
    /// Editor options editor.
    /// </summary>
    public class Editor<T> : GenericEditor
    where T : new()
    {
        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            base.Initialize(layout);

            // Reset options button
            layout.Space(30);
            var panel = layout.Space(30);
            var resetButton = new Button(4, 4, 100)
            {
                Text = "Reset",
                Parent = panel.ContainerControl
            };
            resetButton.Clicked += OnResetButtonClicked;
        }

        private void OnResetButtonClicked()
        {
            var obj = new T();
            var str = JsonSerializer.Serialize(obj);
            JsonSerializer.Deserialize(Values[0], str);
            SetValue(Values[0]);
            Refresh();
        }
    }
}
