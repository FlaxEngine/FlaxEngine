// Copyright (c) 2012-2024 Flax Engine. All rights reserved.

namespace Flax.Build
{
    /// <summary>
    /// The target platform types.
    /// </summary>
    public enum TargetPlatform
    {
        /// <summary>
        /// Running on Windows.
        /// </summary>
        Windows = 1,

        /// <summary>
        /// Running on Xbox One.
        /// </summary>
        XboxOne = 2,

        /// <summary>
        /// Running Windows Store App (Universal Windows Platform).
        /// </summary>
        UWP = 3,

        /// <summary>
        /// Running on Linux.
        /// </summary>
        Linux = 4,

        /// <summary>
        /// Running on PlayStation 4.
        /// </summary>
        PS4 = 5,

        /// <summary>
        /// Running on Xbox Series X.
        /// </summary>
        XboxScarlett = 6,

        /// <summary>
        /// Running on Android.
        /// </summary>
        Android = 7,

        /// <summary>
        /// Running on Switch.
        /// </summary>
        Switch = 8,

        /// <summary>
        /// Running on PlayStation 5.
        /// </summary>
        PS5 = 9,

        /// <summary>
        /// Running on Mac.
        /// </summary>
        Mac = 10,

        /// <summary>
        /// Running on iPhone.
        /// </summary>
        iOS = 11,
    }

    /// <summary>
    /// The target platform architecture types.
    /// </summary>
    public enum TargetArchitecture
    {
        /// <summary>
        /// Anything or not important.
        /// </summary>
        AnyCPU = 0,

        /// <summary>
        /// The x86 32-bit.
        /// </summary>
        x86 = 1,

        /// <summary>
        /// The x86 64-bit.
        /// </summary>
        x64 = 2,

        /// <summary>
        /// The ARM 32-bit.
        /// </summary>
        ARM = 3,

        /// <summary>
        /// The ARM 64-bit.
        /// </summary>
        ARM64 = 4,
    }

    /// <summary>
    /// The target configuration modes.
    /// </summary>
    public enum TargetConfiguration
    {
        /// <summary>
        /// Debug configuration. Without optimizations but with full debugging information.
        /// </summary>
        Debug = 0,

        /// <summary>
        /// Development configuration. With basic optimizations and partial debugging data.
        /// </summary>
        Development = 1,

        /// <summary>
        /// Shipping configuration. With full optimization and no debugging data.
        /// </summary>
        Release = 2,
    }
}
