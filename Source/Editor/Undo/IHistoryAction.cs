// Copyright (c) Wojciech Figat. All rights reserved.

namespace FlaxEditor
{
    /// <summary>
    /// Interface for <see cref="HistoryStack"/> actions.
    /// </summary>
    public interface IHistoryAction
    {
        /// <summary>
        /// Name or key of performed action
        /// </summary>
        string ActionString { get; }

        /// <summary>
        /// Releases unmanaged and - optionally - managed resources.
        /// </summary>
        void Dispose();
    }
}
