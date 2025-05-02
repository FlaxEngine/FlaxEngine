// Copyright (c) Wojciech Figat. All rights reserved.

using System.IO;
using Flax.Build.NativeCpp;

namespace Flax.Build
{
    /// <summary>
    /// The build module that is a part of the engine.
    /// </summary>
    /// <seealso cref="Flax.Build.Target" />
    public class EngineModule : Module
    {
        /// <inheritdoc />
        public override void Setup(BuildOptions options)
        {
            base.Setup(options);

            if (Name != "Core")
            {
                // All Engine modules include Core module
                options.PrivateDependencies.Add("Core");
            }

            if (options.Target.IsEditor)
            {
                // Use Debug module by default in Editor
                if (Name != "Debug")
                    options.PrivateDependencies.Add("Debug");

                options.ScriptingAPI.Defines.Add("FLAX_EDITOR");
            }
            else
            {
                options.ScriptingAPI.Defines.Add("FLAX_GAME");
            }

            // Use custom precompiled header file for the engine to boost compilation time
            options.CompileEnv.PrecompiledHeaderUsage = PrecompiledHeaderFileUsage.CreateManual;
            options.CompileEnv.PrecompiledHeaderSource = Utilities.NormalizePath(Path.Combine(Globals.EngineRoot, "Source/FlaxEngine.pch.h"));

            BinaryModuleName = "FlaxEngine";
            options.ScriptingAPI.Defines.Add("FLAX");
            options.ScriptingAPI.Defines.Add("FLAX_ASSERTIONS");
            options.ScriptingAPI.FileReferences.Add(Utilities.RemovePathRelativeParts(Path.Combine(Globals.EngineRoot, "Source", "Platforms", "DotNet", "Newtonsoft.Json.dll")));
            options.ScriptingAPI.SystemReferences.Add("System.ComponentModel.TypeConverter");
        }
    }
}
