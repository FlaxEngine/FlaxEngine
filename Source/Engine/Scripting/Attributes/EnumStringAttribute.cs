// Copyright (c) Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine
{
    /// <summary>
    /// Changes enum serialization to use string names instead of integer values. This makes saved data resilient to enum reordering or changes in values (but not to renaming enums). Deserialization accepts both string names and integer values for backward compatibility.
    /// </summary>
    /// <seealso cref="System.Attribute" />
    [AttributeUsage(AttributeTargets.Enum)]
    public sealed class EnumStringAttribute : Attribute
    {
    }
}
