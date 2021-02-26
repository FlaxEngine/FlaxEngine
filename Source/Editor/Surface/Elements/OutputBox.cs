// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using FlaxEngine;
using FlaxEngine.GUI;
using System;

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
        /// Distance for the mouse to be considered above the connection
        /// </summary>
        public float MouseOverConnectionDistance => 100f / Surface.ViewScale;

        /// <inheritdoc />
        public OutputBox(SurfaceNode parentNode, NodeElementArchetype archetype)
        : base(parentNode, archetype, archetype.Position + new Vector2(parentNode.Archetype.Size.X, 0))
        {
        }

        /// <summary>
        /// Draws the connection between two boxes.
        /// </summary>
        /// <param name="start">The start location.</param>
        /// <param name="end">The end location.</param>
        /// <param name="color">The connection color.</param>
        /// <param name="thickness">The connection thickness.</param>
        public static void DrawConnection(ref Vector2 start, ref Vector2 end, ref Color color, float thickness = 1)
        {
            // Calculate control points
            var dst = (end - start) * new Vector2(0.5f, 0.05f);
            Vector2 control1 = new Vector2(start.X + dst.X, start.Y + dst.Y);
            Vector2 control2 = new Vector2(end.X - dst.X, end.Y + dst.Y);

            // Draw line
            Render2D.DrawBezier(start, control1, control2, end, color, thickness);

            /*
            // Debug drawing control points
            Vector2 bSize = new Vector2(4, 4);
            Render2D.FillRectangle(new Rectangle(control1 - bSize * 0.5f, bSize), Color.Blue);
            Render2D.FillRectangle(new Rectangle(control2 - bSize * 0.5f, bSize), Color.Gold);
            */
        }

        /// <summary>
        /// Checks if a point intersects a connection
        /// </summary>
        /// <param name="targetBox">The other box.</param>
        /// <param name="mousePosition">The mouse position</param>
        public bool IntersectsConnection(Box targetBox, ref Vector2 mousePosition)
        {
            var startPos = Parent.PointToParent(Center);
            Vector2 endPos = targetBox.Parent.PointToParent(targetBox.Center);
            return IntersectsConnection(ref startPos, ref endPos, ref mousePosition, MouseOverConnectionDistance);
        }

        /// <summary>
        /// Checks if a point intersects a bezier curve
        /// </summary>
        /// <param name="start">The start location.</param>
        /// <param name="end">The end location.</param>
        /// <param name="point">The point</param>
        /// <param name="distance">Distance at which its an intersection</param>
        public static bool IntersectsConnection(ref Vector2 start, ref Vector2 end, ref Vector2 point, float distance)
        {
            // Pretty much a point in rectangle check
            if ((point.X - start.X) * (end.X - point.X) < 0) return false;

            float offset = Mathf.Sign(end.Y - start.Y) * distance;
            if ((point.Y - (start.Y - offset)) * ((end.Y + offset) - point.Y) < 0) return false;

            // Taken from the Render2D.DrawBezier code
            float squaredDistance = distance;

            var dst = (end - start) * new Vector2(0.5f, 0.05f);
            Vector2 control1 = new Vector2(start.X + dst.X, start.Y + dst.Y);
            Vector2 control2 = new Vector2(end.X - dst.X, end.Y + dst.Y);

            Vector2 d1 = control1 - start;
            Vector2 d2 = control2 - control1;
            Vector2 d3 = end - control2;
            float len = d1.Length + d2.Length + d3.Length;
            int segmentCount = Math.Min(Math.Max(Mathf.CeilToInt(len * 0.05f), 1), 100);
            float segmentCountInv = 1.0f / segmentCount;

            Bezier(ref start, ref control1, ref control2, ref end, 0, out Vector2 p);
            for (int i = 1; i <= segmentCount; i++)
            {
                Vector2 oldp = p;
                float t = i * segmentCountInv;
                Bezier(ref start, ref control1, ref control2, ref end, t, out p);

                // Maybe it would be reasonable to return the point?
                CollisionsHelper.ClosestPointPointLine(ref point, ref oldp, ref p, out Vector2 result);
                if (Vector2.DistanceSquared(point, result) <= squaredDistance)
                {
                    return true;
                }
            }
            return false;
        }

        private static void Bezier(ref Vector2 p0, ref Vector2 p1, ref Vector2 p2, ref Vector2 p3, float alpha, out Vector2 result)
        {
            Vector2.Lerp(ref p0, ref p1, alpha, out var p01);
            Vector2.Lerp(ref p1, ref p2, alpha, out var p12);
            Vector2.Lerp(ref p2, ref p3, alpha, out var p23);
            Vector2.Lerp(ref p01, ref p12, alpha, out var p012);
            Vector2.Lerp(ref p12, ref p23, alpha, out var p123);
            Vector2.Lerp(ref p012, ref p123, alpha, out result);
        }

        /// <summary>
        /// Draw all connections coming from this box.
        /// </summary>
        public void DrawConnections(ref Vector2 mousePosition)
        {
            float mouseOverDistance = MouseOverConnectionDistance;
            // Draw all the connections
            var startPos = Parent.PointToParent(Center);
            var startHighlight = ConnectionsHighlightIntensity;
            for (int i = 0; i < Connections.Count; i++)
            {
                Box targetBox = Connections[i];
                Vector2 endPos = targetBox.Parent.PointToParent(targetBox.Center);
                var highlight = 1 + Mathf.Max(startHighlight, targetBox.ConnectionsHighlightIntensity);
                var color = _currentTypeColor * highlight;

                // TODO: Figure out how to only draw the topmost connection
                if (IntersectsConnection(ref startPos, ref endPos, ref mousePosition, mouseOverDistance))
                {
                    highlight += 0.5f;
                }

                DrawConnection(ref startPos, ref endPos, ref color, highlight);
            }
        }

        /// <summary>
        /// Draw a selected connections coming from this box.
        /// </summary>
        public void DrawSelectedConnection(Box targetBox)
        {
            // Draw all the connections
            var startPos = Parent.PointToParent(Center);
            Vector2 endPos = targetBox.Parent.PointToParent(targetBox.Center);
            DrawConnection(ref startPos, ref endPos, ref _currentTypeColor, 2.5f);
        }

        /// <inheritdoc />
        public override bool IsOutput => true;

        /// <inheritdoc />
        public override void Draw()
        {
            base.Draw();

            // Box
            DrawBox();

            // Draw text
            var style = Style.Current;
            var rect = new Rectangle(-100, 0, 100 - 2, Height);
            Render2D.DrawText(style.FontSmall, Text, rect, Enabled ? style.Foreground : style.ForegroundDisabled, TextAlignment.Far, TextAlignment.Center);
        }
    }
}
