// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using Flax.Build.NativeCpp;

namespace Flax.Build
{
    /// <summary>
    /// The base class for game modules.
    /// </summary>
    /// <seealso cref="Flax.Build.Module" />
    public abstract class GameModule : Module
    {
        /// <inheritdoc />
        public override void Setup(BuildOptions options)
        {
            base.Setup(options);

            // Reference main engine modules
            options.PublicDependencies.Add("Core");
            options.PublicDependencies.Add("Engine");
            options.PublicDependencies.Add("Level");
            options.PublicDependencies.Add("Scripting");

            // Setup scripting API environment
            var engineVersion = EngineTarget.EngineVersion;
            AddEngineVersionDefines(engineVersion, options.ScriptingAPI.Defines);
            AddEngineVersionDefines(engineVersion, options.CompileEnv.PreprocessorDefinitions);
            options.ScriptingAPI.Defines.Add("FLAX");
            options.ScriptingAPI.Defines.Add("FLAX_ASSERTIONS");
            if (options.Target.IsEditor)
            {
                options.ScriptingAPI.Defines.Add("FLAX_EDITOR");
            }
            else
            {
                options.ScriptingAPI.Defines.Add("FLAX_GAME");
            }
        }

        private void AddEngineVersionDefines(Version engineVersion, HashSet<string> defines)
        {
            defines.Add(string.Format("FLAX_{0}", engineVersion.Major));
            defines.Add(string.Format("FLAX_{0}_{1}", engineVersion.Major, engineVersion.Minor));
            for (int minor = 1; minor <= engineVersion.Minor; minor++)
                defines.Add(string.Format("FLAX_{0}_{1}_OR_NEWER", engineVersion.Major, minor));
        }
    }

    /// <summary>
    /// The base class for game editor modules.
    /// </summary>
    /// <seealso cref="Flax.Build.Module" />
    /// <seealso cref="Flax.Build.GameModule" />
    public abstract class GameEditorModule : GameModule
    {
        /// <inheritdoc />
        public override void Setup(BuildOptions options)
        {
            base.Setup(options);

            // Reference main editor modules
            options.PublicDependencies.Add("Editor");
        }
    }
}
