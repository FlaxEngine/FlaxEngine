// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#pragma once

#if COMPILE_WITH_MODEL_TOOL

#include "Engine/Core/Config.h"
#include "Engine/Content/Assets/ModelBase.h"
#if USE_EDITOR
#include "Engine/Core/ISerializable.h"
#include "Engine/Graphics/Models/ModelData.h"
#include "Engine/Graphics/Models/SkeletonData.h"
#include "Engine/Animations/AnimationData.h"

class JsonWriter;

/// <summary>
/// The model file import data types (used as flags).
/// </summary>
enum class ImportDataTypes : int32
{
    None = 0,

    /// <summary>
    /// Imports materials and meshes.
    /// </summary>
    Geometry = 1 << 0,

    /// <summary>
    /// Imports the skeleton bones hierarchy.
    /// </summary>
    Skeleton = 1 << 1,

    /// <summary>
    /// Imports the animations.
    /// </summary>
    Animations = 1 << 2,

    /// <summary>
    /// Imports the scene nodes hierarchy.
    /// </summary>
    Nodes = 1 << 3,

    /// <summary>
    /// Imports the materials.
    /// </summary>
    Materials = 1 << 4,

    /// <summary>
    /// Imports the textures.
    /// </summary>
    Textures = 1 << 5,
};

DECLARE_ENUM_OPERATORS(ImportDataTypes);

/// <summary>
/// Imported model data container. Represents unified model source file data (meshes, animations, skeleton, materials).
/// </summary>
class ImportedModelData
{
public:

    struct LOD
    {
        Array<MeshData*> Meshes;

        BoundingBox GetBox() const;
    };

    struct Node
    {
        /// <summary>
        /// The parent node index. The root node uses value -1.
        /// </summary>
        int32 ParentIndex;

        /// <summary>
        /// The local transformation of the node, relative to the parent node.
        /// </summary>
        Transform LocalTransform;

        /// <summary>
        /// The name of this node.
        /// </summary>
        String Name;
    };

public:

    /// <summary>
    /// The import data types types.
    /// </summary>
    ImportDataTypes Types;

    /// <summary>
    /// The textures slots.
    /// </summary>
    Array<TextureEntry> Textures;

    /// <summary>
    /// The material slots.
    /// </summary>
    Array<MaterialSlotEntry> Materials;

    /// <summary>
    /// The level of details data.
    /// </summary>
    Array<LOD> LODs;

    /// <summary>
    /// The skeleton data.
    /// </summary>
    SkeletonData Skeleton;

    /// <summary>
    /// The scene nodes.
    /// </summary>
    Array<Node> Nodes;

    /// <summary>
    /// The node animations.
    /// </summary>
    AnimationData Animation;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="ImportedModelData"/> class.
    /// </summary>
    /// <param name="types">The types.</param>
    ImportedModelData(ImportDataTypes types)
    {
        Types = types;
    }

    /// <summary>
    /// Finalizes an instance of the <see cref="ImportedModelData"/> class.
    /// </summary>
    ~ImportedModelData()
    {
        // Ensure to cleanup data
        for (int32 i = 0; i < LODs.Count(); i++)
            LODs[i].Meshes.ClearDelete();
    }
};

#endif

struct ModelSDFHeader
{
    Float3 LocalToUVWMul;
    float WorldUnitsPerVoxel;
    Float3 LocalToUVWAdd;
    float MaxDistance;
    Float3 LocalBoundsMin;
    int32 MipLevels;
    Float3 LocalBoundsMax;
    int32 Width;
    int32 Height;
    int32 Depth;
    PixelFormat Format;
    float ResolutionScale;
    int32 LOD;

    ModelSDFHeader() = default;
    ModelSDFHeader(const ModelBase::SDFData& sdf, const struct GPUTextureDescription& desc);
};

struct ModelSDFMip
{
    int32 MipIndex;
    uint32 RowPitch;
    uint32 SlicePitch;

    ModelSDFMip() = default;
    ModelSDFMip(int32 mipIndex, uint32 rowPitch, uint32 slicePitch);
    ModelSDFMip(int32 mipIndex, const TextureMipData& mip);
};

/// <summary>
/// Models data  importing and processing utility.
/// </summary>
class FLAXENGINE_API ModelTool
{
public:

    // Optional: inputModel or modelData
    // Optional: outputSDF or null, outputStream or null
    static bool GenerateModelSDF(class Model* inputModel, class ModelData* modelData, float resolutionScale, int32 lodIndex, ModelBase::SDFData* outputSDF, class MemoryWriteStream* outputStream, const StringView& assetName, float backfacesThreshold = 0.6f);

#if USE_EDITOR
public:
    /// <summary>
    /// Declares the imported data type.
    /// </summary>
    DECLARE_ENUM_EX_3(ModelType, int32, 0, Model, SkinnedModel, Animation);

