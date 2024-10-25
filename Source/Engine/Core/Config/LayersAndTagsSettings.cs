// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Runtime.InteropServices.Marshalling;
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
        [EditorOrder(10), EditorDisplay("Layers", EditorDisplayAttribute.InlineStyle), Collection(CanResize = false, Display = CollectionAttribute.DisplayType.Inline)]
        public string[] Layers = new string[32];

        /// <summary>
        /// Gets the current layer names (max 32 items but trims last empty items).
        /// </summary>
        /// <returns>The layers.</returns>
        public static string[] GetCurrentLayers()
        {
            return GetCurrentLayers(out int _);
        }

        /// <summary>
        /// The layers names.
        /// </summary>
        [EditorOrder(10), EditorDisplay("Terrain Layers", EditorDisplayAttribute.InlineStyle), Collection(CanResize = false, Display = CollectionAttribute.DisplayType.Inline)]
        public string[] TerrainLayers = new string[8];

        /// <summary>
        /// Gets the current terrain layer names. Returns "Layer" + index for layers without a name.
        /// </summary>
        /// <returns>The layer names.</returns>
        public static string[] GetCurrentTerrainLayers()
        {
            string[] layerNames = GameSettings.Load<LayersAndTagsSettings>().TerrainLayers.ToArray();

            for (int i = 0; i < layerNames.Length; i++)
            {
                if (string.IsNullOrEmpty(layerNames[i]))
                    layerNames[i] = $"Terrain Layer {i}";
            }

            return layerNames;
        }

        [LibraryImport("FlaxEngine", EntryPoint = "LayersAndTagsSettingsInternal_GetCurrentLayers", StringMarshalling = StringMarshalling.Custom, StringMarshallingCustomType = typeof(FlaxEngine.Interop.StringMarshaller))]
        [return: MarshalUsing(typeof(FlaxEngine.Interop.ArrayMarshaller<,>), CountElementName = "layerCount")]
        internal static partial string[] GetCurrentLayers(out int layerCount);
    }
}
