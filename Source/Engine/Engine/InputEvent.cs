// Copyright (c) Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine
{
    /// <summary>
    /// Virtual input action binding. Helps with listening for a selected input event.
    /// </summary>
    public class InputEvent : IComparable, IComparable<InputEvent>
    {
        /// <summary>
        /// The name of the action to use. See <see cref="Input.ActionMappings"/>.
        /// </summary>
        [Tooltip("The name of the action to use.")]
        public string Name;

        /// <summary>
        /// Returns true if the event has been triggered during the current frame (e.g. user pressed a key). Use <see cref="Pressed"/> to catch events without active waiting.
        /// </summary>
        public bool Active => Input.GetAction(Name);

        /// <summary>
        /// Returns the event state. Use <see cref="Pressed"/>, <see cref="Pressing"/>, <see cref="Released"/> to catch events without active waiting.
        /// </summary>
        public InputActionState State => Input.GetActionState(Name);

        /// <summary>
        /// Occurs when event is triggered (e.g. user pressed a key). Called before scripts update.
        /// </summary>
        [System.Obsolete("Use Pressed instead")]
        public event Action Triggered;

        /// <summary>
        /// Occurs when event is pressed (e.g. user pressed a key). Called before scripts update.
        /// </summary>
        public event Action Pressed;

        /// <summary>
        /// Occurs when event is being pressing (e.g. user pressing a key). Called before scripts update.
        /// </summary>
        public event Action Pressing;

        /// <summary>
        /// Occurs when event is released (e.g. user releases a key). Called before scripts update.
        /// </summary>
        public event Action Released;

        /// <summary>
        /// Initializes a new instance of the <see cref="InputEvent"/> class.
        /// </summary>
        public InputEvent()
        {
            Input.ActionTriggered += Handler;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="InputEvent"/> class.
        /// </summary>
        /// <param name="name">The action name.</param>
        public InputEvent(string name)
        {
            Input.ActionTriggered += Handler;
            Name = name;
        }

        /// <summary>
        /// Finalizes an instance of the <see cref="InputEvent"/> class.
        /// </summary>
        ~InputEvent()
        {
            Input.ActionTriggered -= Handler;
            Triggered = null;
            Pressed = null;
            Pressing = null;
            Released = null;
        }

        private void Handler(string name, InputActionState state)
        {
            if (!string.Equals(name, Name, StringComparison.OrdinalIgnoreCase))
                return;
            switch (state)
            {
            case InputActionState.None: break;
            case InputActionState.Waiting: break;
            case InputActionState.Pressing:
                Pressing?.Invoke();
                break;
            case InputActionState.Press:
                Triggered?.Invoke();
                Pressed?.Invoke();
                break;
            case InputActionState.Release:
                Released?.Invoke();
                break;
            default: break;
            }
        }

        /// <summary>
        /// Releases this object.
        /// </summary>
        public void Dispose()
        {
            Input.ActionTriggered -= Handler;
            Triggered = null;
            Pressed = null;
            Pressing = null;
            Released = null;
            GC.SuppressFinalize(this);
        }

        /// <inheritdoc />
        public int CompareTo(InputEvent other)
        {
            return string.Compare(Name, other.Name, StringComparison.Ordinal);
        }

        /// <inheritdoc />
        public int CompareTo(object obj)
        {
            return obj is InputEvent other ? CompareTo(other) : -1;
        }

        /// <inheritdoc />
        public override int GetHashCode()
        {
            return Name?.GetHashCode() ?? 0;
        }

        /// <inheritdoc />
        public override bool Equals(object obj)
        {
            return obj is InputEvent other && string.Equals(Name, other.Name, StringComparison.Ordinal);
        }

        /// <inheritdoc />
        public override string ToString()
        {
            return Name;
        }
    }
}
