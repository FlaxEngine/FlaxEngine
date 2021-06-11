// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.Scripting;
using FlaxEngine;

namespace FlaxEditor.Surface
{
    /// <summary>
    /// Describes GUI style used by the surface.
    /// </summary>
    [HideInEditor]
    public class SurfaceStyle
    {
        /// <summary>
        /// Description with the colors used by the surface.
        /// </summary>
        public struct ColorsData
        {
            /// <summary>
            /// The connecting nodes color.
            /// </summary>
            public Color Connecting;

            /// <summary>
            /// The connecting nodes color (for valid connection).
            /// </summary>
            public Color ConnectingValid;

            /// <summary>
            /// The connecting nodes color (for invalid connection).
            /// </summary>
            public Color ConnectingInvalid;

            /// <summary>
            /// The impulse boxes color.
            /// </summary>
            public Color Impulse;

            /// <summary>
            /// The boolean boxes color.
            /// </summary>
            public Color Bool;

            /// <summary>
            /// The integer boxes color.
            /// </summary>
            public Color Integer;

            /// <summary>
            /// The floating point boxes color.
            /// </summary>
            public Color Float;

            /// <summary>
            /// The vector boxes color.
            /// </summary>
            public Color Vector;

            /// <summary>
            /// The string boxes color.
            /// </summary>
            public Color String;

            /// <summary>
            /// The object boxes color.
            /// </summary>
            public Color Object;

            /// <summary>
            /// The rotation boxes color.
            /// </summary>
            public Color Rotation;

            /// <summary>
            /// The transform boxes color.
            /// </summary>
            public Color Transform;

            /// <summary>
            /// The enum boxes color.
            /// </summary>
            public Color Enum;

            /// <summary>
            /// The bounding box color.
            /// </summary>
            public Color Bounds;

            /// <summary>
            /// The structures color.
            /// </summary>
            public Color Structures;

            /// <summary>
            /// The default boxes color.
            /// </summary>
            public Color Default;
        }

        /// <summary>
        /// Descriptions for icons used by the surface.
        /// </summary>
        public struct IconsData
        {
            /// <summary>
            /// Icon for boxes without connections.
            /// </summary>
            public SpriteHandle BoxOpen;

            /// <summary>
            /// Icon for boxes with connections.
            /// </summary>
            public SpriteHandle BoxClose;

            /// <summary>
            /// Icon for impulse boxes without connections.
            /// </summary>
            public SpriteHandle ArrowOpen;

            /// <summary>
            /// Icon for impulse boxes with connections.
            /// </summary>
            public SpriteHandle ArrowClose;
        }

        /// <summary>
        /// The colors.
        /// </summary>
        public ColorsData Colors;

        /// <summary>
        /// The icons.
        /// </summary>
        public IconsData Icons;

        /// <summary>
        /// The background image (tiled).
        /// </summary>
        public Texture Background;

        /// <summary>
        /// Gets the color for the connection.
        /// </summary>
        /// <param name="type">The connection type.</param>
        /// <param name="hint">The connection hint.</param>
        /// <param name="color">The color.</param>
        public void GetConnectionColor(ScriptType type, ConnectionsHint hint, out Color color)
        {
            if (type == ScriptType.Null)
                color = Colors.Default;
            else if (type.IsPointer || type.IsReference)
            {
                // Find underlying type without `*` or `&`
                var typeName = type.TypeName;
                type = TypeUtils.GetType(typeName.Substring(0, typeName.Length - 1));
                GetConnectionColor(type, hint, out color);
            }
            else if (type.Type == typeof(void))
                color = Colors.Impulse;
            else if (type.Type == typeof(bool))
                color = Colors.Bool;
            else if (type.Type == typeof(byte) || type.Type == typeof(sbyte) || type.Type == typeof(char) || type.Type == typeof(short) || type.Type == typeof(ushort) || type.Type == typeof(int) || type.Type == typeof(uint) || type.Type == typeof(long) || type.Type == typeof(ulong))
                color = Colors.Integer;
            else if (type.Type == typeof(float) || type.Type == typeof(double) || hint == ConnectionsHint.Scalar)
                color = Colors.Float;
            else if (type.Type == typeof(Vector2) || type.Type == typeof(Vector3) || type.Type == typeof(Vector4) || type.Type == typeof(Color))
                color = Colors.Vector;
            else if (type.Type == typeof(string))
                color = Colors.String;
            else if (type.Type == typeof(Quaternion))
                color = Colors.Rotation;
            else if (type.Type == typeof(Transform))
                color = Colors.Transform;
            else if (type.Type == typeof(BoundingBox) || type.Type == typeof(BoundingSphere) || type.Type == typeof(BoundingFrustum))
                color = Colors.Bounds;
            else if (type.IsEnum || hint == ConnectionsHint.Enum)
                color = Colors.Enum;
            else if (type.IsValueType)
                color = Colors.Structures;
            else if (new ScriptType(typeof(FlaxEngine.Object)).IsAssignableFrom(type))
                color = Colors.Object;
            else if (hint == ConnectionsHint.Vector)
                color = Colors.Vector;
            else
                color = Colors.Default;
        }

        /// <summary>
        ///  Function used to create style for the given surface type. Can be overriden to provide some customization via user plugin.
        /// </summary>
        public static Func<Editor, SurfaceStyle> CreateStyleHandler = CreateDefault;

        /// <summary>
        /// Creates the default style.
        /// </summary>
        /// <param name="editor">The editor.</param>
        /// <returns>Created style.</returns>
        public static SurfaceStyle CreateDefault(Editor editor)
        {
            return new SurfaceStyle
            {
                Colors =
                {
                    // Connecting nodes
                    Connecting = Color.White,
                    ConnectingValid = new Color(11, 252, 11),
                    ConnectingInvalid = new Color(252, 12, 11),

                    // Boxes
                    Impulse = new Color(252, 255, 255),
                    Bool = new Color(237, 28, 36),
                    Integer = new Color(181, 230, 29),
                    Float = new Color(146, 208, 80),
                    Vector = new Color(255, 251, 1),
                    String = new Color(163, 73, 164),
                    Object = new Color(0, 162, 232),
                    Rotation = new Color(153, 217, 234),
                    Transform = new Color(255, 127, 39),
                    Enum = new Color(20, 151, 20),
                    Bounds = new Color(34, 117, 76),
                    Structures = new Color(228, 138, 185),
                    Default = new Color(200, 200, 200),
                },
                Icons =
                {
                    BoxOpen = editor.Icons.VisjectBoxOpen32,
                    BoxClose = editor.Icons.VisjectBoxClosed32,
                    ArrowOpen = editor.Icons.VisjectArrowOpen32,
                    ArrowClose = editor.Icons.VisjectArrowClosed32,
                },
                Background = editor.UI.VisjectSurfaceBackground,
            };
        }
    }
}
