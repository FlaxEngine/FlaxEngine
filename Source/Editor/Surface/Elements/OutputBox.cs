// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;
using FlaxEngine.GUI;
using System;
using FlaxEditor.Options;

namespace FlaxEditor.Surface.Elements
{
    /// <summary>
    /// Visject Surface output box element.
    /// </summary>
    /// <seealso cref="FlaxEditor.Surface.Elements.Box" />
    [HideInEditor]
    public class OutputBox : Box
    {
        /// <summary>
        /// The default thickness of the connection line
        /// </summary>
        public const float DefaultConnectionThickness = 1.5f;

        /// <summary>
        /// The thickness of a highlighted connection line
        /// </summary>
        public const float ConnectingConnectionThickness = 2.5f;

        /// <summary>
        /// The thickness of the selected connection line
        /// </summary>
        public const float SelectedConnectionThickness = 3f;

        /// <summary>
        /// The offset between the connection line and the box
        /// </summary>
        public const float DefaultConnectionOffset = 24f;

        /// <summary>
        /// Distance for the mouse to be considered above the connection
        /// </summary>
        public float MouseOverConnectionDistance => 100f / Surface.ViewScale;

        /// <inheritdoc />
        public OutputBox(SurfaceNode parentNode, NodeElementArchetype archetype)
        : base(parentNode, archetype, archetype.Position + new Float2(parentNode.Archetype.Size.X, 0))
        {
        }

        /// <summary>
        /// Draws the connection between two boxes.
        /// </summary>
        /// <param name="style">The Visject surface style.</param>
        /// <param name="start">The start location.</param>
        /// <param name="end">The end location.</param>
        /// <param name="color">The connection color.</param>
        /// <param name="thickness">The connection thickness.</param>
        public static void DrawConnection(SurfaceStyle style, ref Float2 start, ref Float2 end, ref Color color, float thickness = DefaultConnectionThickness)
        {
            if (style.DrawConnection != null)
            {
                style.DrawConnection(start, end, color, thickness);
                return;
            }

            float connectionOffset = Mathf.Max(0f, DefaultConnectionOffset * (1 - Editor.Instance.Options.Options.Interface.ConnectionCurvature));
            Float2 offsetStart = new Float2(start.X + connectionOffset, start.Y);
            Float2 offsetEnd = new Float2(end.X - connectionOffset, end.Y);

            // Calculate control points
            CalculateBezierControlPoints(offsetStart, offsetEnd, out var control1, out var control2);

            // Draw offset lines only if necessary
            if (connectionOffset >= float.Epsilon)
            {
                Render2D.DrawLine(start, offsetStart, color, thickness);
                Render2D.DrawLine(offsetEnd, end, color, thickness);
            }

            // Draw line
            Render2D.DrawBezier(offsetStart, control1, control2, offsetEnd, color, thickness);

            /*
            // Debug drawing control points
            var bSize = new Float2(4, 4);
            Render2D.FillRectangle(new Rectangle(control1 - bSize * 0.5f, bSize), Color.Blue);
            Render2D.FillRectangle(new Rectangle(control2 - bSize * 0.5f, bSize), Color.Gold);
            */
        }

        private static void CalculateBezierControlPoints(Float2 start, Float2 end, out Float2 control1, out Float2 control2)
        {
            // Control points parameters
            const float minControlLength = 50f;
            const float maxControlLength = 120f;
            var dst = (end - start).Length;
            var yDst = Mathf.Abs(start.Y - end.Y);

            // Calculate control points
            var minControlDst = dst * 0.5f;
            var maxControlDst = Mathf.Max(Mathf.Min(maxControlLength, dst), minControlLength);
            var controlDst = Mathf.Lerp(minControlDst, maxControlDst, Mathf.Clamp(yDst / minControlLength, 0f, 1f));
            controlDst *= Editor.Instance.Options.Options.Interface.ConnectionCurvature;

            control1 = new Float2(start.X + controlDst, start.Y);
            control2 = new Float2(end.X - controlDst, end.Y);
        }

        /// <summary>
        /// Checks if a point intersects a connection
        /// </summary>
        /// <param name="targetBox">The other box.</param>
        /// <param name="mousePosition">The mouse position</param>
        public bool IntersectsConnection(Box targetBox, ref Float2 mousePosition)
        {
            float connectionOffset = Mathf.Max(0f, DefaultConnectionOffset * (1 - Editor.Instance.Options.Options.Interface.ConnectionCurvature));
            Float2 start = new Float2(ConnectionOrigin.X + connectionOffset, ConnectionOrigin.Y);
            Float2 end = new Float2(targetBox.ConnectionOrigin.X - connectionOffset, targetBox.ConnectionOrigin.Y);
            return IntersectsConnection(ref start, ref end, ref mousePosition, MouseOverConnectionDistance);
        }

