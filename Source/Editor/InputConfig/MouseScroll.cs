#pragma warning disable 1591
using System;

namespace FlaxEditor.InputConfig
{
    public enum MouseScroll
    {
        None = 0,
        ScrollUp = 1,
        ScrollDown = -1
    }

    public static class MouseScrollHelper
    {
        public static MouseScroll GetScrollDirection(float delta)
        {
            if (delta > 0)
                return MouseScroll.ScrollUp;
            if (delta < 0)
                return MouseScroll.ScrollDown;
            return MouseScroll.None;
        }
    }

    public static class MouseScrollString
    {
        public const string None = "None";
        public const string Up = "ScrollUp";
        public const string Down = "ScrollDown";
    }
}
