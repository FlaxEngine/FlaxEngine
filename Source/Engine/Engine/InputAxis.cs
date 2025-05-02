// Copyright (c) Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine
{
    /// <summary>
    /// Virtual input axis binding. Helps with listening for a selected axis input.
    /// </summary>
    public class InputAxis : IComparable, IComparable<InputAxis>
    {
        /// <summary>
        /// The name of the axis to use. See <see cref="Input.AxisMappings"/>.
        /// </summary>
        [Tooltip("The name of the axis to use.")]
        public string Name;

        /// <summary>
        /// Gets the current axis value.
        /// </summary>
        public float Value => Input.GetAxis(Name);

        /// <summary>
        /// Gets the current axis raw value.
        /// </summary>
        public float ValueRaw => Input.GetAxisRaw(Name);

        /// <summary>
        /// Occurs when axis is changed. Called before scripts update.
        /// </summary>
        public event Action ValueChanged;

        /// <summary>
        /// Initializes a new instance of the <see cref="InputAxis"/> class.
        /// </summary>
        public InputAxis()
        {
            Input.AxisValueChanged += Handler;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="InputAxis"/> class.
        /// </summary>
        /// <param name="name">The axis name.</param>
        public InputAxis(string name)
        {
            Input.AxisValueChanged += Handler;
            Name = name;
        }

        private void Handler(string name)
        {
            if (string.Equals(Name, name, StringComparison.OrdinalIgnoreCase))
                ValueChanged?.Invoke();
        }

        /// <summary>
        /// Finalizes an instance of the <see cref="InputAxis"/> class.
        /// </summary>
        ~InputAxis()
        {
            Input.AxisValueChanged -= Handler;
            ValueChanged = null;
        }

        /// <summary>
        /// Releases this object.
        /// </summary>
        public void Dispose()
        {
            Input.AxisValueChanged -= Handler;
            ValueChanged = null;
            GC.SuppressFinalize(this);
        }

        /// <inheritdoc />
        public int CompareTo(InputAxis other)
        {
            return string.Compare(Name, other.Name, StringComparison.Ordinal);
        }

        /// <inheritdoc />
        public int CompareTo(object obj)
        {
            return obj is InputAxis other ? CompareTo(other) : -1;
        }

        /// <inheritdoc />
        public override int GetHashCode()
        {
            return Name?.GetHashCode() ?? 0;
        }

        /// <inheritdoc />
        public override bool Equals(object obj)
        {
            return obj is InputAxis other && string.Equals(Name, other.Name, StringComparison.Ordinal);
        }

        /// <inheritdoc />
        public override string ToString()
        {
            return Name;
        }
    }
}