    /// <summary>
    /// Declares the imported animation clip duration.
    /// </summary>
    DECLARE_ENUM_EX_2(AnimationDuration, int32, 0, Imported, Custom);

    /// <summary>
    /// Importing model options
    /// </summary>
    struct Options : public ISerializable
    {
        ModelType Type = ModelType::Model;

        // Geometry
        bool CalculateNormals = false;
        float SmoothingNormalsAngle = 175.0f;
        bool FlipNormals = false;
        float SmoothingTangentsAngle = 45.0f;
        bool CalculateTangents = false;
        bool OptimizeMeshes = true;
        bool MergeMeshes = true;
        bool ImportLODs = true;
        bool ImportVertexColors = true;
        bool ImportBlendShapes = false;
        ModelLightmapUVsSource LightmapUVsSource = ModelLightmapUVsSource::Disable;
        String CollisionMeshesPrefix;

        // Transform
        float Scale = 1.0f;
        Quaternion Rotation = Quaternion::Identity;
        Float3 Translation = Float3::Zero;
        bool CenterGeometry = false;

        // Animation
        AnimationDuration Duration = AnimationDuration::Imported;
        Float2 FramesRange = Float2::Zero;
        float DefaultFrameRate = 0.0f;
        float SamplingRate = 0.0f;
        bool SkipEmptyCurves = true;
        bool OptimizeKeyframes = true;
        bool ImportScaleTracks = false;
        bool EnableRootMotion = false;
        String RootNodeName;

        // Level Of Detail
        bool GenerateLODs = false;
        int32 BaseLOD = 0;
        int32 LODCount = 4;
        float TriangleReduction = 0.5f;

        // Materials
        bool ImportMaterials = true;
        bool ImportTextures = true;
        bool RestoreMaterialsOnReimport = true;

        // SDF
        bool GenerateSDF = false;
        float SDFResolution = 1.0f;

        // Splitting
        bool SplitObjects = false;
        int32 ObjectIndex = -1;

        // Runtime data for objects splitting during import (used internally)
        void* SplitContext = nullptr;
        Function<bool(Options& splitOptions, const String& objectName)> OnSplitImport;

    public:

        // [ISerializable]
        void Serialize(SerializeStream& stream, const void* otherObj) override;
        void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;
    };

public:

    /// <summary>
    /// Imports the model source file data.
    /// </summary>
    /// <param name="path">The file path.</param>
    /// <param name="data">The output data.</param>
    /// <param name="options">The import options.</param>
    /// <param name="errorMsg">The error message container.</param>
    /// <returns>True if fails, otherwise false.</returns>
    static bool ImportData(const String& path, ImportedModelData& data, Options& options, String& errorMsg);

    /// <summary>
    /// Imports the model.
    /// </summary>
    /// <param name="path">The file path.</param>
    /// <param name="meshData">The output data.</param>
    /// <param name="options">The import options.</param>
    /// <param name="errorMsg">The error message container.</param>
    /// <param name="autoImportOutput">The output folder for the additional imported data - optional. Used to auto-import textures and material assets.</param>
    /// <returns>True if fails, otherwise false.</returns>
    static bool ImportModel(const String& path, ModelData& meshData, Options& options, String& errorMsg, const String& autoImportOutput = String::Empty);

public:

    static int32 DetectLodIndex(const String& nodeName);
    static bool FindTexture(const String& sourcePath, const String& file, String& path);

    /// <summary>
    /// Gets the local transformations to go from rootIndex to index.
    /// </summary>
    /// <param name="nodes">The nodes containing the local transformations.</param>
    /// <param name="rootIndex">The root index.</param>
    /// <param name="index">The current index.</param>
    /// <returns>The transformation at this index.</returns>
    template<typename Node>
    static Transform CombineTransformsFromNodeIndices(Array<Node>& nodes, int32 rootIndex, int32 index)
    {
        if (index == -1 || index == rootIndex)
            return Transform::Identity;

        auto result = nodes[index].LocalTransform;
        if (index != rootIndex)
        {
            const auto parentTransform = CombineTransformsFromNodeIndices(nodes, rootIndex, nodes[index].ParentIndex);
            result = parentTransform.LocalToWorld(result);
        }

        return result;
    }

private:

#if USE_ASSIMP
    static bool ImportDataAssimp(const char* path, ImportedModelData& data, Options& options, String& errorMsg);
#endif
#if USE_AUTODESK_FBX_SDK
	static bool ImportDataAutodeskFbxSdk(const char* path, ImportedModelData& data, Options& options, String& errorMsg);
#endif
#if USE_OPEN_FBX
    static bool ImportDataOpenFBX(const char* path, ImportedModelData& data, Options& options, String& errorMsg);
#endif
#endif
};

#endif
