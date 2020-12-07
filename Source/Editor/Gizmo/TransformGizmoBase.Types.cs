// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

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
            None,

            /// <summary>
            /// The X axis.
            /// </summary>
            X,

            /// <summary>
            /// The Y axis.
            /// </summary>
            Y,

            /// <summary>
            /// The Z axis.
            /// </summary>
            Z,

            /// <summary>
            /// The XY plane.
            /// </summary>
            XY,

            /// <summary>
            /// The ZX plane.
            /// </summary>
            ZX,

            /// <summary>
            /// The YZ plane.
            /// </summary>
            YZ,

            /// <summary>
            /// The center point.
            /// </summary>
            Center,
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
