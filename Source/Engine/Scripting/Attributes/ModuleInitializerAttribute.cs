// Copyright (c) Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine
{
    /// <summary>
    /// Indicates that a static class initializes the code module. All static, void and parameterless methods within the class will be invoked upon module loading.
    /// </summary>
    [AttributeUsage(AttributeTargets.Class)]
    public sealed class ModuleInitializerAttribute : Attribute
    {
    }
}
