// Copyright (c) Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine
{
    /// <summary>
    /// Marks a generated interface property as a native scripting object interface reference.
    /// </summary>
    [AttributeUsage(AttributeTargets.Field | AttributeTargets.Property)]
    public sealed class ScriptingObjectInterfaceReferenceAttribute : Attribute
    {
    }
}
