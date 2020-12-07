// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

namespace FlaxEngine
{
    /// <summary>
    /// Virtual input axis binding. Helps with listening for a selected axis input.
    /// </summary>
    public class InputAxis
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
        /// Initializes a new instance of the <see cref="InputAxis"/> class.
        /// </summary>
        public InputAxis()
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="InputAxis"/> class.
        /// </summary>
        /// <param name="name">The axis name.</param>
        public InputAxis(string name)
        {
            Name = name;
        }
    }
}
