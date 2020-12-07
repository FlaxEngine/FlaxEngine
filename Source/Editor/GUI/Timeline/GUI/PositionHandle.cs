// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI.Timeline.GUI
{
    /// <summary>
    /// The timeline current position tracking control.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.ContainerControl" />
    public class PositionHandle : ContainerControl
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
            var icon = Editor.Instance.Icons.VisjectArrowClose;
            var timeAxisHeaderOffset = -_timeline.MediaBackground.ViewOffset.Y;

            Matrix3x3.RotationZ(Mathf.PiOverTwo, out var m1);
            var m2 = Matrix3x3.Translation2D(0, timeAxisHeaderOffset);
            Matrix3x3.Multiply(ref m1, ref m2, out var m3);
            Render2D.PushTransform(ref m3);
            Render2D.DrawSprite(icon, new Rectangle(new Vector2(4, -Width), Size), _timeline.IsMovingPositionHandle ? style.ProgressNormal : style.Foreground);
            Render2D.PopTransform();

            Render2D.FillRectangle(new Rectangle(Width * 0.5f, Height + timeAxisHeaderOffset, 1, _timeline.MediaPanel.Height - timeAxisHeaderOffset), _timeline.IsMovingPositionHandle ? style.ProgressNormal : style.Foreground.RGBMultiplied(0.8f));

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
