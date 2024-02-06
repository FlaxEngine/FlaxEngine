using System;
using FlaxEngine;

namespace FlaxEditor.CustomEditors.Editors
{
    /// <summary>
    /// Custom editor for <see cref="GamepadButton"/>.
    /// Allows capturing gamepad buttons and assigning them
    /// to the edited value.
    /// </summary>
    [CustomEditor(typeof(GamepadButton))]
    public class GamepadButtonEditor : BindableButtonEditor
    {
        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            base.Initialize(layout);
            FlaxEngine.Scripting.Update += OnUpdate;
        }

        /// <inheritdoc />
        protected override void Deinitialize()
        {
            FlaxEngine.Scripting.Update -= OnUpdate;
            base.Deinitialize();
        }

        private void OnUpdate()
        {
            if (!IsListeningForInput)
                return;

            // Since there is no way to get an event about
            // which gamepad pressed what button, we have
            // to poll all gamepads and buttons manually.
            for (var i = 0; i < Input.GamepadsCount; i++)
            {
                var pad = Input.Gamepads[i];
                foreach (var btn in Enum.GetValues<GamepadButton>())
                {
                    if (pad.GetButtonUp(btn))
                    {
                        IsListeningForInput = false;
                        SetValue(btn);
                        return;
                    }
                }
            }
        }
    }
}
