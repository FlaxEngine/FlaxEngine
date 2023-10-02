// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

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
            MaximumSize = new Float2(4100, 4100),
            StartPosition = WindowStartPosition.CenterParent,
            HasBorder = true,
            ShowInTaskbar = true,
            ActivateWhenFirstShown = true,
            AllowInput = true,
            AllowMinimize = true,
            AllowMaximize = true,
            AllowDragAndDrop = true,
            IsRegularWindow = true,
            HasSizingFrame = true,
            ShowAfterFirstPaint = true,
        };
    }
}
