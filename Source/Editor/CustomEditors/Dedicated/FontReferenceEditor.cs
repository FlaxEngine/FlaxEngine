// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using FlaxEditor.CustomEditors.Editors;
using FlaxEditor.Scripting;
using FlaxEngine;

namespace FlaxEditor.CustomEditors.Dedicated
{
    /// <summary>
    /// Custom editor for <see cref="FontReference"/>.
    /// </summary>
    /// <seealso cref="GenericEditor" />
    [CustomEditor(typeof(FontReference)), DefaultEditor]
    public class FontReferenceEditor : GenericEditor
    {
        /// <inheritdoc />
        protected override List<ItemInfo> GetItemsForType(ScriptType type)
        {
            // Show properties
            return GetItemsForType(type, true, false);
        }
    }
}
