// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using Flax.Build;
using Flax.Build.Platforms;
using Flax.Build.Projects.VisualStudio;
using Flax.Deploy;

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
        /// True if build dependency by default, otherwise only when explicitly specified via command line.
        /// </summary>
        public virtual bool BuildByDefault => true;

        /// <summary>
        /// Builds the dependency package using the specified options.
        /// </summary>
        /// <param name="options">The options.</param>
        public abstract void Build(BuildOptions options);

        /// <summary>
        /// Logs build process start.
        /// </summary>
        /// <param name="platform">Target platform.</param>
        protected void BuildStarted(TargetPlatform platform)
        {
            Log.Info($"Building {GetType().Name} for {platform}");
        }

        /// <summary>
        /// Gets the dependency third-party packages binaries folder.
        /// </summary>
        /// <param name="options">The options.</param>
        /// <param name="platform">The target platform.</param>
        /// <param name="architecture">The target architecture.</param>
        /// <param name="createIfMissing">Auto-create directory if it's missing.</param>
        /// <returns>The absolute path to the deps folder for the given platform and architecture configuration.</returns>
        public static string GetThirdPartyFolder(BuildOptions options, TargetPlatform platform, TargetArchitecture architecture, bool createIfMissing = true)
        {
            var path = Path.Combine(options.PlatformsFolder, platform.ToString(), "Binaries", "ThirdParty", architecture.ToString());
            if (createIfMissing && !Directory.Exists(path))
                Directory.CreateDirectory(path);
            return path;
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
        /// Clones the directory.
        /// </summary>
        /// <param name="src">The source folder path.</param>
        /// <param name="dst">The destination folder path.</param>
        public static void CloneDirectory(string src, string dst)
        {
            if (Directory.Exists(dst))
                Utilities.DirectoryDelete(dst);
            Utilities.DirectoryCopy(src, dst);
        }

        /// <summary>
        /// Clones the git repository from the remote url (full repository).
        /// </summary>
        /// <param name="path">The local path for close.</param>
        /// <param name="url">The remote url.</param>
        /// <param name="commit">The commit to checkout.</param>
        /// <param name="args">The custom arguments to add to the clone command.</param>
        /// <param name="submodules">True if initialize submodules of the repository (recursive).</param>
        public static void CloneGitRepo(string path, string url, string commit = null, string args = null, bool submodules = false)
        {
            if (!Directory.Exists(Path.Combine(path, ".git")))
            {
                string cmdLine = string.Format("clone \"{0}\" \"{1}\"", url, path);
                if (args != null)
                    cmdLine += " " + args;
                if (submodules)
                    cmdLine += " --recurse-submodules";

                Utilities.Run("git", cmdLine, null, path, Utilities.RunOptions.DefaultTool);
                if (submodules)
                    Utilities.Run("git", "submodule update --init --recursive", null, path, Utilities.RunOptions.DefaultTool);
            }
            if (commit != null)
                Utilities.Run("git", string.Format("reset --hard {0}", commit), null, path, Utilities.RunOptions.DefaultTool);
        }

        /// <summary>
        /// Clones the git repository from the remote url.
        /// </summary>
        /// <param name="path">The local path for close.</param>
        /// <param name="url">The remote url.</param>
        /// <param name="args">The custom arguments to add to the clone command.</param>
        /// <param name="submodules">True if initialize submodules of the repository (recursive).</param>
        public static void CloneGitRepoFast(string path, string url, string args = null, bool submodules = false)
        {
            if (!Directory.Exists(Path.Combine(path, ".git")))
            {
                string cmdLine = string.Format("clone \"{0}\" \"{1}\" --depth 1", url, path);
                if (args != null)
                    cmdLine += " " + args;
                if (submodules)
                    cmdLine += " --recurse-submodules";

                Utilities.Run("git", cmdLine, null, path, Utilities.RunOptions.DefaultTool);
                if (submodules)
                    Utilities.Run("git", "submodule update --init --recursive", null, path, Utilities.RunOptions.DefaultTool);
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
        /// <param name="submodules">True if initialize submodules of the repository (recursive).</param>
        public static void CloneGitRepoSingleBranch(string path, string url, string branch, string commit = null, string args = null, bool submodules = false)
        {
            if (!Directory.Exists(Path.Combine(path, ".git")))
            {
                string cmdLine = string.Format("clone --single-branch --branch {2} \"{0}\" \"{1}\"", url, path, branch);
                if (commit == null)
                    cmdLine += " --depth 1";
                if (args != null)
                    cmdLine += " " + args;
                if (submodules)
                    cmdLine += " --recurse-submodules";

                Utilities.Run("git", cmdLine, null, path, Utilities.RunOptions.DefaultTool);
                if (submodules)
                    Utilities.Run("git", "submodule update --init --recursive", null, path, Utilities.RunOptions.DefaultTool);
            }

            if (commit != null)
            {
                Utilities.Run("git", string.Format("reset --hard {0}", commit), null, path, Utilities.RunOptions.DefaultTool);
            }
        }

        /// <summary>
        /// Changes the current branch to the given branch in the git repository (resets all the local changes). Does not perform pull command to update branch with remote repo.
        /// </summary>
        /// <param name="path">The local path that contains git repository.</param>
        /// <param name="branch">The name of the branch to checkout.</param>
        /// <param name="commit">The commit to checkout.</param>
        /// <param name="args">The custom arguments to add to the clone command.</param>
        /// <param name="submodules">True if initialize submodules of the repository (recursive).</param>
        public static void GitCheckout(string path, string branch, string commit = null, string args = null, bool submodules = false)
        {
            string cmdLine = string.Format("checkout -B {0} origin/{0}", branch);
            if (args != null)
                cmdLine += " " + args;
            if (submodules)
                cmdLine += " --recurse-submodules";

            Utilities.Run("git", cmdLine, null, path, Utilities.RunOptions.DefaultTool);
            if (submodules)
                Utilities.Run("git", "submodule update --init --recursive", null, path, Utilities.RunOptions.DefaultTool);

            if (commit != null)
            {
                Utilities.Run("git", string.Format("reset --hard {0}", commit), null, path, Utilities.RunOptions.DefaultTool);
            }
        }

        /// <summary>
        /// Resets all the local changes.
        /// </summary>
        /// <param name="path">The local path that contains git repository.</param>
        public static void GitResetLocalChanges(string path)
        {
            Utilities.Run("git", "reset --hard", null, path, Utilities.RunOptions.DefaultTool);
        }

        /// <summary>
        /// Builds the cmake project.
        /// </summary>
        /// <param name="path">The path.</param>
        /// <param name="envVars">Custom environment variables to pass to the child process.</param>
        public static void BuildCmake(string path, Dictionary<string, string> envVars = null)
        {
            Utilities.Run("cmake", "--build .  --config Release", null, path, Utilities.RunOptions.DefaultTool, envVars);
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
                    arch = "Win32";
                    break;
                case TargetArchitecture.x64:
                    arch = "x64";
                    break;
                case TargetArchitecture.ARM:
                    arch = "ARM";
                    break;
                case TargetArchitecture.ARM64:
                    arch = "ARM64";
                    break;
                default: throw new InvalidArchitectureException(architecture);
                }
                cmdLine = string.Format("CMakeLists.txt -G \"Visual Studio 17 2022\" -A {0}", arch);
                break;
            }
            case TargetPlatform.PS4:
                cmdLine = "CMakeLists.txt -DCMAKE_GENERATOR_PLATFORM=ORBIS -G \"Visual Studio 15 2017\"";
                break;
            case TargetPlatform.PS5:
                cmdLine = "CMakeLists.txt -DCMAKE_GENERATOR_PLATFORM=PROSPERO -G \"Visual Studio 16 2019\"";
                break;
            case TargetPlatform.Linux:
                cmdLine = "CMakeLists.txt";
                break;
            case TargetPlatform.Switch:
                cmdLine = string.Format("-DCMAKE_TOOLCHAIN_FILE=\"{1}\\Source\\Platforms\\Switch\\Binaries\\Data\\Switch.cmake\" -G \"NMake Makefiles\" -DCMAKE_MAKE_PROGRAM=\"{0}..\\..\\VC\\bin\\nmake.exe\"", Environment.GetEnvironmentVariable("VS140COMNTOOLS"), Globals.EngineRoot);
                break;
            case TargetPlatform.Android:
            {
                var ndk = AndroidNdk.Instance.RootPath;
                var abi = AndroidToolchain.GetAbiName(architecture);
                var hostName = AndroidSdk.GetHostName();
                cmdLine = string.Format("-DCMAKE_TOOLCHAIN_FILE=\"{0}/build/cmake/android.toolchain.cmake\" -DANDROID_NDK=\"{0}\" -DANDROID_STL=c++_shared -DANDROID_ABI={1} -DANDROID_PLATFORM=android-{2} -G \"MinGW Makefiles\" -DCMAKE_MAKE_PROGRAM=\"{0}/prebuilt/{3}/bin/make.exe\"", ndk, abi, Configuration.AndroidPlatformApi, hostName);
                break;
            }
            case TargetPlatform.Mac:
            {
                var arch = GetAppleArchName(architecture);
                cmdLine = string.Format("CMakeLists.txt -DCMAKE_OSX_DEPLOYMENT_TARGET=\"{0}\" -DCMAKE_OSX_ARCHITECTURES={1}", Configuration.MacOSXMinVer, arch);
                break;
            }
            case TargetPlatform.iOS:
            {
                var arch = GetAppleArchName(architecture);
                cmdLine = string.Format("CMakeLists.txt -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_DEPLOYMENT_TARGET=\"{0}\" -DCMAKE_OSX_ARCHITECTURES={1}", Configuration.iOSMinVer, arch);
                break;
            }
            default: throw new InvalidPlatformException(platform);
            }

            if (customArgs != null)
                cmdLine += " " + customArgs;

            Utilities.Run("cmake", cmdLine, null, path, Utilities.RunOptions.DefaultTool, envVars);
        }

        /// <summary>
        /// Gets the Apple architecture name (for toolchain).
        /// </summary>
        public static string GetAppleArchName(TargetArchitecture architecture)
        {
            string arch;
            switch (architecture)
            {
            case TargetArchitecture.x86:
                arch = "i386";
                break;
            case TargetArchitecture.x64:
                arch = "x86_64";
                break;
            case TargetArchitecture.ARM:
                arch = "arm";
                break;
            case TargetArchitecture.ARM64:
                arch = "arm64";
                break;
            default: throw new InvalidArchitectureException(architecture);
            }
            return arch;
        }

        /// <summary>
        /// Runs the bash script via Cygwin tool (native bash on platforms other than Windows).
        /// </summary>
        /// <param name="path">The path.</param>
        /// <param name="workspace">The workspace folder.</param>
        public static void RunCygwin(string path, string workspace = null)
        {
            RunBash(path, string.Empty, workspace);
        }

        /// <summary>
        /// Runs the bash script (executes natively on Unix platforms, uses Cygwin on Windows).
        /// </summary>
        /// <param name="path">The script or command path.</param>
        /// <param name="args">The arguments.</param>
        /// <param name="workspace">The workspace folder.</param>
        /// <param name="envVars">Custom environment variables to pass to the child process.</param>
        public static void RunBash(string path, string args = null, string workspace = null, Dictionary<string, string> envVars = null)
        {
            switch (BuildPlatform)
            {
            case TargetPlatform.Windows:
            {
                // Find Cygwin
                var cygwinFolder = Environment.GetEnvironmentVariable("CYGWIN");
                if (string.IsNullOrEmpty(cygwinFolder) || !Directory.Exists(cygwinFolder))
                {
                    cygwinFolder = "C:\\cygwin";
                    if (!Directory.Exists(cygwinFolder))
                    {
                        cygwinFolder = "C:\\cygwin64";
                        if (!Directory.Exists(cygwinFolder))
                            throw new Exception("Missing Cygwin. Install Cygwin64 to C:\\cygwin or set CYGWIN env variable to install location folder.");
                    }
                }
                var cygwinBinFolder = Path.Combine(cygwinFolder, "bin");

                // Ensure that Cygwin binaries folder is in a PATH
                string envPath = null;
                envVars?.TryGetValue("PATH", out envPath);
                if (envPath == null || envPath.IndexOf(cygwinBinFolder, StringComparison.OrdinalIgnoreCase) == -1)
                {
                    if (envVars == null)
                        envVars = new Dictionary<string, string>();
                    envVars["PATH"] = cygwinBinFolder;
                    if (envPath != null)
                        envVars["PATH"] += ";" + envPath;
                }

                // Get the executable file path
                if (path.EndsWith(".sh", StringComparison.OrdinalIgnoreCase))
                {
                    // Bash script
                    if (args == null)
                        args = path.Replace('\\', '/');
                    else
                        args = path.Replace('\\', '/') + " " + args;
                    path = Path.Combine(cygwinBinFolder, "bash.exe");
                }
                else if (File.Exists(Path.Combine(cygwinBinFolder, path + ".exe")))
                {
                    // Tool (eg. make)
                    path = Path.Combine(cygwinBinFolder, path + ".exe");
                }
                else
                {
                    throw new Exception("Cannot execute command " + path + " with args " + args);
                }
                break;
            }
            case TargetPlatform.Linux:
            case TargetPlatform.Mac: break;
            default: throw new InvalidPlatformException(BuildPlatform);
            }
            Utilities.Run(path, args, null, workspace, Utilities.RunOptions.ThrowExceptionOnError, envVars);
        }

        internal bool GetMsBuildForPlatform(TargetPlatform targetPlatform, out VisualStudioVersion vsVersion, out string msBuildPath)
        {
            // Some consoles don't support the latest Visual Studio 2022
            vsVersion = VisualStudioVersion.VisualStudio2022;
            switch (targetPlatform)
            {
            case TargetPlatform.PS4:
                vsVersion = VisualStudioVersion.VisualStudio2017;
                break;
            case TargetPlatform.PS5:
            case TargetPlatform.Switch:
                vsVersion = VisualStudioVersion.VisualStudio2019;
                break;
            }
            if (vsVersion != VisualStudioVersion.VisualStudio2022)
            {
                var visualStudioInstances = VisualStudioInstance.GetInstances();
                foreach (var visualStudioInstance in visualStudioInstances)
                {
                    if (visualStudioInstance.Version <= vsVersion)
                    {
                        var toolPath = Path.Combine(visualStudioInstance.Path, "MSBuild\\Current\\Bin\\MSBuild.exe");
                        if (!File.Exists(toolPath))
                            toolPath = Path.Combine(visualStudioInstance.Path, "MSBuild\\15.0\\Bin\\MSBuild.exe");
                        if (File.Exists(toolPath))
                        {
                            vsVersion = visualStudioInstance.Version;
                            msBuildPath = toolPath;
                            return true;
                        }
                    }
                }
            }
            msBuildPath = VCEnvironment.MSBuildPath;
            return false;
        }
    }
}
