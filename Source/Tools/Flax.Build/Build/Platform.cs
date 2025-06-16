// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Diagnostics;
using Flax.Build.NativeCpp;
using System.Runtime.InteropServices;

namespace Flax.Build
{
    /// <summary>
    /// The base class for all platform toolsets.
    /// </summary>
    public abstract class Platform
    {
        private static Platform _buildPlatform;
        private static Platform[] _platforms;
        private Dictionary<TargetArchitecture, Toolchain> _toolchains;
        private uint _failedArchitectures = 0;

        /// <summary>
        /// Gets the current target platform that build tool runs on.
        /// </summary>
        public static TargetPlatform BuildTargetPlatform
        {
            get
            {
                if (_buildPlatform == null)
                {
                    OperatingSystem os = Environment.OSVersion;
                    PlatformID platformId = os.Platform;
                    switch (platformId)
                    {
                    case PlatformID.Win32NT:
                    case PlatformID.Win32S:
                    case PlatformID.Win32Windows:
                    case PlatformID.WinCE: return TargetPlatform.Windows;
                    case PlatformID.Unix:
                    {
                        try
                        {
                            Process p = new Process
                            {
                                StartInfo =
                                {
                                    UseShellExecute = false,
                                    RedirectStandardOutput = true,
                                    FileName = "uname",
                                    Arguments = "-s",
                                }
                            };
                            p.Start();
                            string uname = p.StandardOutput.ReadToEnd().Trim();
                            if (uname == "Darwin")
                                return TargetPlatform.Mac;
                        }
                        catch (Exception)
                        {
                        }
                        return TargetPlatform.Linux;
                    }
                    case PlatformID.MacOSX: return TargetPlatform.Mac;
                    default: throw new NotImplementedException(string.Format("Unsupported build platform {0}.", platformId));
                    }
                }
                return _buildPlatform.Target;
            }
        }

        /// <summary>
        /// Gets the current target architecture that build tool runs on.
        /// </summary>
        public static TargetArchitecture BuildTargetArchitecture
        {
            get
            {
                var architectureId = RuntimeInformation.ProcessArchitecture;
                switch (architectureId)
                {
                case Architecture.X86: return TargetArchitecture.x86;
                case Architecture.X64: return TargetArchitecture.x64;
                case Architecture.Arm: return TargetArchitecture.ARM;
                case Architecture.Arm64: return TargetArchitecture.ARM64;
                default: throw new NotImplementedException(string.Format("Unsupported build platform {0}.", architectureId));
                }
            }
        }

        /// <summary>
        /// Gets the current platform that build tool runs on.
        /// </summary>
        public static Platform BuildPlatform
        {
            get
            {
                if (_buildPlatform == null)
                {
                    _buildPlatform = GetPlatform(BuildTargetPlatform);
                }
                return _buildPlatform;
            }
        }

        /// <summary>
        /// Gets the platform target type.
        /// </summary>
        public abstract TargetPlatform Target { get; }

        /// <summary>
        /// Gets a value indicating whether required external SDKs are installed for this platform.
        /// </summary>
        public abstract bool HasRequiredSDKsInstalled { get; }

        /// <summary>
        /// Gets a value indicating whether that platform supports shared libraries (dynamic link libraries).
        /// </summary>
        public abstract bool HasSharedLibrarySupport { get; }

        /// <summary>
        /// Gets a value indicating whether that platform supports building target into modular libraries (otherwise will force monolithic linking).
        /// </summary>
        public virtual bool HasModularBuildSupport => true;

        /// <summary>
        /// Gets a value indicating whether that platform supports using executable file as a reference when linking shared library. Otherwise, platform enforces monolithic linking or separate shared libraries usage.
        /// </summary>
        public virtual bool HasExecutableFileReferenceSupport => false;

        /// <summary>
        /// Gets a value indicating whether that platform supports executing native code generated dynamically (JIT), otherwise requires ahead-of-time compilation (AOT).
        /// </summary>
        public virtual bool HasDynamicCodeExecutionSupport => true;

        /// <summary>
        /// Gets the executable file extension (including leading dot).
        /// </summary>
        public abstract string ExecutableFileExtension { get; }

