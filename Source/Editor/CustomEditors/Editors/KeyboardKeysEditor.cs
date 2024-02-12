using FlaxEngine;

namespace FlaxEditor.CustomEditors.Editors
{
    /// <summary>
    /// Custom editor for <see cref="KeyboardKeys"/>.
    /// Allows capturing key presses and assigning them
    /// to the edited value.
    /// </summary>
    [CustomEditor(typeof(KeyboardKeys))]
    public class KeyboardKeysEditor : BindableButtonEditor
    {
        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            base.Initialize(layout);
            Window.KeyUp += OnKeyUp;
        }

        /// <inheritdoc />
        protected override void Deinitialize()
        {
            Window.KeyUp -= OnKeyUp;
            base.Deinitialize();
        }

        private void OnKeyUp(KeyboardKeys key)
        {
            if (!IsListeningForInput)
                return;
            IsListeningForInput = false;

            SetValue(key);
        }
    }
}
