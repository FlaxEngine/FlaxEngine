// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#if COMPILE_WITH_MATERIAL_GRAPH

#include "Engine/Graphics/Materials/MaterialInfo.h"
#include "Engine/Graphics/Materials/MaterialParams.h"
#include "Engine/Content/Utilities/AssetsContainer.h"
#include "MaterialLayer.h"
#include "Types.h"

/// <summary>
/// Material node input boxes (each enum item value maps to box ID).
/// </summary>
enum class MaterialGraphBoxes
{
    /// <summary>
    /// The layer input.
    /// </summary>
    Layer = 0,

    /// <summary>
    /// The color input.
    /// </summary>
    Color = 1,

    /// <summary>
    /// The mask input.
    /// </summary>
    Mask = 2,

    /// <summary>
    /// The emissive input.
    /// </summary>
    Emissive = 3,

    /// <summary>
    /// The metalness input.
    /// </summary>
    Metalness = 4,

    /// <summary>
    /// The specular input.
    /// </summary>
    Specular = 5,

    /// <summary>
    /// The roughness input.
    /// </summary>
    Roughness = 6,

    /// <summary>
    /// The ambient occlusion input.
    /// </summary>
    AmbientOcclusion = 7,

    /// <summary>
    /// The normal input.
    /// </summary>
    Normal = 8,

    /// <summary>
    /// The opacity input.
    /// </summary>
    Opacity = 9,

    /// <summary>
    /// The refraction input.
    /// </summary>
    Refraction = 10,

    /// <summary>
    /// The position offset input.
    /// </summary>
    PositionOffset = 11,

    /// <summary>
    /// The tessellation multiplier input.
    /// </summary>
    TessellationMultiplier = 12,

    /// <summary>
    /// The world displacement input.
    /// </summary>
    WorldDisplacement = 13,

    /// <summary>
    /// The subsurface color input.
    /// </summary>
    SubsurfaceColor = 14,

    /// <summary>
    /// The amount of input boxes.
    /// </summary>
    MAX
};

/// <summary>
/// Material shaders generator from graphs.
/// </summary>
class MaterialGenerator : public ShaderGenerator
{
public:

    struct MaterialGraphBoxesMapping
    {
        byte ID;
        const Char* SubName;
        MaterialTreeType TreeType;
        MaterialValue DefaultValue;
    };

private:

    Array<MaterialLayer*> _layers;
    Array<MaterialGraphBox*, FixedAllocation<16>> _vsToPsInterpolants;
    MaterialTreeType _treeType;
    MaterialLayer* _treeLayer = nullptr;
    String _treeLayerVarName;
    MaterialValue _ddx, _ddy, _cameraVector;

public:

    MaterialGenerator();
    ~MaterialGenerator();

public:

    /// <summary>
    /// Gets material root layer
    /// </summary>
    /// <returns>Base layer</returns>
    MaterialLayer* GetRootLayer() const;

    /// <summary>
    /// Add new layer to the generator data (will be deleted after usage)
    /// </summary>
    /// <param name="layer">Layer to add</param>
    void AddLayer(MaterialLayer* layer);

    /// <summary>
    /// Gets layer that has given ID, if not loaded tries to load it
    /// </summary>
    /// <param name="id">Layer ID</param>
    /// <param name="caller">Calling node</param>
    /// <returns>Material layer or null if cannot do that</returns>
    MaterialLayer* GetLayer(const Guid& id, Node* caller);

    /// <summary>
    /// Generate material source code (first layer should be the base one)
    /// </summary>
    /// <param name="source">Output source code</param>
    /// <param name="materialInfo">Material info structure (will contain output data)</param>
    /// <param name="parametersData">Output material parameters data</param>
    /// <returns>True if cannot generate code, otherwise false</returns>
    bool Generate(WriteStream& source, MaterialInfo& materialInfo, BytesContainer& parametersData);

private:

    void clearCache();
    void createGradients(Node* caller);
    Value getCameraVector(Node* caller);

    void eatMaterialGraphBox(String& layerVarName, MaterialGraphBox* nodeBox, MaterialGraphBoxes box);
    void eatMaterialGraphBox(MaterialLayer* layer, MaterialGraphBoxes box);
    void eatMaterialGraphBoxWithDefault(MaterialLayer* layer, MaterialGraphBoxes box);

    void ProcessGroupLayers(Box* box, Node* node, Value& value);
    void ProcessGroupMaterial(Box* box, Node* node, Value& value);
    void ProcessGroupMath(Box* box, Node* node, Value& value);
    void ProcessGroupParameters(Box* box, Node* node, Value& value);
    void ProcessGroupTextures(Box* box, Node* node, Value& value);
    void ProcessGroupTools(Box* box, Node* node, Value& value);
    void ProcessGroupParticles(Box* box, Node* node, Value& value);
    void ProcessGroupFunction(Box* box, Node* node, Value& value);

    void writeBlending(MaterialGraphBoxes box, Value& result, const Value& bottom, const Value& top, const Value& alpha);

    using ShaderGenerator::findParam;
    SerializedMaterialParam* findParam(const Guid& id, MaterialLayer* layer);
    MaterialGraphParameter* findGraphParam(const Guid& id);

    MaterialValue* sampleTextureRaw(Node* caller, Value& value, Box* box, SerializedMaterialParam* texture);
    void sampleTexture(Node* caller, Value& value, Box* box, SerializedMaterialParam* texture);
    void sampleSceneDepth(Node* caller, Value& value, Box* box);
    void linearizeSceneDepth(Node* caller, const Value& depth, Value& value);

    // This must match ParticleAttribute::ValueTypes enum in Particles Module
    enum class ParticleAttributeValueTypes
    {
        Float,
        Vector2,
        Vector3,
        Vector4,
        Int,
        Uint,
    };

    MaterialValue AccessParticleAttribute(Node* caller, const StringView& name, ParticleAttributeValueTypes valueType, const Char* index = nullptr);
    void prepareLayer(MaterialLayer* layer, bool allowVisibleParams);

public:

    static MaterialValue getUVs;
    static MaterialValue getTime;
    static MaterialValue getNormal;
    static MaterialValue getNormalZero;
    static MaterialValue getVertexColor;
    static MaterialGraphBoxesMapping MaterialGraphBoxesMappings[];

    static const MaterialGraphBoxesMapping& GetMaterialRootNodeBox(MaterialGraphBoxes box);

    static byte getStartSrvRegister(MaterialLayer* baseLayer);
};

#endif
