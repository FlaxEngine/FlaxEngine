// Copyright (c) Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine
{
    /// <summary>
    /// Makes a variable show up in the editor.
    /// </summary>
    /// <remarks>
    /// If used on a private field/property you may also need to add <see cref="SerializeAttribute"/> to ensure that modified value is being serialized.
    /// </remarks>
    public sealed class ShowInEditorAttribute : Attribute
    {
    }
}