        /// <summary>
        /// Gets the shared library file extension (including leading dot).
        /// </summary>
        public abstract string SharedLibraryFileExtension { get; }

        /// <summary>
        /// Gets the static library file extension (including leading dot).
        /// </summary>
        public abstract string StaticLibraryFileExtension { get; }

        /// <summary>
        /// Gets the program database file extension (including leading dot).
        /// </summary>
        public abstract string ProgramDatabaseFileExtension { get; }

        /// <summary>
        /// Gets the executable files prefix.
        /// </summary>
        public virtual string ExecutableFilePrefix => string.Empty;

        /// <summary>
        /// Gets the shared library files prefix.
        /// </summary>
        public virtual string SharedLibraryFilePrefix => string.Empty;

        /// <summary>
        /// Gets the static library files prefix.
        /// </summary>
        public virtual string StaticLibraryFilePrefix => string.Empty;

        /// <summary>
        /// Gets the default project format used by the given platform.
        /// </summary>
        public abstract Projects.ProjectFormat DefaultProjectFormat { get; }

        /// <summary>
        /// Creates the toolchain for a given architecture.
        /// </summary>
        /// <param name="architecture">The architecture.</param>
        /// <returns>The toolchain.</returns>
        protected abstract Toolchain CreateToolchain(TargetArchitecture architecture);

        /// <summary>
        /// Determines whether this platform can build for the specified platform.
        /// </summary>
        /// <param name="platform">The platform.</param>
        /// <returns><c>true</c> if this platform can build the specified platform; otherwise, <c>false</c>.</returns>
        public virtual bool CanBuildPlatform(TargetPlatform platform)
        {
            return false;
        }

        /// <summary>
        /// Determines whether this platform can compile or cross-compile for the specified architecture.
        /// </summary>
        /// <param name="targetArchitecture">The architecture.</param>
        /// <returns><c>true</c> if this platform can build the specified architecture; otherwise, <c>false</c>.</returns>
        public virtual bool CanBuildArchitecture(TargetArchitecture targetArchitecture)
        {
            return IsPlatformSupported(Target, targetArchitecture);
        }

        /// <summary>
        /// Gets the path to the output file for the linker.
        /// </summary>
        /// <param name="name">The original library name.</param>
        /// <param name="output">The output file type.</param>
        /// <returns>The file name (including prefix, name and extension).</returns>
        public string GetLinkOutputFileName(string name, LinkerOutput output)
        {
            switch (output)
            {
            case LinkerOutput.Executable: return ExecutableFilePrefix + name + ExecutableFileExtension;
            case LinkerOutput.SharedLibrary: return SharedLibraryFilePrefix + name + SharedLibraryFileExtension;
            case LinkerOutput.StaticLibrary:
            case LinkerOutput.ImportLibrary: return StaticLibraryFilePrefix + name + StaticLibraryFileExtension;
            default: throw new ArgumentOutOfRangeException(nameof(output), output, null);
            }
        }

        /// <summary>
        /// Creates the build toolchain for a given platform and architecture.
        /// </summary>
        /// <param name="targetPlatform">The target platform.</param>
        /// <param name="nullIfMissing">True if return null platform if it's missing, otherwise will invoke an exception.</param>
        /// <returns>The platform.</returns>
        public static Platform GetPlatform(TargetPlatform targetPlatform, bool nullIfMissing = false)
        {
            if (_platforms == null)
            {
                using (new ProfileEventScope("GetPlatforms"))
                    _platforms = Builder.BuildTypes.Where(x => x.IsClass && !x.IsAbstract && x.IsSubclassOf(typeof(Platform))).Select(Activator.CreateInstance).Cast<Platform>().ToArray();
            }

            foreach (var platform in _platforms)
            {
                if (platform.Target == targetPlatform)
                {
                    return platform;
                }
            }

            if (nullIfMissing)
                return null;
            throw new Exception(string.Format("Platform {0} is not supported.", targetPlatform));
        }

