// Copyright (c) Wojciech Figat. All rights reserved.

using System;

namespace Flax.Build
{
    internal class BuildException : Exception
    {
        public BuildException(string message)
        : base(message)
        {
        }
    }
}
