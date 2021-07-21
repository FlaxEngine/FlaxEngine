// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using Flax.Build;
using Flax.Build.Platforms;

namespace Flax.Deps
{
    /// <summary>
    /// Represents a single dependency package required by the engine to be pre-build.
    /// </summary>
    abstract class Dependency
    {
        /// <summary>
        /// The options.
        /// </summary>
        public class BuildOptions
        {
            /// <summary>
            /// The intermediate folder for the deps to be build inside it.
            /// </summary>
            public string IntermediateFolder;

            /// <summary>
            /// The output folder for the platform dependencies.
            /// </summary>
            public string PlatformsFolder;

            /// <summary>
            /// The ThirdParty header files folder for the deps.
            /// </summary>
            public string ThirdPartyFolder;

            /// <summary>
            /// The target platforms to build dependency for (contains only platforms supported by the dependency itself).
            /// </summary>
            public TargetPlatform[] Platforms;
        }

        /// <summary>
        /// Gets the build platform.
        /// </summary>
        protected static TargetPlatform BuildPlatform => Platform.BuildPlatform.Target;

        /// <summary>
        /// Gets the platforms list supported by this dependency to build on the current build platform (based on <see cref="Platform.BuildPlatform"/>).
        /// </summary>
        public abstract TargetPlatform[] Platforms { get; }

        /// <summary>
        /// Builds the dependency package using the specified options.
        /// </summary>
        /// <param name="options">The options.</param>
        public abstract void Build(BuildOptions options);

        /// <summary>
        /// Gets the dependency third-party packages binaries folder.
        /// </summary>
        /// <param name="options">The options.</param>
        /// <param name="platform">The target platform.</param>
        /// <param name="architecture">The target architecture.</param>
        /// <returns>The absolute path to the deps folder for the given platform and architecture configuration.</returns>
        public static string GetThirdPartyFolder(BuildOptions options, TargetPlatform platform, TargetArchitecture architecture)
        {
            return Path.Combine(options.PlatformsFolder, platform.ToString(), "Binaries", "ThirdParty", architecture.ToString());
        }

        /// <summary>
        /// Gets the dependency packages binaries folder.
        /// </summary>
        /// <param name="options">The options.</param>
        /// <param name="platform">The target platform.</param>
        /// <returns>The absolute path to the deps folder for the given platform and architecture configuration.</returns>
        public static string GetBinariesFolder(BuildOptions options, TargetPlatform platform)
        {
            return Path.Combine(options.PlatformsFolder, platform.ToString(), "Binaries");
        }

        /// <summary>
        /// Setups the directory.
        /// </summary>
        /// <param name="path">The path.</param>
        /// <param name="forceEmpty">If set to <c>true</c> the directory will be cleared if not empty.</param>
        public static void SetupDirectory(string path, bool forceEmpty)
        {
            if (Directory.Exists(path))
            {
                if (forceEmpty)
                {
                    Utilities.DirectoryDelete(path);
                    Directory.CreateDirectory(path);
                }
            }
            else
            {
                Directory.CreateDirectory(path);
            }
        }

        /// <summary>
        /// Clones the git repository from the remote url (full repository).
        /// </summary>
        /// <param name="path">The local path for close.</param>
        /// <param name="url">The remote url.</param>
        /// <param name="commit">The commit to checkout.</param>
        /// <param name="args">The custom arguments to add to the clone command.</param>
        public static void CloneGitRepo(string path, string url, string commit = null, string args = null)
        {
            if (!Directory.Exists(Path.Combine(path, Path.GetFileNameWithoutExtension(url), ".git")))
            {
                string cmdLine = string.Format("clone \"{0}\" \"{1}\"", url, path);
                if (args != null)
                    cmdLine += " " + args;

                Utilities.Run("git", cmdLine, null, null, Utilities.RunOptions.None);
            }

            if (commit != null)
            {
                Utilities.Run("git", string.Format("reset --hard {0}", commit), null, null, Utilities.RunOptions.None);
            }
        }

        /// <summary>
        /// Clones the git repository from the remote url.
        /// </summary>
        /// <param name="path">The local path for close.</param>
        /// <param name="url">The remote url.</param>
        /// <param name="args">The custom arguments to add to the clone command.</param>
        public static void CloneGitRepoFast(string path, string url, string args = null)
        {
            if (!Directory.Exists(Path.Combine(path, Path.GetFileNameWithoutExtension(url), ".git")))
            {
                string cmdLine = string.Format("clone \"{0}\" \"{1}\" --depth 1", url, path);
                if (args != null)
                    cmdLine += " " + args;

                Utilities.Run("git", cmdLine, null, null, Utilities.RunOptions.None);
            }
        }

        /// <summary>
        /// Clones the git repository from the remote url (clones a single branch).
        /// </summary>
        /// <param name="path">The local path for close.</param>
        /// <param name="url">The remote url.</param>
        /// <param name="branch">The name of the branch to checkout.</param>
        /// <param name="commit">The commit to checkout.</param>
        /// <param name="args">The custom arguments to add to the clone command.</param>
        public static void CloneGitRepoSingleBranch(string path, string url, string branch, string commit = null, string args = null)
        {
            if (!Directory.Exists(Path.Combine(path, Path.GetFileNameWithoutExtension(url), ".git")))
            {
                string cmdLine = string.Format("clone --single-branch --branch {2} \"{0}\" \"{1}\"", url, path, branch);
                if (commit == null)
                    cmdLine += " --depth 1";
                if (args != null)
                    cmdLine += " " + args;

                Utilities.Run("git", cmdLine, null, null, Utilities.RunOptions.None);
            }

            if (commit != null)
            {
                Utilities.Run("git", string.Format("reset --hard {0}", commit), null, path, Utilities.RunOptions.None);
            }
        }

