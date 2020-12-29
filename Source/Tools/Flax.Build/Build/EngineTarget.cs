// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using System;
using System.IO;

namespace Flax.Build
{
    /// <summary>
    /// The build target that builds the engine (eg. as game player or editor).
    /// </summary>
    /// <seealso cref="Flax.Build.ProjectTarget" />
    public class EngineTarget : ProjectTarget
    {
        private static Version _engineVersion;

        /// <summary>
        /// Gets the engine version (from Version.h in the engine source).
        /// </summary>
        public static Version EngineVersion
        {
            get
            {
                if (_engineVersion == null)
                {
                    _engineVersion = ProjectInfo.Load(Path.Combine(Globals.EngineRoot, "Flax.flaxproj")).Version;
                    Log.Verbose(string.Format("Engine build version: {0}", _engineVersion));
                }
                return _engineVersion;
            }
        }

        /// <inheritdoc />
        public override void Init()
        {
            base.Init();

            // Merge all modules from engine source into a single executable and single C# library
            LinkType = TargetLinkType.Monolithic;

            Modules.Add("Main");
            Modules.Add("Engine");
        }
    }
}
