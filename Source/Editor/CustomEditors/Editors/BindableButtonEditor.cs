using FlaxEditor.CustomEditors.Elements;
using FlaxEditor.GUI;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.CustomEditors.Editors
{
    /// <summary>
    /// Base class for custom button editors.
    /// See <seealso cref="MouseButtonEditor"/>, <seealso cref="KeyboardKeysEditor"/> and <seealso cref="GamepadButtonEditor"/>.
    /// </summary>
    public class BindableButtonEditor : EnumEditor
    {
        private bool _isListeningForInput;
        private Button _button;

        /// <summary>
        /// Where or not we are currently listening for any input.
        /// </summary>
        protected bool IsListeningForInput
        {
            get => _isListeningForInput;
            set
            {
                _isListeningForInput = value;
                if (_isListeningForInput)
                    SetupButton();
                else
                    ResetButton();
            }
        }

        /// <summary>
        /// The window this editor is attached to.
        /// Useful to hook into for key pressed, mouse buttons etc.
        /// </summary>
        protected Window Window { get; private set; }

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            Window = layout.Control.RootWindow.Window;
            var panelElement = layout.CustomContainer<Panel>();
            var panel = panelElement.ContainerControl as Panel;

            var button = panelElement.Button("Listen", "Press to listen for input events");
            _button = button.Button;
            _button.Width = FlaxEngine.GUI.Style.Current.FontMedium.MeasureText("Listening...").X + 8;
            _button.Height = ComboBox.DefaultHeight;
            _button.Clicked += OnButtonClicked;
            ResetButton();

            panel.Height = ComboBox.DefaultHeight;

            base.Initialize(panelElement);

            if (panelElement.Children.Find(x => x is EnumElement) is EnumElement comboBoxElement)
            {
                var comboBox = comboBoxElement.ComboBox;
                comboBox.AnchorPreset = AnchorPresets.StretchAll;
                comboBox.Offsets = new Margin(0, _button.Width + 2, 0, 0);
                comboBox.LocalX += _button.Width + 2;
            }
        }

        /// <inheritdoc />
        protected override void Deinitialize()
        {
            _button.Clicked -= OnButtonClicked;
            base.Deinitialize();
        }

        private void ResetButton()
        {
            _button.Text = "Listen";
            _button.BorderThickness = 1;

            var style = FlaxEngine.GUI.Style.Current;
            _button.BorderColor = style.BorderNormal;
            _button.BorderColorHighlighted = style.BorderHighlighted;
            _button.BorderColorSelected = style.BorderSelected;
        }

        private void SetupButton()
        {
            _button.Text = "Listening...";
            _button.BorderThickness = 2;

            var color = FlaxEngine.GUI.Style.Current.ProgressNormal;
            _button.BorderColor = color;
            _button.BorderColorHighlighted = color;
            _button.BorderColorSelected = color;
        }

        private void OnButtonClicked()
        {
            _isListeningForInput = !_isListeningForInput;
            if (_isListeningForInput)
                SetupButton();
            else
                ResetButton();
        }
    }
}
