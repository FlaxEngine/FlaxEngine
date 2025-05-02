// Copyright (c) Wojciech Figat. All rights reserved.

namespace FlaxEditor.Gizmo
{
    public partial class TransformGizmoBase
    {
        /// <summary>
        /// Gizmo axis modes.
        /// </summary>
        public enum Axis
        {
            /// <summary>
            /// None.
            /// </summary>
            None = 0,

            /// <summary>
            /// The X axis.
            /// </summary>
            X = 1,

            /// <summary>
            /// The Y axis.
            /// </summary>
            Y = 2,

            /// <summary>
            /// The Z axis.
            /// </summary>
            Z = 4,

            /// <summary>
            /// The XY plane.
            /// </summary>
            XY = X | Y,

            /// <summary>
            /// The ZX plane.
            /// </summary>
            ZX = Z | X,

            /// <summary>
            /// The YZ plane.
            /// </summary>
            YZ = Y | Z,

            /// <summary>
            /// The center point.
            /// </summary>
            Center = 8,
        };

        /// <summary>
        /// Gizmo tool mode.
        /// </summary>
        public enum Mode
        {
            /// <summary>
            /// Translate object(s)
            /// </summary>
            Translate,

            /// <summary>
            /// Rotate object(s)
            /// </summary>
            Rotate,

            /// <summary>
            /// Scale object(s)
            /// </summary>
            Scale
        }

        /// <summary>
        /// Transform object space.
        /// </summary>
        public enum TransformSpace
        {
            /// <summary>
            /// Object local space coordinates
            /// </summary>
            Local,

            /// <summary>
            /// World space coordinates
            /// </summary>
            World
        }

        /// <summary>
        /// Pivot location type.
        /// </summary>
        public enum PivotType
        {
            /// <summary>
            /// First selected object center
            /// </summary>
            ObjectCenter,

            /// <summary>
            /// Selection pool center point
            /// </summary>
            SelectionCenter,

            /// <summary>
            /// World origin
            /// </summary>
            WorldOrigin
        }
    }
}
