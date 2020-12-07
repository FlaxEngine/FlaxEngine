// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

namespace FlaxEditor.Content
{
    /// <summary>
    /// File entry action (import or create).
    /// </summary>
    public interface IFileEntryAction
    {
        /// <summary>
        /// The source file path (may be empty or null).
        /// </summary>
        string SourceUrl { get; }

        /// <summary>
        /// The result file path.
        /// </summary>
        string ResultUrl { get; }

        /// <summary>
        /// Executes this action.
        /// </summary>
        /// <returns>True if, failed, otherwise false.</returns>
        bool Execute();
    }
}
