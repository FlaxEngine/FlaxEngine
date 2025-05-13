using System;
using FlaxEngine;

namespace FlaxEditor.Options
{
    /// <summary>
    /// The input trigger, either mouse or keyboard
    /// </summary>
    [Serializable]
    public struct InputTrigger : IEquatable<InputTrigger>
    {
        public enum InputType { Mouse, Key, None }
        public MouseButton Button;
        public KeyboardKeys Key;
        public InputType Type;
        public bool IsKeyboard => Type == InputType.Key;
        public bool IsMouse => Type == InputType.Mouse;
        public InputTrigger(string token)
        {
            if (!TryParseInput(token, out var parsed))
                throw new ArgumentException($"Invalid input trigger token: {token}");

            this = parsed;
        }
        public override string ToString()
        {
            return Type switch
            {
                InputType.Key => Key.ToString(),
                InputType.Mouse => Button.ToString(),
                _ => string.Empty
            };
        }
        public static bool TryParseInput(string token, out InputTrigger trigger)
        {
            token = token.Trim(); // Always trim!

            if (Enum.TryParse<KeyboardKeys>(token, true, out var key))
            {
                trigger = new InputTrigger()
                {
                    Type = InputType.Key,
                    Button = MouseButton.None,
                    Key = key
                };
                return true;
            }

            if (Enum.TryParse<MouseButton>(token, true, out var button))
            {
                trigger = new InputTrigger()
                {
                    Type = InputType.Mouse,
                    Button = button,
                    Key = KeyboardKeys.None
                };
                return true;
            }
            trigger = new InputTrigger();
            Debug.Log("problem parsing " + token);
            return false;
        }
        /// <summary>
        /// Checks whether this individual input trigger is currently active.
        /// </summary>
        /// <param name="getKey">Function to check if a specific keyboard key is pressed.</param>
        /// <param name="getMouse">Function to check if a specific mouse button is pressed.</param>
        /// <returns>True if the trigger is currently active; otherwise, false.</returns>
        public bool IsPressed(Func<KeyboardKeys, bool> getKey, Func<MouseButton, bool> getMouse)
        {
            return Type switch
            {
                InputType.Key => getKey(Key),
                InputType.Mouse => getMouse(Button),
                _ => false
            };
        }

        public override bool Equals(object obj)
        {
            return obj is InputTrigger other && Equals(other);
        }
        public override int GetHashCode()
        {
            return Type switch
            {
                InputType.Key => HashCode.Combine(Type, (int)Key),
                InputType.Mouse => HashCode.Combine(Type, (int)Button),
                _ => Type.GetHashCode()
            };
        }

        public bool Equals(InputTrigger other)
        {
            return Type == other.Type &&
                   Key == other.Key &&
                   Button == other.Button;
        }

        public static bool operator ==(InputTrigger left, InputTrigger right)
        {
            return left.Equals(right);
        }
        public static bool operator !=(InputTrigger left, InputTrigger right)
        {
            return !left.Equals(right);
        }
    }
}
