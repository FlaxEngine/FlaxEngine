// Copyright (c) Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine
{
    /// <summary>
    /// Indicates that a field or a property of a serializable class should be serialized.
    /// The <see cref="FlaxEngine.ShowInEditorAttribute"/> attribute is required to show hidden fields in the editor.
    /// </summary>
    [AttributeUsage(AttributeTargets.Field | AttributeTargets.Property)]
    public sealed class SerializeAttribute : Attribute
    {
    }
}
