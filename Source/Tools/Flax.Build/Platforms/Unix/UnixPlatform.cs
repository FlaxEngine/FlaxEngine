// Copyright (c) Wojciech Figat. All rights reserved.

using System.Diagnostics;
using Flax.Build.Projects;

namespace Flax.Build.Platforms
{
    /// <summary>
    /// The build platform for all Unix-like systems.
    /// </summary>
    /// <seealso cref="Platform" />
    public abstract class UnixPlatform : Platform
    {
        /// <inheritdoc />
        public override string ExecutableFileExtension => string.Empty;

        /// <inheritdoc />
        public override string SharedLibraryFileExtension => ".so";

        /// <inheritdoc />
        public override string StaticLibraryFileExtension => ".a";

        /// <inheritdoc />
        public override string ProgramDatabaseFileExtension => string.Empty;

        /// <inheritdoc />
        public override string SharedLibraryFilePrefix => "lib";

        /// <inheritdoc />
        public override string StaticLibraryFilePrefix => "lib";

        /// <inheritdoc />
        public override ProjectFormat DefaultProjectFormat => ProjectFormat.VisualStudioCode;

        /// <summary>
        /// Initializes a new instance of the <see cref="Flax.Build.Platforms.UnixPlatform"/> class.
        /// </summary>
        protected UnixPlatform()
        {
        }

        /// <summary>
        /// Uses which command to find the given file.
        /// </summary>
        /// <param name="name">The name of the file to find.</param>
        /// <returns>The full path or null if not found anything valid.</returns>
        public static string Which(string name)
        {
            if (string.IsNullOrEmpty(name))
                return null;

            Process proc = new Process
            {
                StartInfo =
                {
                    FileName = "/bin/sh",
                    ArgumentList = { "-c", $"which {name}" },
                    UseShellExecute = false,
                    CreateNoWindow = true,
                    RedirectStandardOutput = true,
                    RedirectStandardError = true
                }
            };

            proc.Start();
            proc.WaitForExit();

            string path = proc.StandardOutput.ReadLine();
            Log.Verbose(string.Format("which {0} exit code: {1}, result: {2}", name, proc.ExitCode, path));

            if (proc.ExitCode == 0)
            {
                string err = proc.StandardError.ReadToEnd();
                if (!string.IsNullOrEmpty(err))
                    Log.Verbose(string.Format("which stderr: {0}", err));
                return path;
            }

            return null;
        }
    }
}
