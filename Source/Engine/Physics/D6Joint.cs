// Copyright (c) Wojciech Figat. All rights reserved.

using System.ComponentModel;

namespace FlaxEngine
{
    partial struct D6JointDrive
    {
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
    partial class D6Joint
    {
#if FLAX_EDITOR
        internal struct D6JointAxisMotion
        {
            [EditorOrder(0), DefaultValue(D6JointMotion.Locked)]
            [Tooltip("Movement on the X axis.")]
            public D6JointMotion X;

            [EditorOrder(1), DefaultValue(D6JointMotion.Locked)]
            [Tooltip("Movement on the Y axis.")]
            public D6JointMotion Y;

            [EditorOrder(2), DefaultValue(D6JointMotion.Locked)]
            [Tooltip("Movement on the Z axis.")]
            public D6JointMotion Z;

            [EditorOrder(2), DefaultValue(D6JointMotion.Locked)]
            [Tooltip("Rotation around the X axis.")]
            public D6JointMotion Twist;

            [EditorOrder(4), DefaultValue(D6JointMotion.Locked)]
            [Tooltip("Rotation around the Y axis."), EditorDisplay(null, "Swing Y")]
            public D6JointMotion SwingY;

            [EditorOrder(5), DefaultValue(D6JointMotion.Locked)]
            [Tooltip("Rotation around the Z axis."), EditorDisplay(null, "Swing Z")]
            public D6JointMotion SwingZ;
        }

        [ShowInEditor, Serialize, EditorOrder(100), EditorDisplay("Joint")]
        [Tooltip("Joint motion options (per-axis).")]
        internal D6JointAxisMotion AxisMotion
        {
            get => new D6JointAxisMotion
            {
                X = GetMotion(D6JointAxis.X),
                Y = GetMotion(D6JointAxis.Y),
                Z = GetMotion(D6JointAxis.Z),
                Twist = GetMotion(D6JointAxis.Twist),
                SwingY = GetMotion(D6JointAxis.SwingY),
                SwingZ = GetMotion(D6JointAxis.SwingZ)
            };
            set
            {
                SetMotion(D6JointAxis.X, value.X);
                SetMotion(D6JointAxis.Y, value.Y);
                SetMotion(D6JointAxis.Z, value.Z);
                SetMotion(D6JointAxis.Twist, value.Twist);
                SetMotion(D6JointAxis.SwingY, value.SwingY);
                SetMotion(D6JointAxis.SwingZ, value.SwingZ);
            }
        }
#endif
    }
}
