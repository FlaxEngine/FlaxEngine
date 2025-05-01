// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEngine;

namespace FlaxEditor
{
    partial class GameCooker
    {
        /// <summary>
        /// Build options data.
        /// </summary>
        public struct Options
        {
            /// <summary>
            /// The platform.
            /// </summary>
            public BuildPlatform Platform;

            /// <summary>
            /// The build configuration.
            /// </summary>
            public BuildConfiguration Configuration;

            /// <summary>
            /// The options.
            /// </summary>
            public BuildOptions Flags;

            /// <summary>
            /// The output path (normalized, absolute).
            /// </summary>
            public string OutputPath;
        }

        /// <summary>
        /// Building event type.
        /// </summary>
        public enum EventType
        {
            /// <summary>
            /// The build started.
            /// </summary>
            BuildStarted = 0,

            /// <summary>
            /// The build failed.
            /// </summary>
            BuildFailed = 1,

            /// <summary>
            /// The build done.
            /// </summary>
            BuildDone = 2,
        }

        /// <summary>
        /// Game building event delegate.
        /// </summary>
        /// <param name="type">The type.</param>
        public delegate void BuildEventDelegate(EventType type);

        /// <summary>
        /// Game building progress reporting delegate type.
        /// </summary>
        /// <param name="info">The information text.</param>
        /// <param name="totalProgress">The total progress percentage (normalized to 0-1).</param>
        public delegate void BuildProgressDelegate(string info, float totalProgress);

        /// <summary>
        /// Occurs when building event rises.
        /// </summary>
        public static event BuildEventDelegate Event;

        /// <summary>
        /// Occurs when building game progress fires.
        /// </summary>
        public static event BuildProgressDelegate Progress;

        /// <summary>
        /// Occurs when building collects assets to cook.
        /// </summary>
        public static event Action<List<Guid>> CollectAssets;

        /// <summary>
        /// Gets the type of the platform from the game build platform type.
        /// </summary>
        /// <param name="buildPlatform">The build platform.</param>
        /// <returns>The run-type platform type.</returns>
        public static PlatformType GetPlatformType(BuildPlatform buildPlatform)
        {
            switch (buildPlatform)
            {
            case BuildPlatform.Windows32:
            case BuildPlatform.Windows64: return PlatformType.Windows;
            case BuildPlatform.UWPx86:
            case BuildPlatform.UWPx64: return PlatformType.UWP;
            case BuildPlatform.XboxOne: return PlatformType.XboxOne;
            case BuildPlatform.LinuxX64: return PlatformType.Linux;
            case BuildPlatform.PS4: return PlatformType.PS4;
            case BuildPlatform.PS5: return PlatformType.PS5;
            case BuildPlatform.AndroidARM64: return PlatformType.Android;
            case BuildPlatform.XboxScarlett: return PlatformType.XboxScarlett;
            case BuildPlatform.Switch: return PlatformType.Switch;
            case BuildPlatform.MacOSARM64:
            case BuildPlatform.MacOSx64: return PlatformType.Mac;
            case BuildPlatform.iOSARM64: return PlatformType.iOS;
            default: throw new ArgumentOutOfRangeException(nameof(buildPlatform), buildPlatform, null);
            }
        }

        internal static void Internal_OnEvent(EventType type)
        {
            Event?.Invoke(type);
        }

        internal static void Internal_OnProgress(string info, float totalProgress)
        {
            Progress?.Invoke(info, totalProgress);
        }

        internal static Guid[] Internal_OnCollectAssets()
        {
            var list = new List<Guid>();

            // Custom assets
            CollectAssets?.Invoke(list);

            // Plugin assets
            foreach (var plugin in PluginManager.GamePlugins)
            {
                var pluginRefs = plugin.GetReferences();
                if (pluginRefs != null)
                    list.AddRange(pluginRefs);
            }

            if (list.Count == 0)
                return null;
            return list.ToArray();
        }
    }
}
