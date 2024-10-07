// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

namespace FlaxEngine
{
    partial struct CreateWindowSettings
    {
        /// <summary>
        /// Gets the default settings.
        /// </summary>
        public static CreateWindowSettings Default => new CreateWindowSettings
        {
            Position = new Float2(100, 100),
            Size = new Float2(640, 480),
            MinimumSize = Float2.One,
            MaximumSize = Float2.Zero, // Unlimited size
            StartPosition = WindowStartPosition.CenterParent,
            HasBorder = true,
            ShowInTaskbar = true,
            ActivateWhenFirstShown = true,
            AllowInput = true,
            AllowMinimize = true,
            AllowMaximize = true,
            AllowDragAndDrop = true,
            Type = WindowType.Regular,
            HasSizingFrame = true,
            ShowAfterFirstPaint = true,
        };
    }
}
