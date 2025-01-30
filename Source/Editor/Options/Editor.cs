// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System.Text;
using FlaxEditor.CustomEditors;
using FlaxEditor.CustomEditors.Editors;
using FlaxEngine;
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
            var editorClassName = typeof(T).Name;
            
            var editorName = new StringBuilder();
            editorName.Append(editorClassName[0]);
            for (var i = 1; i < editorClassName.Length; i++)
            {
                // Whenever there is an uppercase letter, add a space to make it more pretty for the end user
                if (char.IsUpper(editorClassName[i]))
                {
                    editorName.Append(' ');
                }
                editorName.Append(editorClassName[i]);
            }
            
            var result = MessageBox.Show($"Are you sure you want to reset \"{editorName}\" to default values?", 
                                         "Reset values?",
                                         MessageBoxButtons.YesNo
                                         );
            
            if (result == DialogResult.Yes || result == DialogResult.OK)
            {
                var obj = new T();
                var str = JsonSerializer.Serialize(obj);
                JsonSerializer.Deserialize(Values[0], str);
                SetValue(Values[0]);
                Refresh();
            }
        }
    }
}
