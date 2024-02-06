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

            var panel = layout.CustomContainer<UniformGridPanel>();
            panel.CustomControl.SlotsHorizontally = 2;
            panel.CustomControl.SlotsVertically = 1;

            var button = panel.Button("Listen", "Press to listen for input events");
            _button = button.Button;
            _button.Clicked += OnButtonClicked;
            ResetButton();

            var padding = panel.CustomControl.SlotPadding;
            panel.CustomControl.Height = ComboBox.DefaultHeight + padding.Height;

            base.Initialize(panel);
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
