// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

namespace Flax.Stats
{
    /// <summary>
    /// Code statistics app task types
    /// </summary>
    internal enum TaskType
    {
        /// <summary>
        /// By default just show code statistics
        /// </summary>
        Show = 0,

        /// <summary>
        /// Peek code stats to the database
        /// </summary>
        Peek,

        /// <summary>
        /// Clear whole code statistics
        /// </summary>
        Clear,
    }
}
