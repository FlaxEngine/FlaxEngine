using FlaxEngine;

namespace FlaxEditor.CustomEditors.Editors
{
    /// <summary>
    /// Custom editor for <see cref="MouseButton"/>.
    /// Allows capturing mouse button presses and assigning them
    /// to the edited value.
    /// </summary>
    [CustomEditor(typeof(MouseButton))]
    public class MouseButtonEditor : BindableButtonEditor
    {
        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            base.Initialize(layout);
            Window.MouseUp += OnMouseUp;
        }

        /// <inheritdoc />
        protected override void Deinitialize()
        {
            Window.MouseUp -= OnMouseUp;
            base.Deinitialize();
        }

        private void OnMouseUp(ref Float2 mouse, MouseButton button, ref bool handled)
        {
            if (!IsListeningForInput)
                return;
            IsListeningForInput = false;

            SetValue(button);
            handled = true;
        }
    }
}