        /// <summary>
        /// Tries to create the build toolchain for a given architecture. Returns null if platform is not supported.
        /// </summary>
        /// <param name="targetArchitecture">The target architecture.</param>
        /// <returns>The toolchain.</returns>
        public Toolchain TryGetToolchain(TargetArchitecture targetArchitecture)
        {
            Toolchain result = null;
            uint failedMask = 1u << (int)targetArchitecture; // Skip retrying if it already failed once on this arch
            if (HasRequiredSDKsInstalled && (_failedArchitectures & failedMask) != failedMask)
            {
                try
                {
                    result = GetToolchain(targetArchitecture);
                }
                catch (Exception ex)
                {
                    _failedArchitectures |= failedMask;
                    Log.Exception(ex);
                }
            }
            return result;
        }

        /// <summary>
        /// Creates the build toolchain for a given architecture.
        /// </summary>
        /// <param name="targetArchitecture">The target architecture.</param>
        /// <returns>The toolchain.</returns>
        public Toolchain GetToolchain(TargetArchitecture targetArchitecture)
        {
            if (!HasRequiredSDKsInstalled)
                throw new Exception(string.Format("Platform {0} has no required SDK installed and cannot be used.", Target));

            if (_toolchains == null)
                _toolchains = new Dictionary<TargetArchitecture, Toolchain>();

            Toolchain toolchain;
            if (_toolchains.TryGetValue(targetArchitecture, out toolchain))
                return toolchain;

            toolchain = CreateToolchain(targetArchitecture);
            _toolchains.Add(targetArchitecture, toolchain);

            return toolchain;
        }

        /// <summary>
        /// Gets path to the current platform binary directory 
        /// </summary>
        public static string GetEditorBinaryDirectory()
        {
            var subdir = "Binaries/Editor/";
            switch (Platform.BuildTargetPlatform)
            {
            case TargetPlatform.Windows:
            {
                switch (Platform.BuildTargetArchitecture)
                {
                case TargetArchitecture.x64:
                    return subdir + "Win64";
                case TargetArchitecture.x86:
                    return subdir + "Win32";
                case TargetArchitecture.ARM64:
                    return subdir + "ARM64";
                default:
                    throw new NotImplementedException($"{Platform.BuildTargetPlatform}: {Platform.BuildTargetArchitecture}");
                }
            }
            case TargetPlatform.Linux: return subdir + "Linux";
            case TargetPlatform.Mac: return subdir + "Mac";
            }
            throw new NotImplementedException(Platform.BuildTargetPlatform.ToString());
        }

        /// <summary>
        /// Creates the project files generator for the specified project format.
        /// </summary>
        /// <param name="targetPlatform">The target platform.</param>
        /// <param name="targetArchitecture">The target architecture.</param>
        /// <returns>True if the given platform is supported, otherwise false.</returns>
        public static bool IsPlatformSupported(TargetPlatform targetPlatform, TargetArchitecture targetArchitecture)
        {
            if (targetArchitecture == TargetArchitecture.AnyCPU)
                return true;

            switch (targetPlatform)
            {
            case TargetPlatform.Windows: return targetArchitecture == TargetArchitecture.x64 || targetArchitecture == TargetArchitecture.x86 || targetArchitecture == TargetArchitecture.ARM64;
            case TargetPlatform.XboxScarlett: return targetArchitecture == TargetArchitecture.x64;
            case TargetPlatform.XboxOne: return targetArchitecture == TargetArchitecture.x64;
            case TargetPlatform.UWP: return targetArchitecture == TargetArchitecture.x64;
            case TargetPlatform.Linux: return targetArchitecture == TargetArchitecture.x64;
            case TargetPlatform.PS4: return targetArchitecture == TargetArchitecture.x64;
            case TargetPlatform.PS5: return targetArchitecture == TargetArchitecture.x64;
            case TargetPlatform.Android: return targetArchitecture == TargetArchitecture.ARM64;
            case TargetPlatform.Switch: return targetArchitecture == TargetArchitecture.ARM64;
            case TargetPlatform.Mac: return targetArchitecture == TargetArchitecture.ARM64 || targetArchitecture == TargetArchitecture.x64;
            case TargetPlatform.iOS: return targetArchitecture == TargetArchitecture.ARM64;
            default: return false;
            }
        }
    }
}
