// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine.GUI
{
    /// <summary>
    /// Button with an icon.
    /// </summary>
    public class IconButton : Button
    {
        /// <inheritdoc />
        public override void ClearState()
        {
            base.ClearState();
            
            if (_isPressed)
                OnPressEnd();
        }

        /// <inheritdoc />
        public override void DrawSelf()
        {
            // Cache data
            Rectangle clientRect = new Rectangle(Float2.Zero, Size);
            bool enabled = EnabledInHierarchy;
            Color backgroundColor = BackgroundColor;
            Color borderColor = BorderColor;
            Color textColor = TextColor;
            if (!enabled)
            {
                backgroundColor *= 0.5f;
                borderColor *= 0.5f;
                textColor *= 0.6f;
            }
            else if (_isPressed)
            {
                backgroundColor = BackgroundColorSelected;
                borderColor = BorderColorSelected;
            }
            else if (IsMouseOver || IsNavFocused)
            {
                backgroundColor = BackgroundColorHighlighted;
                borderColor = BorderColorHighlighted;
            }

            // Draw background
            if (BackgroundBrush != null)
                BackgroundBrush.Draw(clientRect, backgroundColor);
            else
                Render2D.FillRectangle(clientRect, backgroundColor);
            Render2D.DrawRectangle(clientRect, borderColor);

            // Draw text
            Render2D.DrawText(_font?.GetFont(), TextMaterial, _text, clientRect, textColor, TextAlignment.Center, TextAlignment.Center);
        }

        /// <inheritdoc />
        public override void OnMouseEnter(Float2 location)
        {
            base.OnMouseEnter(location);

            HoverBegin?.Invoke();
        }

        /// <inheritdoc />
        public override void OnMouseLeave()
        {
            if (_isPressed)
            {
                OnPressEnd();
            }

            HoverEnd?.Invoke();

            base.OnMouseLeave();
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (base.OnMouseDown(location, button))
                return true;

            if (button == MouseButton.Left && !_isPressed)
            {
                OnPressBegin();
                return true;
            }
            return false;
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (base.OnMouseUp(location, button))
                return true;

            if (button == MouseButton.Left && _isPressed)
            {
                OnPressEnd();
                OnClick();
                return true;
            }
            return false;
        }

        /// <inheritdoc />
        public override bool OnTouchDown(Float2 location, int pointerId)
        {
            if (base.OnTouchDown(location, pointerId))
                return true;

            if (!_isPressed)
            {
                OnPressBegin();
                return true;
            }
            return false;
        }

        /// <inheritdoc />
        public override bool OnTouchUp(Float2 location, int pointerId)
        {
            if (base.OnTouchUp(location, pointerId))
                return true;

            if (_isPressed)
            {
                OnPressEnd();
                OnClick();
                return true;
            }
            return false;
        }

        /// <inheritdoc />
        public override void OnTouchLeave()
        {
            if (_isPressed)
            {
                OnPressEnd();
            }

            base.OnTouchLeave();
        }

        /// <inheritdoc />
        public override void OnLostFocus()
        {
            if (_isPressed)
            {
                OnPressEnd();
            }

            base.OnLostFocus();
        }

        /// <inheritdoc />
        public override void OnSubmit()
        {
            OnClick();

            base.OnSubmit();
        }
    }
}
