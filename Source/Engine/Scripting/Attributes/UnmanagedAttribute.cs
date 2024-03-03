// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine
{
    /// <summary>
    /// Marks the types and members defined in unmanaged code (native C++).
    /// </summary>
    [AttributeUsage(AttributeTargets.All)]
    public sealed class UnmanagedAttribute : Attribute
    {
    }
}
