// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if COMPILE_WITH_MATERIAL_GRAPH

#include "Types.h"
#include "Engine/Graphics/Materials/MaterialInfo.h"

/// <summary>
/// We use them to map layer's params ID.
/// Used in layered materials to change param ID for each layer (two or more layers may have the same params ID from the same base material)
/// </summary>
struct LayerParamMapping
{
    Guid SrcId;
    Guid DstId;
};

/// <summary>
/// Single material layer
/// </summary>
class MaterialLayer
{
public:

    struct LayerUsage
    {
        String VarName;
        void* Hint;

        LayerUsage()
        {
            Hint = nullptr;
        }
    };

public:

    /// <summary>
    /// Layer ID
    /// </summary>
    Guid ID;

    /// <summary>
    /// Graph data
    /// </summary>
    MaterialGraph Graph;

    /// <summary>
    /// Root node
    /// </summary>
    MaterialGraph::Node* Root;

    /// <summary>
    /// Material structure variable name (different for every layer sampling with different UVs, default UVs are a first index)
    /// </summary>
    LayerUsage Usage[4];

    /// <summary>
    /// Layer features flags
    /// </summary>
    MaterialFeaturesFlags FeaturesFlags;

    /// <summary>
    /// Layer usage flags
    /// </summary>
    MaterialUsageFlags UsageFlags;

    /// <summary>
    /// Domain
    /// </summary>
    MaterialDomain Domain;

    /// <summary>
    /// Blending mode
    /// </summary>
    MaterialBlendMode BlendMode;

    /// <summary>
    /// The shading model.
    /// </summary>
    MaterialShadingModel ShadingModel;

    /// <summary>
    /// The opacity threshold value (masked materials pixels clipping).
    /// </summary>
    float MaskThreshold;

    /// <summary>
    /// The opacity threshold value (transparent materials shadow pass though clipping).
    /// </summary>
    float OpacityThreshold;

    /// <summary>
    /// Helper array with original layer parameters Ids mappings into new Ids
    /// Note: during sampling different materials layers we have to change their parameters Ids due to possible Ids collisions
    /// Collisions may occur in duplicated materials so we want to resolve them.
    /// </summary>
    Array<LayerParamMapping> ParamIdsMappings;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="MaterialLayer"/> class.
    /// </summary>
    /// <param name="id">The layer asset identifier.</param>
    MaterialLayer(const Guid& id);

public:

    /// <summary>
    /// Clear all cached values
    /// </summary>
    void ClearCache();

    /// <summary>
    /// Prepare layer for the material compilation process
    /// </summary>
    void Prepare();

    Guid GetMappedParamId(const Guid& id);

    String& GetVariableName(void* hint);

    bool HasAnyVariableName();

    void UpdateFeaturesFlags();

public:

    /// <summary>
    /// Create default empty layer
    /// </summary>
    /// <param name="id">Layer id</param>
    /// <returns>Layer</returns>
    static MaterialLayer* CreateDefault(const Guid& id);

    /// <summary>
    /// Load layer data
    /// </summary>
    /// <param name="id">Layer id</param>
    /// <param name="graphData">Stream with saved graph object</param>
    /// <param name="info">Material info structure</param>
    /// <param name="caller">Calling object name</param>
    /// <returns>Layer</returns>
    static MaterialLayer* Load(const Guid& id, ReadStream* graphData, const MaterialInfo& info, const String& caller);

private:

    void createRootNode();
};

#endif
