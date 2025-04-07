// Copyright (c) Wojciech Figat. All rights reserved.

using System;

namespace Flax.Build
{
    /// <summary>
    /// The unsupported platform exception for code paths that require other platform.
    /// </summary>
    /// <seealso cref="System.Exception" />
    public class InvalidPlatformException : Exception
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="InvalidPlatformException"/> class.
        /// </summary>
        /// <param name="platform">The platform.</param>
        public InvalidPlatformException(TargetPlatform platform)
        : base(string.Format("Unknown platform {0}.", platform))
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="InvalidPlatformException"/> class.
        /// </summary>
        /// <param name="platform">The platform.</param>
        /// <param name="message">The additional message.</param>
        public InvalidPlatformException(TargetPlatform platform, string message)
        : base(string.Format("Unknown platform {0}. " + message, platform))
        {
        }
    }
}
