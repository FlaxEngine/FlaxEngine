// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using FlaxEditor.CustomEditors;
using FlaxEditor.CustomEditors.Editors;
using FlaxEditor.GUI;
using FlaxEditor.GUI.Input;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEngine.Utilities;

namespace FlaxEditor.Windows
{
    /// <summary>
    /// Editor window for UI <see cref="Style"/>.
    /// </summary>
    /// <seealso cref="FlaxEditor.Windows.EditorWindow" />
    public class StyleEditorWindow : EditorWindow
    {
        private const float ButtonsWidth = 60.0f;
        private const float PickerMargin = 6.0f;
        private const float PreviewX = 50;

        private Style _initialValue;
        private Style _value;

        private bool _useDynamicEditing;
        private StyleValueEditor.ValueChangedEvent _onChanged;

        private Button _cCancel;
        private Button _cOK;
        private CustomEditorPresenter _valueEditor;

        private Panel _previewPanel;

        /// <summary>
        /// Initializes a new instance of the <see cref="StyleEditorWindow"/> class.
        /// </summary>
        /// <param name="editor"></param>
        /// <param name="initialValue">The initial value.</param>
        /// <param name="valueChanged">The changed event.</param>
        /// <param name="useDynamicEditing">True if allow dynamic value editing (slider-like usage), otherwise will change event only on editing end.</param>
        public StyleEditorWindow(Editor editor, Style initialValue, StyleValueEditor.ValueChangedEvent valueChanged, bool useDynamicEditing) : base(editor, false, ScrollBars.None)
        {
            Title = "Style";
            _initialValue = initialValue;
            _value = _initialValue.DeepClone();
            _useDynamicEditing = useDynamicEditing;
            _onChanged = valueChanged;

            var container = AddChild<UniformGridPanel>();
            container.Offsets = Margin.Zero;
            container.AnchorPreset = AnchorPresets.StretchAll;
            container.SlotsVertically = 1;
            container.SlotsHorizontally = 2;

            var optionsPanel = container.AddChild<Panel>();
            container.Offsets = Margin.Zero;
            container.AnchorPreset = AnchorPresets.StretchAll;
            optionsPanel.IsScrollable = true;
            optionsPanel.ScrollBars = ScrollBars.Vertical;

            _valueEditor = new CustomEditorPresenter(null);
            _valueEditor.Panel.Parent = optionsPanel;
            _valueEditor.OverrideEditor = new GenericEditor();
            _valueEditor.Select(_value);
            _valueEditor.Modified += OnEdited;

            _previewPanel = container.AddChild<Panel>();
            container.Offsets = Margin.Zero;
            container.AnchorPreset = AnchorPresets.StretchAll;

            var preview = CreatePreview(_value);
            preview.Parent = _previewPanel;

            // Cancel
            _cCancel = new Button
            {
                Text = "Cancel",
                Parent = this,
                AnchorPreset = AnchorPresets.BottomRight,
                Bounds = new Rectangle(Width - ButtonsWidth - PickerMargin, Height - Button.DefaultHeight - PickerMargin, ButtonsWidth, Button.DefaultHeight),
            };
            _cCancel.Clicked += OnCancelClicked;

            // OK
            _cOK = new Button
            {
                Text = "Ok",
                Parent = this,
                AnchorPreset = AnchorPresets.BottomRight,
                Bounds = new Rectangle(_cCancel.Left - ButtonsWidth - PickerMargin, _cCancel.Y, ButtonsWidth, Button.DefaultHeight),
            };
            _cOK.Clicked += OnOkClicked;
        }

        /// <summary>
        /// Creates a preview
        /// </summary>
        /// <param name="style">The style to use for the preview</param>
        /// <returns>The preview</returns>
        private static ContainerControl CreatePreview(Style style)
        {
            var currentStyle = Style.Current;
            Style.Current = style;

            var preview = new ContainerControl
            {
                Offsets = Margin.Zero,
                AnchorPreset = AnchorPresets.StretchAll,
                BackgroundColor = style.Background
            };

            var label = new Label
            {
                Text = "Example Label",
                Parent = preview,
                Location = new Float2(PreviewX, 50),
                TooltipText = "Example Tooltip"
            };

            var button = new Button(PreviewX, 100)
            {
                Text = "Example Button",
                Parent = preview,
                TooltipText = "Example Tooltip"
            };

            var textBox = new TextBox(true, PreviewX, 150)
            {
                Text = "Example TextBox",
                Parent = preview
            };

            var checkBox = new CheckBox(PreviewX, 200)
            {
                Parent = preview
            };

            var panel = new Panel
            {
                Parent = preview,
                X = PreviewX,
                Y = 250,
                BackgroundColor = style.BackgroundSelected,
                Width = 250,
                Height = 30
            };

            var progressBar = new ProgressBar(20, 5, 150, 20)
            {
                Value = 42,
                Parent = panel
            };

            var comboBox = new ComboBox
            {
                Items = new List<string>
                {
                    "Item 1",
                    "Item 2",
                    "Item 3"
                },
                X = PreviewX,
                Y = 300,
                Parent = preview,
                SelectedIndex = 0,
                SelectedItem = "Item 1"
            };

            var slider = new SliderControl(30, PreviewX, 350, min: 0, max: 100)
            {
                Parent = preview,
                Value = 31
            };

            Style.Current = currentStyle;
            return preview;
        }

        /// <summary>
        /// Gets called when the style has been edited
        /// </summary>
        private void OnEdited()
        {
            if (_previewPanel != null)
            {
                _previewPanel.DisposeChildren();
                var preview = CreatePreview(_value);
                preview.Parent = _previewPanel;
            }
            if (_useDynamicEditing)
            {
                _useDynamicEditing = false;
                _onChanged?.Invoke(_value, true);
            }
        }

        private void OnOkClicked()
        {
            _onChanged?.Invoke(_value, false);
            Close();
        }

        private void OnCancelClicked()
        {
            // Restore old style
            if (_useDynamicEditing)
                _onChanged?.Invoke(_initialValue, false);

            Close(ClosingReason.User);
        }

        /// <inheritdoc />
        protected override void OnClose()
        {
            base.OnClose();

            if (_useDynamicEditing)
                _onChanged?.Invoke(_initialValue, false);
        }
    }
}
