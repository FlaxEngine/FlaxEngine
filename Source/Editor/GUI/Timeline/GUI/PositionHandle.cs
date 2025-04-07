// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Globalization;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI.Timeline.GUI
{
    /// <summary>
    /// The timeline current position tracking control.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.ContainerControl" />
    class PositionHandle : ContainerControl
    {
        private Timeline _timeline;

        /// <summary>
        /// Initializes a new instance of the <see cref="PositionHandle"/> class.
        /// </summary>
        /// <param name="timeline">The timeline.</param>
        public PositionHandle(Timeline timeline)
        {
            _timeline = timeline;
        }

        /// <inheritdoc />
        public override void Draw()
        {
            var style = Style.Current;
            var icon = Editor.Instance.Icons.VisjectArrowClosed32;
            var timeAxisOverlap = Timeline.HeaderTopAreaHeight * 0.5f;
            var timeAxisHeaderOffset = -_timeline.MediaBackground.ViewOffset.Y - timeAxisOverlap;

            // Time label
            string labelText;
            switch (_timeline.TimeShowMode)
            {
            case Timeline.TimeShowModes.Frames:
                labelText = _timeline.CurrentFrame.ToString("###0", CultureInfo.InvariantCulture);
                break;
            case Timeline.TimeShowModes.Seconds:
                labelText = _timeline.CurrentTime.ToString("###0.##'s'", CultureInfo.InvariantCulture);
                break;
            case Timeline.TimeShowModes.Time:
                labelText = TimeSpan.FromSeconds(_timeline.CurrentTime).ToString("g");
                break;
            default: throw new ArgumentOutOfRangeException();
            }
            var color = (_timeline.IsMovingPositionHandle ? style.SelectionBorder : style.Foreground).AlphaMultiplied(0.6f);
            Matrix3x3.RotationZ(Mathf.PiOverTwo, out var m1);
            var m2 = Matrix3x3.Translation2D(0, timeAxisHeaderOffset);
            Matrix3x3.Multiply(ref m1, ref m2, out var m3);
            Render2D.PushTransform(ref m3);
            // TODO: Convert to its own sprite or 9 slice
            Render2D.DrawSprite(icon, new Rectangle(new Float2(10, -icon.Size.X * 0.5f - 1), Size + new Float2(0, 1)), color);
            Render2D.FillRectangle(new Rectangle(new Float2(-6, -icon.Size.Y * 0.5f + 7), new Float2(timeAxisOverlap, 5)), color);
            Render2D.PopTransform();
            var textMatrix = Matrix3x3.Translation2D(12, timeAxisHeaderOffset);
            Render2D.PushTransform(ref textMatrix);
            Render2D.DrawText(style.FontSmall, labelText, style.Foreground, new Float2(2, -6));
            Render2D.PopTransform();

            color = _timeline.IsMovingPositionHandle ? style.SelectionBorder : style.Foreground.RGBMultiplied(0.8f);
            Render2D.FillRectangle(new Rectangle(Width * 0.5f, Height + timeAxisHeaderOffset, 1, _timeline.MediaPanel.Height - timeAxisHeaderOffset - timeAxisOverlap), color);

            base.Draw();
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            _timeline = null;

            base.OnDestroy();
        }
    }
}
