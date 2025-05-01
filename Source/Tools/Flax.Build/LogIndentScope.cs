// Copyright (c) Wojciech Figat. All rights reserved.

using System;

namespace Flax.Build
{
    /// <summary>
    /// Applies a log indent for the lifetime of an object.
    /// </summary>
    public class LogIndentScope : IDisposable
    {
        private string _prevIndent;

        /// <summary>
        /// Initializes a new instance of the <see cref="LogIndentScope"/> class.
        /// </summary>
        /// <param name="indent">The indent to apply to the existing indentation.</param>
        public LogIndentScope(string indent = "  ")
        {
            _prevIndent = Log.Indent;
            Log.Indent += indent;
        }

        /// <summary>
        /// Restores the log indent to previous state.
        /// </summary>
        public void Dispose()
        {
            Log.Indent = _prevIndent;
            _prevIndent = null;
        }
    }
}