        /// <summary>
        /// Changes the current branch to the given branch in the git repository (resets all the local changes). Does not perform pull command to update branch with remote repo.
        /// </summary>
        /// <param name="path">The local path that contains git repository.</param>
        /// <param name="branch">The name of the branch to checkout.</param>
        /// <param name="commit">The commit to checkout.</param>
        /// <param name="args">The custom arguments to add to the clone command.</param>
        public static void GitCheckout(string path, string branch, string commit = null, string args = null)
        {
            string cmdLine = string.Format("checkout -B {0} origin/{0}", branch);
            if (args != null)
                cmdLine += " " + args;

            Utilities.Run("git", cmdLine, null, path, Utilities.RunOptions.None);

            if (commit != null)
            {
                Utilities.Run("git", string.Format("reset --hard {0}", commit), null, path, Utilities.RunOptions.None);
            }
        }

        /// <summary>
        /// Resets all the local changes.
        /// </summary>
        /// <param name="path">The local path that contains git repository.</param>
        public static void GitResetLocalChanges(string path)
        {
            Utilities.Run("git", "reset --hard", null, path, Utilities.RunOptions.None);
        }

        /// <summary>
        /// Runs the cmake tool.
        /// </summary>
        /// <param name="path">The path.</param>
        /// <param name="platform">The output platform.</param>
        /// <param name="architecture">The output architecture.</param>
        /// <param name="customArgs">The custom arguments for the CMake.</param>
        /// <param name="envVars">Custom environment variables to pass to the child process.</param>
        public static void RunCmake(string path, TargetPlatform platform, TargetArchitecture architecture, string customArgs = null, Dictionary<string, string> envVars = null)
        {
            string cmdLine;
            switch (platform)
            {
            case TargetPlatform.Windows:
            case TargetPlatform.XboxOne:
            case TargetPlatform.XboxScarlett:
            case TargetPlatform.UWP:
            {
                string arch;
                switch (architecture)
                {
                case TargetArchitecture.x86:
                    arch = string.Empty;
                    break;
                case TargetArchitecture.x64:
                    arch = " Win64";
                    break;
                case TargetArchitecture.ARM:
                    arch = " ARM";
                    break;
                case TargetArchitecture.ARM64:
                    arch = " ARM64";
                    break;
                default: throw new InvalidArchitectureException(architecture);
                }
                cmdLine = string.Format("CMakeLists.txt -G \"Visual Studio 14 2015{0}\"", arch);
                break;
            }
            case TargetPlatform.Linux:
            case TargetPlatform.PS4:
            {
                cmdLine = "CMakeLists.txt";
                break;
            }
            case TargetPlatform.Switch:
            {
                cmdLine = string.Format("-DCMAKE_TOOLCHAIN_FILE=\"{1}\\Source\\Platforms\\Switch\\Data\\Switch.cmake\" -G \"NMake Makefiles\" -DCMAKE_MAKE_PROGRAM=\"{0}..\\..\\VC\\bin\\nmake.exe\"", Environment.GetEnvironmentVariable("VS140COMNTOOLS"), Globals.EngineRoot);
                break;
            }
            case TargetPlatform.Android:
            {
                var ndk = AndroidNdk.Instance.RootPath;
                var abi = AndroidToolchain.GetAbiName(architecture);
                var hostName = AndroidSdk.GetHostName();
                cmdLine = string.Format("-DCMAKE_TOOLCHAIN_FILE=\"{0}/build/cmake/android.toolchain.cmake\" -DANDROID_NDK=\"{0}\" -DANDROID_STL=c++_shared -DANDROID_ABI={1} -DANDROID_PLATFORM=android-{2} -G \"MinGW Makefiles\" -DCMAKE_MAKE_PROGRAM=\"{0}/prebuilt/{3}/bin/make.exe\"", ndk, abi, Configuration.AndroidPlatformApi, hostName);
                break;
            }
            default: throw new InvalidPlatformException(platform);
            }

            if (customArgs != null)
                cmdLine += " " + customArgs;

            Utilities.Run("cmake", cmdLine, null, path, Utilities.RunOptions.None, envVars);
        }

        /// <summary>
        /// Runs the bash script via Cygwin tool (native bash on platforms other than Windows).
        /// </summary>
        /// <param name="path">The path.</param>
        /// <param name="workspace">The workspace folder.</param>
        /// <param name="envVars">Custom environment variables to pass to the child process.</param>
        public static void RunCygwin(string path, string workspace = null, Dictionary<string, string> envVars = null)
        {
            string app;
            switch (BuildPlatform)
            {
            case TargetPlatform.Windows:
            {
                var cygwinFolder = Environment.GetEnvironmentVariable("CYGWIN");
                if (string.IsNullOrEmpty(cygwinFolder) || !Directory.Exists(cygwinFolder))
                {
                    cygwinFolder = "C:\\cygwin";
                    if (!Directory.Exists(cygwinFolder))
                        throw new Exception("Missing Cygwin. Install Cygwin64 to C:\\cygwin or set CYGWIN env variable to install location folder.");
                }
                app = Path.Combine(cygwinFolder, "bin\\bash.exe");
                break;
            }
            case TargetPlatform.Linux:
                app = "bash";
                break;
            default: throw new InvalidPlatformException(BuildPlatform);
            }
            Utilities.Run(app, path, null, workspace, Utilities.RunOptions.None, envVars);
        }
    }
}