        /// <summary>
        /// Checks if a point intersects a bezier curve
        /// </summary>
        /// <param name="start">The start location.</param>
        /// <param name="end">The end location.</param>
        /// <param name="point">The point</param>
        /// <param name="distance">Distance at which its an intersection</param>
        public static bool IntersectsConnection(ref Float2 start, ref Float2 end, ref Float2 point, float distance)
        {
            // Pretty much a point in rectangle check
            if ((point.X - start.X) * (end.X - point.X) < 0)
                return false;

            float offset = Mathf.Sign(end.Y - start.Y) * distance;
            if ((point.Y - (start.Y - offset)) * ((end.Y + offset) - point.Y) < 0)
                return false;

            float connectionOffset = Mathf.Max(0f, DefaultConnectionOffset * (1 - Editor.Instance.Options.Options.Interface.ConnectionCurvature));
            Float2 offsetStart = new Float2(start.X + connectionOffset, start.Y);
            Float2 offsetEnd = new Float2(end.X - connectionOffset, end.Y);

            float squaredDistance = distance;
            CalculateBezierControlPoints(offsetStart, offsetEnd, out var control1, out var control2);

            var d1 = control1 - offsetStart;
            var d2 = control2 - control1;
            var d3 = offsetEnd - control2;
            float len = d1.Length + d2.Length + d3.Length;
            int segmentCount = Math.Min(Math.Max(Mathf.CeilToInt(len * 0.05f), 1), 100);
            float segmentCountInv = 1.0f / segmentCount;

            Bezier(ref offsetStart, ref control1, ref control2, ref offsetEnd, 0, out var p);
            for (int i = 1; i <= segmentCount; i++)
            {
                var oldp = p;
                float t = i * segmentCountInv;
                Bezier(ref offsetStart, ref control1, ref control2, ref offsetEnd, t, out p);

                // Maybe it would be reasonable to return the point?
                CollisionsHelper.ClosestPointPointLine(ref point, ref oldp, ref p, out var result);
                if (Float2.DistanceSquared(point, result) <= squaredDistance)
                {
                    return true;
                }
            }
            return false;
        }

        private static void Bezier(ref Float2 p0, ref Float2 p1, ref Float2 p2, ref Float2 p3, float alpha, out Float2 result)
        {
            Float2.Lerp(ref p0, ref p1, alpha, out var p01);
            Float2.Lerp(ref p1, ref p2, alpha, out var p12);
            Float2.Lerp(ref p2, ref p3, alpha, out var p23);
            Float2.Lerp(ref p01, ref p12, alpha, out var p012);
            Float2.Lerp(ref p12, ref p23, alpha, out var p123);
            Float2.Lerp(ref p012, ref p123, alpha, out result);
        }

        /// <summary>
        /// Draw all connections coming from this box.
        /// </summary>
        public void DrawConnections(ref Float2 mousePosition)
        {
            // Draw all the connections
            var style = Surface.Style;
            var mouseOverDistance = MouseOverConnectionDistance;
            var startPos = ConnectionOrigin;
            var startHighlight = ConnectionsHighlightIntensity;
            for (int i = 0; i < Connections.Count; i++)
            {
                Box targetBox = Connections[i];
                var endPos = targetBox.ConnectionOrigin;
                var highlight = DefaultConnectionThickness + Mathf.Max(startHighlight, targetBox.ConnectionsHighlightIntensity);
                var alpha = targetBox.Enabled && targetBox.IsActive ? 1.0f : 0.6f;

                // We have to calculate an offset here to preserve the original color for when the default connection thickness is larger than 1
                var highlightOffset = (highlight - (DefaultConnectionThickness - 1));
                var color = _currentTypeColor * highlightOffset * alpha;

                // TODO: Figure out how to only draw the topmost connection
                if (IntersectsConnection(ref startPos, ref endPos, ref mousePosition, mouseOverDistance))
                    highlight += DefaultConnectionThickness * 0.5f;

                // Increase thickness on impulse/ execution lines
                if (targetBox.CurrentType.IsVoid)
                    highlight *= 2;
                
                DrawConnection(style, ref startPos, ref endPos, ref color, highlight);
            }
        }

        /// <summary>
        /// Draw a selected connections coming from this box.
        /// </summary>
        public void DrawSelectedConnection(Box targetBox)
        {
            // Draw all the connections
            var startPos = ConnectionOrigin;
            var endPos = targetBox.ConnectionOrigin;
            var alpha = targetBox.Enabled && targetBox.IsActive ? 1.0f : 0.6f;
            var color = _currentTypeColor * alpha;
            DrawConnection(Surface.Style, ref startPos, ref endPos, ref color, SelectedConnectionThickness);
        }

        /// <inheritdoc />
        public override bool IsOutput => true;

        /// <inheritdoc />
        public override void Draw()
        {
            base.Draw();

            // Box
            Surface.Style.DrawBox(this);

            // Draw text
            var style = Style.Current;
            var rect = new Rectangle(-100, 0, 100 - 2, Height);
            Render2D.DrawText(style.FontSmall, Text, rect, Enabled ? style.Foreground : style.ForegroundDisabled, TextAlignment.Far, TextAlignment.Center);
        }
    }
}
