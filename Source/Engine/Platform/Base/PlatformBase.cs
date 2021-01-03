// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

namespace FlaxEngine
{
    partial class Platform
    {
        /// <summary>
        /// Checks if current execution in on the main thread.
        /// </summary>
        /// <returns>True if running on the main thread, otherwise false.</returns>
        public static bool IsInMainThread => CurrentThreadID == Globals.MainThreadID;
    }
}
