// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;

namespace Flax.Build.NativeCpp
{
    /// <summary>
    /// The native C++ module build settings container.
    /// </summary>
    public sealed class BuildOptions
    {
        /// <summary>
        /// The target that builds this module.
        /// </summary>
        public Target Target;

        /// <summary>
        /// The build platform.
        /// </summary>
        public Platform Platform;

        /// <summary>
        /// The build platform toolchain.
        /// </summary>
        public Toolchain Toolchain;

        /// <summary>
        /// The build architecture.
        /// </summary>
        public TargetArchitecture Architecture;

        /// <summary>
        /// The build configuration.
        /// </summary>
        public TargetConfiguration Configuration;

        /// <summary>
        /// The module compilation environment.
        /// </summary>
        public CompileEnvironment CompileEnv;

        /// <summary>
        /// The module linking environment.
        /// </summary>
        public LinkEnvironment LinkEnv;

        /// <summary>
        /// The source file directories. By default it contains the directory that contains this module file.
        /// </summary>
        public List<string> SourcePaths = new List<string>();

        /// <summary>
        /// The source files to include in module build.
        /// </summary>
        public List<string> SourceFiles = new List<string>();

        /// <summary>
        /// The collection of the modules that are required by this module (for linking). Inherited by the modules that include it.
        /// </summary>
        public List<string> PublicDependencies = new List<string>();

        /// <summary>
        /// The collection of the modules that are required by this module (for linking).
        /// </summary>
        public List<string> PrivateDependencies = new List<string>();

        /// <summary>
        /// The collection of defines with preprocessing symbol for a source files of this module. Inherited by the modules that include it.
        /// </summary>
        public readonly HashSet<string> PublicDefinitions = new HashSet<string>();

        /// <summary>
        /// The collection of defines with preprocessing symbol for a source files of this module.
        /// </summary>
        public readonly HashSet<string> PrivateDefinitions = new HashSet<string>();

        /// <summary>
        /// The collection of additional include paths for a source files of this module. Inherited by the modules that include it.
        /// </summary>
        public readonly HashSet<string> PublicIncludePaths = new HashSet<string>();

        /// <summary>
        /// The collection of additional include paths for a source files of this module.
        /// </summary>
        public readonly HashSet<string> PrivateIncludePaths = new HashSet<string>();

        /// <summary>
        /// The dependency files to include with output (additional debug files, dynamic libraries, etc.).
        /// </summary>
        public HashSet<string> DependencyFiles = new HashSet<string>();

        /// <summary>
        /// The optional dependency files to include with output (additional debug files, dynamic libraries, etc.). Missing files won't fail the build.
        /// </summary>
        public HashSet<string> OptionalDependencyFiles = new HashSet<string>();

        /// <summary>
        /// The list of libraries to link (typically external and third-party plugins).
        /// </summary>
        public HashSet<string> Libraries = new HashSet<string>();

        /// <summary>
        /// The list of libraries to link for delay-load (typically external and third-party plugins).
        /// </summary>
        public HashSet<string> DelayLoadLibraries = new HashSet<string>();

        /// <summary>
        /// The build output files (binaries, object files and static or dynamic libraries).
        /// </summary>
        public List<string> OutputFiles = new List<string>();

        /// <summary>
        /// The intermediate build artifacts folder directory.
        /// </summary>
        public string IntermediateFolder;

        /// <summary>
        /// The output build artifacts folder directory.
        /// </summary>
        public string OutputFolder;

        /// <summary>
        /// The build commands working folder directory.
        /// </summary>
        public string WorkingDirectory;

        /// <summary>
        /// The hot reload postfix added to the output binaries.
        /// </summary>
        public string HotReloadPostfix;

        /// <summary>
        /// The full path to the dependencies folder for the current build platform, configuration, and architecture.
        /// </summary>
        public string DepsFolder => Path.Combine(Globals.EngineRoot, "Source", "Platforms", Platform.Target.ToString(), "Binaries", "ThirdParty", Architecture.ToString());

