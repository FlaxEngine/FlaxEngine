// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

namespace FlaxEngine
{
    partial struct D6JointDrive
    {
        /// <summary>
        /// The default <see cref="D6JointDrive"/> structure.
        /// </summary>
        public static readonly D6JointDrive Default = new D6JointDrive(0.0f, 0.0f, float.MaxValue, false);

        /// <summary>
        /// Initializes a new instance of the <see cref="D6JointDrive"/> struct.
        /// </summary>
        /// <param name="stiffness">The stiffness.</param>
        /// <param name="damping">The damping.</param>
        /// <param name="forceLimit">The force limit.</param>
        /// <param name="acceleration">if set to <c>true</c> the drive will generate acceleration instead of forces.</param>
        public D6JointDrive(float stiffness, float damping, float forceLimit, bool acceleration)
        {
            Stiffness = stiffness;
            Damping = damping;
            ForceLimit = forceLimit;
            Acceleration = acceleration;
        }
    }
}
