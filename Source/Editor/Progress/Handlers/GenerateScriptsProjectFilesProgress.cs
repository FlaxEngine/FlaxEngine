// Copyright (c) Wojciech Figat. All rights reserved.

using System.Threading.Tasks;

namespace FlaxEditor.Progress.Handlers
{
    /// <summary>
    /// Async scripts project files generation progress reporting handler.
    /// </summary>
    /// <seealso cref="FlaxEditor.Progress.ProgressHandler" />
    public sealed class GenerateScriptsProjectFilesProgress : ProgressHandler
    {
        /// <inheritdoc />
        protected override void OnStart()
        {
            base.OnStart();

            OnUpdate(0.1f, "Generating scripts project files...");
        }

        /// <summary>
        /// Runs the projects generation (as async task).
        /// </summary>
        public void RunAsync()
        {
            if (IsActive)
                return;

            Task.Run(Run);
        }

        /// <summary>
        /// Runs the projects generation.
        /// </summary>
        public void Run()
        {
            if (IsActive)
                return;

            try
            {
                OnStart();
                var customArgs = Editor.Instance.CodeEditing.SelectedEditor.GenerateProjectCustomArgs;
                ScriptsBuilder.GenerateProject(customArgs);
            }
            finally
            {
                OnEnd();
            }
        }
    }
}
