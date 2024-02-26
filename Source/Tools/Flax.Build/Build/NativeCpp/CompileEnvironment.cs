// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;

namespace Flax.Build.NativeCpp
{
    /// <summary>
    /// The compilation optimization hint.
    /// </summary>
    public enum FavorSizeOrSpeed
    {
        /// <summary>
        /// The default option.
        /// </summary>
        Neither,

        /// <summary>
        /// The fast code.
        /// </summary>
        FastCode,

        /// <summary>
        /// The small code.
        /// </summary>
        SmallCode
    }

    /// <summary>
    /// The compilation optimization hint.
    /// </summary>
    public enum CppVersion
    {
        /// <summary>
        /// C++14
        /// </summary>
        Cpp14,

        /// <summary>
        /// C++17
        /// </summary>
        Cpp17,

        /// <summary>
        /// C++20
        /// </summary>
        Cpp20,

        /// <summary>
        /// The latest version supported by the compiler.
        /// </summary>
        Latest,
    }

    /// <summary>
    /// The C++ compilation environment required to build source files in the native modules.
    /// </summary>
    public class CompileEnvironment : ICloneable
    {
        /// <summary>
        /// C++ standard version to use for compilation.
        /// </summary>
        public CppVersion CppVersion = CppVersion.Cpp14;

        /// <summary>
        /// Selects a predefined set of options that affect the size and speed of generated code.
        /// </summary>
        public FavorSizeOrSpeed FavorSizeOrSpeed = FavorSizeOrSpeed.Neither;

        /// <summary>
        /// Enables exceptions support.
        /// </summary>
        public bool EnableExceptions = false;

        /// <summary>
        /// Enables RTTI.
        /// </summary>
        public bool RuntimeTypeInfo = true;

        /// <summary>
        /// Enables functions inlining support.
        /// </summary>
        public bool Inlining = false;

        /// <summary>
        /// Enables code optimization.
        /// </summary>
        public bool Optimization = false;

        /// <summary>
        /// Enables the whole program optimization.
        /// </summary>
        public bool WholeProgramOptimization = false;

        /// <summary>
        /// Enables functions level linking support.
        /// </summary>
        public bool FunctionLevelLinking = false;

        /// <summary>
        /// Enables debug information generation.
        /// </summary>
        public bool DebugInformation = false;

        /// <summary>
        /// Hints to use Debug version of the standard library.
        /// </summary>
        public bool UseDebugCRT = false;

        /// <summary>
        /// Hints to compile code for WinRT.
        /// </summary>
        public bool CompileAsWinRT = false;

        /// <summary>
        /// Enables WinRT component extensions.
        /// </summary>
        public bool WinRTComponentExtensions = false;

        /// <summary>
        /// Enables documentation generation.
        /// </summary>
        public bool GenerateDocumentation = false;

        /// <summary>
        /// Enables runtime checks.
        /// </summary>
        public bool RuntimeChecks = false;

        /// <summary>
        /// Enables string pooling.
        /// </summary>
        public bool StringPooling = false;

        /// <summary>
        /// Enables the compiler intrinsic functions.
        /// </summary>
        public bool IntrinsicFunctions = false;

        /// <summary>
        /// Enables buffer security checks.
        /// </summary>
        public bool BufferSecurityCheck = true;

        /// <summary>
        /// Hints to treat warnings as errors.
        /// </summary>
        public bool TreatWarningsAsErrors = false;

        /// <summary>
        /// The collection of defines with preprocessing symbol for a source files.
        /// </summary>
        public readonly HashSet<string> PreprocessorDefinitions = new HashSet<string>();

        /// <summary>
        /// The additional paths to add to the list of directories searched for include files.
        /// </summary>
        public readonly List<string> IncludePaths = new List<string>();

        /// <summary>
        /// The collection of custom arguments to pass to the compilator.
        /// </summary>
        public readonly HashSet<string> CustomArgs = new HashSet<string>();

        /// <summary>
        /// The Precompiled Header File (PCH) usage.
        /// </summary>
        public PrecompiledHeaderFileUsage PrecompiledHeaderUsage = PrecompiledHeaderFileUsage.None;

        /// <summary>
        /// The Precompiled Header File (PCH) binary path. Null if not created.
        /// </summary>
        public string PrecompiledHeaderFile;

        /// <summary>
        /// The Precompiled Header File (PCH) source path. Null if not provided.
        /// </summary>
        public string PrecompiledHeaderSource;

        /// <inheritdoc />
        public object Clone()
        {
            var clone = new CompileEnvironment
            {
                CppVersion = CppVersion,
                FavorSizeOrSpeed = FavorSizeOrSpeed,
                EnableExceptions = EnableExceptions,
                RuntimeTypeInfo = RuntimeTypeInfo,
                Inlining = Inlining,
                Optimization = Optimization,
                WholeProgramOptimization = WholeProgramOptimization,
                FunctionLevelLinking = FunctionLevelLinking,
                DebugInformation = DebugInformation,
                UseDebugCRT = UseDebugCRT,
                CompileAsWinRT = CompileAsWinRT,
                WinRTComponentExtensions = WinRTComponentExtensions,
                GenerateDocumentation = GenerateDocumentation,
                RuntimeChecks = RuntimeChecks,
                StringPooling = StringPooling,
                IntrinsicFunctions = IntrinsicFunctions,
                BufferSecurityCheck = BufferSecurityCheck,
                TreatWarningsAsErrors = TreatWarningsAsErrors,
                PrecompiledHeaderUsage = PrecompiledHeaderUsage,
                PrecompiledHeaderFile = PrecompiledHeaderFile,
                PrecompiledHeaderSource = PrecompiledHeaderSource,
            };
            clone.PreprocessorDefinitions.AddRange(PreprocessorDefinitions);
            clone.IncludePaths.AddRange(IncludePaths);
            clone.CustomArgs.AddRange(CustomArgs);
            return clone;
        }
    }
}
