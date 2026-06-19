// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.Utilities
{
    /// <summary>
    /// Editor input utilities.
    /// </summary>
    public static class InputUtils
    {
        /// <summary>
        /// Checks if the key should perform deletion on the current platform.
        /// </summary>
        /// <param name="key">The keyboard key.</param>
        /// <returns>True if the key should perform deletion.</returns>
        public static bool IsKeyPerformDeletion(KeyboardKeys key)
        {
#if PLATFORM_MAC
            return key == KeyboardKeys.Backspace;
#else
            return false;
#endif
        }
    }
}