        /// <summary>
        /// The scripting API building options.
        /// </summary>
        public struct ScriptingAPIOptions
        {
            /// <summary>
            /// The preprocessor defines.
            /// </summary>
            public HashSet<string> Defines;

            /// <summary>
            /// The system libraries references.
            /// </summary>
            public HashSet<string> SystemReferences;

            /// <summary>
            /// The .Net libraries references (dll or exe files paths).
            /// </summary>
            public HashSet<string> FileReferences;

            /// <summary>
            /// True if ignore compilation warnings due to missing code documentation comments.
            /// </summary>
            public bool IgnoreMissingDocumentationWarnings;

            /// <summary>
            /// Adds the other options into this.
            /// </summary>
            /// <param name="other">The other.</param>
            public void Add(ScriptingAPIOptions other)
            {
                Defines.AddRange(other.Defines);
                SystemReferences.AddRange(other.SystemReferences);
                FileReferences.AddRange(other.FileReferences);
                IgnoreMissingDocumentationWarnings |= other.IgnoreMissingDocumentationWarnings;
            }
        }

        /// <summary>
        /// The scripting API building options.
        /// </summary>
        public ScriptingAPIOptions ScriptingAPI = new ScriptingAPIOptions
        {
            Defines = new HashSet<string>(),
            SystemReferences = new HashSet<string>
            {
                "System",
                "System.Xml",
                "System.Core",
            },
            FileReferences = new HashSet<string>(),
        };

        /// <summary>
        /// The external module linking options.
        /// </summary>
        public struct ExternalModule : IEquatable<ExternalModule>
        {
            public enum Types
            {
                Native,
                CSharp,
                Custom,
            }

            public string Name;
            public string Path;
            public Types Type;

            public ExternalModule(Types type, string path, string name = null)
            {
                Name = name ?? System.IO.Path.GetFileNameWithoutExtension(path);
                Type = type;
                Path = path;
            }

            /// <inheritdoc />
            public bool Equals(ExternalModule other)
            {
                return Name == other.Name;
            }

            /// <inheritdoc />
            public override bool Equals(object obj)
            {
                return obj is ExternalModule other && Equals(other);
            }

            /// <inheritdoc />
            public override int GetHashCode()
            {
                return Name.GetHashCode();
            }

            /// <inheritdoc />
            public override string ToString()
            {
                return Name;
            }
        }

        /// <summary>
        /// The custom external binary modules to referenced by this module. Can be used to extern the custom C# or C++ library with scripts to be used at runtime.
        /// </summary>
        public HashSet<ExternalModule> ExternalModules = new HashSet<ExternalModule>();

        /// <summary>
        /// Merges the files from input source paths into source files and clears the source paths list.
        /// </summary>
        internal void MergeSourcePathsIntoSourceFiles()
        {
            if (SourcePaths.Count == 0)
                return;

            using (new ProfileEventScope("MergeSourcePathsIntoSourceFiles"))
            {
                for (var i = 0; i < SourcePaths.Count; i++)
                {
                    var path = SourcePaths[i];
                    if (!Directory.Exists(path))
                        continue;
                    var files = Directory.GetFiles(path, "*", SearchOption.AllDirectories);
                    var count = SourceFiles.Count;
                    if (SourceFiles.Count == 0)
                    {
                        SourceFiles.AddRange(files);
                    }
                    else
                    {
                        for (int j = 0; j < files.Length; j++)
                        {
                            bool unique = true;
                            for (int k = 0; k < count; k++)
                            {
                                if (SourceFiles[k] == files[j])
                                {
                                    unique = false;
                                    break;
                                }
                            }
                            if (unique)
                                SourceFiles.Add(files[j]);
                        }
                    }
                }
                SourcePaths.Clear();
            }
        }
    }
}
