// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.Content
{
    /// <summary>
    /// Content item that contains C++ script file with source code.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.ScriptItem" />
    public class CppScriptItem : ScriptItem
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="CppScriptItem"/> class.
        /// </summary>
        /// <param name="path">The path to the item.</param>
        public CppScriptItem(string path)
        : base(path)
        {
        }

        /// <inheritdoc />
        public override SpriteHandle DefaultThumbnail => Editor.Instance.Icons.CPPScript128;
    }
}
