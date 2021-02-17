// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

namespace FlaxEngine
{
    partial class Scene
    {
        /// <summary>
        /// The scene asset typename. Type of the serialized scene asset data. Hidden class for the scene assets. Actors deserialization rules are strictly controlled under the hood by the C++ core parts. Mostly because scene asset has the same ID as scene root actor so loading both managed objects for scene asset and scene will crash (due to object ids conflict).
        /// </summary>
        public const string AssetTypename = "FlaxEngine.SceneAsset";

        /// <summary>
        /// The scene asset typename used by the Editor asset picker control. Use it for asset reference picker filter.
        /// </summary>
        public const string EditorPickerTypename = "FlaxEditor.Content.SceneItem";
    }
}
