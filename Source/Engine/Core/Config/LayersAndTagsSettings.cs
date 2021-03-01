// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using System.Runtime.CompilerServices;
using FlaxEngine;

namespace FlaxEditor.Content.Settings
{
    partial class LayersAndTagsSettings
    {
        /// <summary>
        /// The tag names.
        /// </summary>
        [EditorOrder(10), EditorDisplay("Tags", EditorDisplayAttribute.InlineStyle)]
        public List<string> Tags = new List<string>();

        /// <summary>
        /// The layers names.
        /// </summary>
        [EditorOrder(10), EditorDisplay("Layers", EditorDisplayAttribute.InlineStyle), Collection(ReadOnly = true)]
        public string[] Layers = new string[32];

        /// <summary>
        /// Gets the current tags collection.
        /// </summary>
        /// <returns>The tags collection.</returns>
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern string[] GetCurrentTags();

        /// <summary>
        /// Gets the current layer names (max 32 items but trims last empty items).
        /// </summary>
        /// <returns>The layers.</returns>
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern string[] GetCurrentLayers();
    }
}
