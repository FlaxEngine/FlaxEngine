// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.Progress.Handlers
{
    /// <summary>
    /// Async code editor opening progress reporting handler.
    /// </summary>
    /// <seealso cref="FlaxEditor.Progress.ProgressHandler" />
    public sealed class CodeEditorOpenProgress : ProgressHandler
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="CodeEditorOpenProgress"/> class.
        /// </summary>
        public CodeEditorOpenProgress()
        {
            // Link for events
            ScriptsBuilder.CodeEditorAsyncOpenBegin += OnStart;
            ScriptsBuilder.CodeEditorAsyncOpenEnd += OnEnd;
        }

        /// <inheritdoc />
        protected override void OnStart()
        {
            base.OnStart();

            OnUpdate(0.1f, "Starting code editor...");
        }
    }
}
