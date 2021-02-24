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

        private static bool IsRight(ref Vector2 a, ref Vector2 b, ref Vector2 point)
        {
            return ((b.X - a.X) * (point.Y - a.Y) - (b.Y - a.Y) * (point.X - a.X)) <= 0;
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
            if ((point.X - start.X) * (end.X - point.X) < 0) return false;

            var dst = (end - start) * new Vector2(0.25f, 0.05f); // Purposefully tweaked to 0.25
            Vector2 control1 = new Vector2(start.X + dst.X, start.Y + dst.Y);
            Vector2 control2 = new Vector2(end.X - dst.X, end.Y + dst.Y);

            // I'm ignoring the start.Y + dst.Y

            Vector2 direction = end - start;
            float offset = Mathf.Sign(direction.Y) * distance;

            Vector2 pointAbove1 = new Vector2(control1.X, start.Y - offset);
            Vector2 pointAbove2 = new Vector2(end.X, end.Y - offset);
            Vector2 pointBelow1 = new Vector2(start.X, start.Y + offset);
            Vector2 pointBelow2 = new Vector2(control2.X, end.Y + offset);

            /*  Render2D.DrawLine(pointAbove1, pointAbove2, Color.Red);
              Render2D.DrawRectangle(new Rectangle(pointAbove2 - new Vector2(5), new Vector2(10)), Color.Red);
              Render2D.DrawLine(pointBelow1, pointBelow2, Color.Green);
              Render2D.DrawRectangle(new Rectangle(pointBelow2 - new Vector2(5), new Vector2(10)), Color.Green);
            */
            // TODO: are they parallel ^?

            return IsRight(ref pointAbove1, ref pointAbove2, ref point) == IsRight(ref pointBelow2, ref pointBelow1, ref point);


            // Taken from the Render2D.DrawBezier code
            float squaredDistance = distance;

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
            // Draw all the connections
            var center = Size * 0.5f;
            var tmp = PointToParent(ref center);
            var startPos = Parent.PointToParent(ref tmp);
            var startHighlight = ConnectionsHighlightIntensity;
            for (int i = 0; i < Connections.Count; i++)
            {
                Box targetBox = Connections[i];
                tmp = targetBox.PointToParent(ref center);
                Vector2 endPos = targetBox.Parent.PointToParent(ref tmp);
                var highlight = 1 + Mathf.Max(startHighlight, targetBox.ConnectionsHighlightIntensity);
                var color = _currentTypeColor * highlight;

                // if (PointInRectangle(ref startPos, ref endPos, ref mousePosition))
                {
                    for (int ix = 0; ix < 10000; ix++)
                    {
                        OutputBox.IntersectsConnection(ref startPos, ref endPos, ref mousePosition, 30f);
                    }
                    if (OutputBox.IntersectsConnection(ref startPos, ref endPos, ref mousePosition, 30f))
                    {
                        highlight += 2;
                    }
                }


                DrawConnection(ref startPos, ref endPos, ref color, highlight);
            }
        }

        private bool PointInRectangle(ref Vector2 a, ref Vector2 b, ref Vector2 point)
        {
            // This has the convenient property that it doesn't matter if a and b are switched
            Vector2 signs = (point - a) * (b - point);
            return signs.X >= 0 && signs.Y >= 0;
        }

        /// <summary>
        /// Draw a selected connections coming from this box.
        /// </summary>
        public void DrawSelectedConnection(Box targetBox)
        {
            // Draw all the connections
            var center = Size * 0.5f;
            var tmp = PointToParent(ref center);
            var startPos = Parent.PointToParent(ref tmp);
            tmp = targetBox.PointToParent(ref center);
            Vector2 endPos = targetBox.Parent.PointToParent(ref tmp);
            DrawConnection(ref startPos, ref endPos, ref _currentTypeColor, 2);
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
