// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

namespace FlaxEngine
{
    partial struct CreateWindowSettings
    {
        /// <summary>
        /// Gets the default settings.
        /// </summary>
        public static CreateWindowSettings Default => new CreateWindowSettings
        {
            Position = new Vector2(100, 100),
            Size = new Vector2(640, 480),
            MinimumSize = Vector2.One,
            MaximumSize = new Vector2(4100, 4100),
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
        };
    }
}
