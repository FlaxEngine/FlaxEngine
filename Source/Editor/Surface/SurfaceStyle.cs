// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.Scripting;
using FlaxEngine;
using FlaxEngine.Utilities;

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
        /// Boxes drawing callback.
        /// </summary>
        public Action<Elements.Box> DrawBox = DefaultDrawBox;

        /// <summary>
        /// Custom box connection drawing callback (null by default).
        /// </summary>
        public Action<Float2, Float2, Color, float> DrawConnection = null;

        /// <summary>
        /// Gets the color for the connection.
        /// </summary>
        /// <param name="type">The connection type.</param>
        /// <param name="hint">The connection hint.</param>
        /// <param name="color">The color.</param>
        public void GetConnectionColor(ScriptType type, ConnectionsHint hint, out Color color)
        {
            if (type == ScriptType.Null)
            {
                if (hint == ConnectionsHint.Vector)
                    color = Colors.Vector;
                else if (hint == ConnectionsHint.Scalar)
                    color = Colors.Float;
                else if (hint == ConnectionsHint.Enum)
                    color = Colors.Enum;
                else
                    color = Colors.Default;
            }
            else if (type.IsPointer || type.IsReference)
            {
                // Find underlying type without `*` or `&`
                var typeName = type.TypeName;
                type = TypeUtils.GetType(typeName.Substring(0, typeName.Length - 1));
                GetConnectionColor(type, hint, out color);
            }
            else if (type.IsArray)
                GetConnectionColor(type.GetElementType(), hint, out color);
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
            else if (type.Type == typeof(Float2) || type.Type == typeof(Float3) || type.Type == typeof(Float4))
                color = Colors.Vector;
            else if (type.Type == typeof(Double2) || type.Type == typeof(Double3) || type.Type == typeof(Double4))
                color = Colors.Vector;
            else if (type.Type == typeof(Int2) || type.Type == typeof(Int3) || type.Type == typeof(Int4))
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
            else if (ScriptType.FlaxObject.IsAssignableFrom(type) || type.IsInterface)
                color = Colors.Object;
            else if (hint == ConnectionsHint.Vector)
                color = Colors.Vector;
            else
                color = Colors.Default;
        }

        private static void DefaultDrawBox(Elements.Box box)
        {
            var rect = new Rectangle(Float2.Zero, box.Size);

            // Size culling
            const float minBoxSize = 5.0f;
            if (rect.Size.LengthSquared < minBoxSize * minBoxSize)
                return;

            // Debugging boxes size
            //Render2D.DrawRectangle(rect, Color.Orange); return;

            // Draw icon
            bool hasConnections = box.HasAnyConnection;
            float alpha = box.Enabled && box.IsActive ? 1.0f : 0.6f;
            Color color = box.CurrentTypeColor * alpha;
            var style = box.Surface.Style;
            SpriteHandle icon;
            if (box.CurrentType.Type == typeof(void))
                icon = hasConnections ? style.Icons.ArrowClose : style.Icons.ArrowOpen;
            else
                icon = hasConnections ? style.Icons.BoxClose : style.Icons.BoxOpen;
            color *= box.ConnectionsHighlightIntensity + 1;
            Render2D.DrawSprite(icon, rect, color);

            // Draw selection hint
            if (box.IsSelected)
            {
                float outlineAlpha = Mathf.Sin(Time.TimeSinceStartup * 4.0f) * 0.5f + 0.5f;
                float outlineWidth = Mathf.Lerp(1.5f, 4.0f, outlineAlpha);
                var outlineRect = new Rectangle(rect.X - outlineWidth, rect.Y - outlineWidth, rect.Width + outlineWidth * 2, rect.Height + outlineWidth * 2);
                Render2D.DrawSprite(icon, outlineRect, FlaxEngine.GUI.Style.Current.BorderSelected.RGBMultiplied(1.0f + outlineAlpha * 0.4f));
            }
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

        /// <summary>
        /// Draws a simple straight connection between two locations.
        /// </summary>
        /// <param name="startPos">The start position.</param>
        /// <param name="endPos">The end position.</param>
        /// <param name="color">The line color.</param>
        /// <param name="thickness">The line thickness.</param>
        public static void DrawStraightConnection(Float2 startPos, Float2 endPos, Color color, float thickness = 1.0f)
        {
            var sub = endPos - startPos;
            var length = sub.Length;
            if (length > Mathf.Epsilon)
            {
                var dir = sub / length;
                var arrowRect = new Rectangle(0, 0, 16.0f, 16.0f);
                float rotation = Float2.Dot(dir, Float2.UnitY);
                if (endPos.X < startPos.X)
                    rotation = 2 - rotation;
                var sprite = Editor.Instance.Icons.VisjectArrowClosed32;
                var arrowTransform =
                    Matrix3x3.Translation2D(-6.5f, -8) *
                    Matrix3x3.RotationZ(rotation * Mathf.PiOverTwo) * 
                    Matrix3x3.Translation2D(endPos - dir * 8);

                Render2D.PushTransform(ref arrowTransform);
                Render2D.DrawSprite(sprite, arrowRect, color);
                Render2D.PopTransform();

                endPos -= dir * 4.0f;
            }
            Render2D.DrawLine(startPos, endPos, color);
        }
    }
}
