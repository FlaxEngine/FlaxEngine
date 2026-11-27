// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
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
        /// The layer names.
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
        /// The light layer names.
        /// </summary>
        [EditorOrder(10), EditorDisplay("Light Layers", EditorDisplayAttribute.InlineStyle), Collection(CanResize = false, Display = CollectionAttribute.DisplayType.Inline)]
        public string[] LightLayers = new string[32];

        /// <summary>
        /// Gets the current light layer names. Returns "Layer" + index for layers without a name.
        /// </summary>
        /// <returns>The light layer names.</returns>
        public static string[] GetCurrentLightLayers()
        {
#if FLAX_TESTS
    return System.Array.Empty<string>();
#else      
            var settings = GameSettings.Load<LayersAndTagsSettings>();
            var layerNames = (string[])settings.LightLayers.Clone();
            for (int i = 0; i < layerNames.Length; i++)
            {
                if (string.IsNullOrEmpty(layerNames[i]))
                    layerNames[i] = $"Layer {i}";
            }
            return layerNames;
#endif
        }

        /// <summary>
        /// The layer names.
        /// </summary>
        [EditorOrder(10), EditorDisplay("Terrain Layers", EditorDisplayAttribute.InlineStyle), Collection(CanResize = false, Display = CollectionAttribute.DisplayType.Inline)]
        public string[] TerrainLayers = new string[8];

        /// <summary>
        /// Gets the current terrain layer names. Returns "Layer" + index for layers without a name.
        /// </summary>
        /// <returns>The layer names.</returns>
        public static string[] GetCurrentTerrainLayers()
        {
#if FLAX_TESTS
            return System.Array.Empty<string>();
#else
            string[] layerNames = GameSettings.Load<LayersAndTagsSettings>().TerrainLayers;
            for (int i = 0; i < layerNames.Length; i++)
            {
                if (string.IsNullOrEmpty(layerNames[i]))
                    layerNames[i] = $"Layer {i}";
            }
            return layerNames;
#endif
        }

        [LibraryImport("FlaxEngine", EntryPoint = "LayersAndTagsSettingsInternal_GetCurrentLayers", StringMarshalling = StringMarshalling.Custom, StringMarshallingCustomType = typeof(FlaxEngine.Interop.StringMarshaller))]
        [return: MarshalUsing(typeof(FlaxEngine.Interop.ArrayMarshaller<,>), CountElementName = "layerCount")]
        internal static partial string[] GetCurrentLayers(out int layerCount);
    }
}
