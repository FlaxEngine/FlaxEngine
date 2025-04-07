// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/Guid.h"
#include "Engine/Core/Math/Rectangle.h"
#include "Engine/Core/ISerializable.h"

#if USE_EDITOR
// Additional options used in editor for lightmaps baking
extern bool IsRunningRadiancePass;
extern bool IsBakingLightmaps;
extern bool EnableLightmapsUsage;
#endif

/// <summary>
/// Single lightmap info data
/// </summary>
struct SavedLightmapInfo
{
    /// <summary>
    /// Lightmap 0 texture ID
    /// </summary>
    Guid Lightmap0;

    /// <summary>
    /// Lightmap 1 texture ID
    /// </summary>
    Guid Lightmap1;

    /// <summary>
    /// Lightmap 2 texture ID
    /// </summary>
    Guid Lightmap2;
};

/// <summary>
/// Describes object reference to the lightmap
/// </summary>
struct LightmapEntry
{
    /// <summary>
    /// Index of the lightmap
    /// </summary>
    int32 TextureIndex;

    /// <summary>
    /// Lightmap UVs area that entry occupies
    /// </summary>
    Rectangle UVsArea;

    /// <summary>
    /// Init
    /// </summary>
    LightmapEntry()
        : TextureIndex(INVALID_INDEX)
        , UVsArea(Rectangle::Empty)
    {
    }
};

/// <summary>
/// Describes lightmap generation options
/// </summary>
API_STRUCT() struct LightmapSettings : ISerializable
{
DECLARE_SCRIPTING_TYPE_MINIMAL(LightmapSettings);

    /// <summary>
    /// Lightmap atlas sizes (in pixels).
    /// </summary>
    API_ENUM() enum class AtlasSizes
    {
        /// <summary>
        /// 64x64
        /// </summary>
        _64 = 64,

        /// <summary>
        /// 128x128
        /// </summary>
        _128 = 128,

        /// <summary>
        /// 256x256
        /// </summary>
        _256 = 256,

        /// <summary>
        /// 512x512
        /// </summary>
        _512 = 512,

        /// <summary>
        /// 1024x1024
        /// </summary>
        _1024 = 1024,

        /// <summary>
        /// 2048x2048
        /// </summary>
        _2048 = 2048,

        /// <summary>
        /// 4096x4096
        /// </summary>
        _4096 = 4096,
    };

    /// <summary>
    /// Controls how much all lights will contribute to indirect lighting.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(0), Limit(0, 100.0f, 0.1f)")
    float IndirectLightingIntensity = 1.0f;

    /// <summary>
    /// Global scale for objects in the lightmap to increase quality
    /// </summary>
    API_FIELD(Attributes="EditorOrder(10), Limit(0, 100.0f, 0.1f)")
    float GlobalObjectsScale = 1.0f;

    /// <summary>
    /// Amount of pixel space between charts in lightmap atlas
    /// </summary>
    API_FIELD(Attributes="EditorOrder(20), Limit(0, 16, 0.1f)")
    int32 ChartsPadding = 3;

    /// <summary>
    /// Single lightmap atlas size (width and height in pixels)
    /// </summary>
    API_FIELD(Attributes="EditorOrder(30)")
    AtlasSizes AtlasSize = AtlasSizes::_1024;

    /// <summary>
    /// Amount of indirect light GI bounce passes
    /// </summary>
    API_FIELD(Attributes="EditorOrder(40), Limit(1, 16, 0.1f)")
    int32 BounceCount = 1;

    /// <summary>
    /// Enable/disable compressing lightmap textures (3 textures per lightmap with RGBA data in HDR)
    /// </summary>
    API_FIELD(Attributes="EditorOrder(45)")
    bool CompressLightmaps = true;

    /// <summary>
    /// Enable/disable rendering static light for geometry with missing or empty material slots
    /// </summary>
    API_FIELD(Attributes="EditorOrder(50)")
    bool UseGeometryWithNoMaterials = true;

    /// <summary>
    /// GI quality (range  [0;100])
    /// </summary>
    API_FIELD(Attributes="EditorOrder(60), Limit(0, 100, 0.1f)")
    int32 Quality = 10;

public:

    // [ISerializable]
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;
};
