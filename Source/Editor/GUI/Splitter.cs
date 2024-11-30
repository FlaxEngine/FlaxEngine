// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI
{
    sealed class Splitter : Control
    {
        private bool _clicked;

        public Action<Float2> Moved;
        public const float DefaultHeight = 5.0f;

        public override void Draw()
        {
            var style = Style.Current;
            if (IsMouseOver || _clicked)
                Render2D.FillRectangle(new Rectangle(Float2.Zero, Size), _clicked ? style.BackgroundSelected : style.BackgroundHighlighted);
        }

        public override void OnEndMouseCapture()
        {
            base.OnEndMouseCapture();

            _clicked = false;
        }

        public override void Defocus()
        {
            base.Defocus();

            _clicked = false;
        }

        public override void OnMouseEnter(Float2 location)
        {
            base.OnMouseEnter(location);

            Cursor = CursorType.SizeNS;
        }

        public override void OnMouseLeave()
        {
            Cursor = CursorType.Default;

            base.OnMouseLeave();
        }

        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left)
            {
                _clicked = true;
                Focus();
                StartMouseCapture();
                return true;
            }

            return base.OnMouseDown(location, button);
        }

        public override void OnMouseMove(Float2 location)
        {
            base.OnMouseMove(location);

            if (_clicked)
            {
                Moved(location);
            }
        }

        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left && _clicked)
            {
                _clicked = false;
                EndMouseCapture();
                return true;
            }

            return base.OnMouseUp(location, button);
        }
    }
}
