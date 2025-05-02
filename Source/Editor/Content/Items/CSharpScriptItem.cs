// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.Content
{
    /// <summary>
    /// Content item that contains C# file with source code.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.ScriptItem" />
    public class CSharpScriptItem : ScriptItem
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="CSharpScriptItem"/> class.
        /// </summary>
        /// <param name="path">The path to the item.</param>
        public CSharpScriptItem(string path)
        : base(path)
        {
        }

        /// <inheritdoc />
        public override string TypeDescription => "C# Source Code";

        /// <inheritdoc />
        public override SpriteHandle DefaultThumbnail => Editor.Instance.Icons.CSharpScript128;
    }
}
