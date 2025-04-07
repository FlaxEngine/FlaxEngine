// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Windows.Profiler
{
    /// <summary>
    /// Events timeline control.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.Panel" />
    public class Timeline : Panel
    {
        /// <summary>
        /// Single timeline event control.
        /// </summary>
        /// <seealso cref="ContainerControl" />
        public class Event : ContainerControl
        {
            private static readonly Color[] Colors =
            {
                new Color(0.8f, 0.894117653f, 0.709803939f, 1f),
                new Color(0.1254902f, 0.698039234f, 0.6666667f, 1f),
                new Color(0.4831376f, 0.6211768f, 0.0219608f, 1f),
                new Color(0.3827448f, 0.2886272f, 0.5239216f, 1f),
                new Color(0.8f, 0.4423528f, 0f, 1f),
                new Color(0.4486272f, 0.4078432f, 0.050196f, 1f),
                new Color(0.4831376f, 0.6211768f, 0.0219608f, 1f),
                new Color(0.4831376f, 0.6211768f, 0.0219608f, 1f),
                new Color(0.2070592f, 0.5333336f, 0.6556864f, 1f),
                new Color(0.8f, 0.4423528f, 0f, 1f),
                new Color(0.4486272f, 0.4078432f, 0.050196f, 1f),
                new Color(0.7749016f, 0.6368624f, 0.0250984f, 1f),
                new Color(0.5333336f, 0.16f, 0.0282352f, 1f),
                new Color(0.3827448f, 0.2886272f, 0.5239216f, 1f),
                new Color(0.478431374f, 0.482352942f, 0.117647059f, 1f),
                new Color(0.9411765f, 0.5019608f, 0.5019608f, 1f),
                new Color(0.6627451f, 0.6627451f, 0.6627451f, 1f),
                new Color(0.545098066f, 0f, 0.545098066f, 1f),
            };

            private Color _color;
            private string _name;
            private float _nameLength = -1;

            /// <summary>
            /// The default height of the event.
            /// </summary>
            public const float DefaultHeight = 25.0f;

            /// <summary>
            /// Gets or sets the event name.
            /// </summary>
            public string Name
            {
                get => _name;
                set
                {
                    _name = value;
                    _nameLength = -1;
                }
            }

            /// <inheritdoc />
            protected override void OnParentChangedInternal()
            {
                base.OnParentChangedInternal();

                int key = (HasParent ? Parent.GetChildIndex(this) : 1) * (string.IsNullOrEmpty(Name) ? 1 : Name[0]);
                _color = Colors[key % Colors.Length] * 0.8f;
            }

            /// <inheritdoc />
            public override void Draw()
            {
                base.Draw();

                var style = Style.Current;
                var bounds = new Rectangle(Float2.Zero, Size);
                Color color = _color;
                if (IsMouseOver)
                    color *= 1.1f;

                Render2D.FillRectangle(bounds, color);
                Render2D.DrawRectangle(bounds, color * 0.5f);

                if (_nameLength < 0 && style.FontMedium)
                    _nameLength = style.FontMedium.MeasureText(_name).X;

                if (_nameLength < bounds.Width + 4)
                {
                    Render2D.PushClip(bounds);
                    Render2D.DrawText(style.FontMedium, _name, bounds, Style.Current.Foreground, TextAlignment.Center, TextAlignment.Center);
                    Render2D.PopClip();
                }
            }
        }

        /// <summary>
        /// Timeline track label
        /// </summary>
        /// <seealso cref="FlaxEngine.GUI.ContainerControl" />
        public class TrackLabel : ContainerControl
        {
            /// <summary>
            /// Gets or sets the name.
            /// </summary>
            public string Name { get; set; }

            /// <inheritdoc />
            public override void Draw()
            {
                base.Draw();

                var style = Style.Current;
                var rect = new Rectangle(Float2.Zero, Size);
                Render2D.PushClip(rect);
                Render2D.DrawText(style.FontMedium, Name, rect, Style.Current.Foreground, TextAlignment.Center, TextAlignment.Center, TextWrapping.WrapChars);
                Render2D.PopClip();
            }
        }

        /// <summary>
        /// Gets the events container control. Use it to remove/add events to the timeline.
        /// </summary>
        public ContainerControl EventsContainer => this;

        /// <summary>
        /// Initializes a new instance of the <see cref="Timeline"/> class.
        /// </summary>
        public Timeline()
        : base(ScrollBars.Both)
        {
        }
    }
}
