// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using FlaxEngine;
using FlaxEngine.GUI;

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

        /// <summary>
        /// Draw all connections coming from this box.
        /// </summary>
        public void DrawConnections()
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
                DrawConnection(ref startPos, ref endPos, ref color, highlight);
            }
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
