// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#if COMPILE_WITH_MODEL_TOOL

#include "Engine/Core/Config.h"
#include "Engine/Content/Assets/ModelBase.h"
#include "Engine/Physics/CollisionData.h"
#if USE_EDITOR
#include "Engine/Core/ISerializable.h"
#include "Engine/Graphics/Models/ModelData.h"
#include "Engine/Graphics/Models/SkeletonData.h"
#include "Engine/Animations/AnimationData.h"
#include "Engine/Content/Assets/MaterialBase.h"

class JsonWriter;

/// <summary>
/// The model file import data types (used as flags).
/// </summary>
enum class ImportDataTypes : int32
{
    None = 0,

    /// <summary>
    /// Imports meshes (and LODs).
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
/// Models data importing and processing utility.
/// </summary>
API_CLASS(Namespace="FlaxEngine.Tools", Static) class FLAXENGINE_API ModelTool
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(ModelTool);

    // Optional: inputModel or modelData
    // Optional: outputSDF or null, outputStream or null
    static bool GenerateModelSDF(class Model* inputModel, class ModelData* modelData, float resolutionScale, int32 lodIndex, ModelBase::SDFData* outputSDF, class MemoryWriteStream* outputStream, const StringView& assetName, float backfacesThreshold = 0.6f, bool useGPU = true);

#if USE_EDITOR

public:
    /// <summary>
    /// Declares the imported data type.
    /// </summary>
    API_ENUM(Attributes="HideInEditor") enum class ModelType : int32
    {
        // The model asset.
        Model = 0,
        // The skinned model asset.
        SkinnedModel = 1,
        // The animation asset.
        Animation = 2,
        // The prefab scene.
        Prefab = 3,
    };

    /// <summary>
    /// Declares the imported animation clip duration.
    /// </summary>
    API_ENUM(Attributes="HideInEditor") enum class AnimationDuration : int32
    {
        // The imported duration.
        Imported = 0,
        // The custom duration specified via keyframes range.
        Custom = 1,
    };

    /// <summary>
    /// Declares the imported animation Root Motion modes.
    /// </summary>
    API_ENUM(Attributes="HideInEditor") enum class RootMotionMode
    {
        // Root Motion feature is disabled.
        None = 0,
        // Motion is extracted from the root node (or node specified by name).
        ExtractNode = 1,
        // Motion is extracted from the center of mass movement (estimated based on the skeleton pose animation).
        ExtractCenterOfMass = 2,
    };

    /// <summary>
    /// Model import options.
    /// </summary>
    API_STRUCT(Attributes="HideInEditor") struct FLAXENGINE_API Options : public ISerializable
    {
        DECLARE_SCRIPTING_TYPE_MINIMAL(Options);

        // Type of the imported asset.
        API_FIELD(Attributes="EditorOrder(0)")
        ModelType Type = ModelType::Model;

    public: // Geometry

        // Enable model normal vectors re-calculating.
        API_FIELD(Attributes="EditorOrder(20), EditorDisplay(\"Geometry\"), VisibleIf(nameof(ShowGeometry))")
        bool CalculateNormals = false;
        // Specifies the maximum angle (in degrees) that may be between two face normals at the same vertex position before they are smoothed together. The default value is 175.
        API_FIELD(Attributes="EditorOrder(30), EditorDisplay(\"Geometry\"), VisibleIf(nameof(ShowSmoothingNormalsAngle)), Limit(0, 175, 0.1f)")
        float SmoothingNormalsAngle = 175.0f;
        // If checked, the imported normal vectors of the mesh will be flipped (scaled by -1).
        API_FIELD(Attributes="EditorOrder(35), EditorDisplay(\"Geometry\"), VisibleIf(nameof(ShowGeometry))")
        bool FlipNormals = false;
        // Enable model tangent vectors re-calculating.
        API_FIELD(Attributes="EditorOrder(40), EditorDisplay(\"Geometry\"), VisibleIf(nameof(ShowGeometry))")
        bool CalculateTangents = false;
        // Specifies the maximum angle (in degrees) that may be between two vertex tangents before their tangents and bi-tangents are smoothed. The default value is 45.
        API_FIELD(Attributes="EditorOrder(45), EditorDisplay(\"Geometry\"), VisibleIf(nameof(ShowSmoothingTangentsAngle)), Limit(0, 45, 0.1f)")
        float SmoothingTangentsAngle = 45.0f;
        // If checked, the winding order of the vertices will be reversed.
        API_FIELD(Attributes="EditorOrder(47), EditorDisplay(\"Geometry\"), VisibleIf(nameof(ShowGeometry))")
        bool ReverseWindingOrder = false;
        // Enable/disable meshes geometry optimization.
        API_FIELD(Attributes="EditorOrder(50), EditorDisplay(\"Geometry\"), VisibleIf(nameof(ShowGeometry))")
        bool OptimizeMeshes = true;
        // Enable/disable geometry merge for meshes with the same materials. Index buffer will be reordered to improve performance and other modifications will be applied. However, importing time will be increased.
        API_FIELD(Attributes="EditorOrder(60), EditorDisplay(\"Geometry\"), VisibleIf(nameof(ShowGeometry))")
        bool MergeMeshes = true;
        // Enable/disable importing meshes Level of Details.
        API_FIELD(Attributes="EditorOrder(70), EditorDisplay(\"Geometry\", \"Import LODs\"), VisibleIf(nameof(ShowGeometry))")
        bool ImportLODs = true;
        // Enable/disable importing vertex colors (channel 0 only).
        API_FIELD(Attributes="EditorOrder(80), EditorDisplay(\"Geometry\"), VisibleIf(nameof(ShowModel))")
        bool ImportVertexColors = true;
        // Enable/disable importing blend shapes (morph targets).
        API_FIELD(Attributes="EditorOrder(85), EditorDisplay(\"Geometry\"), VisibleIf(nameof(ShowSkinnedModel))")
        bool ImportBlendShapes = false;
        // Enable skeleton bones offset matrices re-calculating.
        API_FIELD(Attributes="EditorOrder(86), EditorDisplay(\"Geometry\"), VisibleIf(nameof(ShowSkinnedModel))")
        bool CalculateBoneOffsetMatrices = false;
        // The lightmap UVs source.
        API_FIELD(Attributes="EditorOrder(90), EditorDisplay(\"Geometry\", \"Lightmap UVs Source\"), VisibleIf(nameof(ShowModel))")
        ModelLightmapUVsSource LightmapUVsSource = ModelLightmapUVsSource::Disable;
        // If specified, all meshes that name starts with this prefix in the name will be imported as a separate collision data asset (excluded used for rendering).
        API_FIELD(Attributes="EditorOrder(100), EditorDisplay(\"Geometry\"), VisibleIf(nameof(ShowGeometry))")
        String CollisionMeshesPrefix = TEXT("");
        // The type of collision that should be generated if the mesh has a collision prefix specified.
        API_FIELD(Attributes = "EditorOrder(105), EditorDisplay(\"Geometry\"), VisibleIf(nameof(ShowGeometry))")
        CollisionDataType CollisionType = CollisionDataType::ConvexMesh;

    public: // Transform

        // Custom uniform import scale.
        API_FIELD(Attributes="EditorOrder(500), EditorDisplay(\"Transform\")")
        float Scale = 1.0f;
        // Custom import geometry rotation.
        API_FIELD(Attributes="EditorOrder(510), EditorDisplay(\"Transform\")")
        Quaternion Rotation = Quaternion::Identity;
        // Custom import geometry offset.
        API_FIELD(Attributes="EditorOrder(520), EditorDisplay(\"Transform\")")
        Float3 Translation = Float3::Zero;
        // If checked, the imported geometry will be shifted to its local transform origin.
        API_FIELD(Attributes="EditorOrder(530), EditorDisplay(\"Transform\")")
        bool UseLocalOrigin = false;
        // If checked, the imported geometry will be shifted to the center of mass.
        API_FIELD(Attributes="EditorOrder(540), EditorDisplay(\"Transform\")")
        bool CenterGeometry = false;

    public: // Animation

        // Imported animation duration mode. Can use the original value or be overriden by settings.
        API_FIELD(Attributes="EditorOrder(1000), EditorDisplay(\"Animation\"), VisibleIf(nameof(ShowAnimation))")
        AnimationDuration Duration = AnimationDuration::Imported;
        // Imported animation first/last frame index. Used only if Duration mode is set to Custom.
        API_FIELD(Attributes="EditorOrder(1010), EditorDisplay(\"Animation\"), VisibleIf(nameof(ShowFramesRange)), Limit(0)")
        Float2 FramesRange = Float2::Zero;
        // The imported animation default frame rate. Can specify the default frames per second amount for imported animations. If the value is 0 then the original animation frame rate will be used.
        API_FIELD(Attributes="EditorOrder(1020), EditorDisplay(\"Animation\"), VisibleIf(nameof(ShowAnimation)), Limit(0, 1000, 0.01f)")
        float DefaultFrameRate = 0.0f;
        // The imported animation sampling rate. If value is 0 then the original animation speed will be used.
        API_FIELD(Attributes="EditorOrder(1030), EditorDisplay(\"Animation\"), VisibleIf(nameof(ShowAnimation)), Limit(0, 1000, 0.01f)")
        float SamplingRate = 0.0f;
        // The imported animation will have tracks with no keyframes or unspecified data removed.
        API_FIELD(Attributes="EditorOrder(1040), EditorDisplay(\"Animation\"), VisibleIf(nameof(ShowAnimation))")
        bool SkipEmptyCurves = true;
        // The imported animation channels will be optimized to remove redundant keyframes.
        API_FIELD(Attributes="EditorOrder(1050), EditorDisplay(\"Animation\"), VisibleIf(nameof(ShowAnimation))")
        bool OptimizeKeyframes = true;
        // If checked, the importer will import scale animation tracks (otherwise scale animation will be ignored).
        API_FIELD(Attributes="EditorOrder(1055), EditorDisplay(\"Animation\"), VisibleIf(nameof(ShowAnimation))")
        bool ImportScaleTracks = false;
        // Enables root motion extraction support from this animation.
        API_FIELD(Attributes="EditorOrder(1060), EditorDisplay(\"Animation\"), VisibleIf(nameof(ShowAnimation))")
        RootMotionMode RootMotion = RootMotionMode::None;
        // Adjusts root motion applying flags. Can customize how root node animation can affect target actor movement (eg. apply both position and rotation changes).
        API_FIELD(Attributes="EditorOrder(1060), EditorDisplay(\"Animation\"), VisibleIf(nameof(ShowRootMotion))")
        AnimationRootMotionFlags RootMotionFlags = AnimationRootMotionFlags::RootPositionXZ;
        // The custom node name to be used as a root motion source. If not specified the actual root node will be used.
        API_FIELD(Attributes="EditorOrder(1070), EditorDisplay(\"Animation\"), VisibleIf(nameof(ShowRootMotion))")
        String RootNodeName = TEXT("");

    public: // Level Of Detail

        // If checked, the importer will generate a sequence of LODs based on the base LOD index.
        API_FIELD(Attributes="EditorOrder(1100), EditorDisplay(\"Level Of Detail\", \"Generate LODs\"), VisibleIf(nameof(ShowGeometry))")
        bool GenerateLODs = false;
        // The index of the LOD from the source model data to use as a reference for following LODs generation.
        API_FIELD(Attributes="EditorOrder(1110), EditorDisplay(\"Level Of Detail\", \"Base LOD\"), VisibleIf(nameof(ShowGeometry)), Limit(0, 5)")
        int32 BaseLOD = 0;
        // The amount of LODs to include in the model (all remaining ones starting from Base LOD will be generated).
        API_FIELD(Attributes="EditorOrder(1120), EditorDisplay(\"Level Of Detail\", \"LOD Count\"), VisibleIf(nameof(ShowGeometry)), Limit(1, 6)")
        int32 LODCount = 4;
        // The target amount of triangles for the generated LOD (based on the higher LOD). Normalized to range 0-1. For instance 0.4 cuts the triangle count to 40%.
        API_FIELD(Attributes="EditorOrder(1130), EditorDisplay(\"Level Of Detail\"), VisibleIf(nameof(ShowGeometry)), Limit(0, 1, 0.001f)")
        float TriangleReduction = 0.5f;
        // Whether to do a sloppy mesh optimization. This is faster but does not follow the topology of the original mesh.
        API_FIELD(Attributes="EditorOrder(1140), EditorDisplay(\"Level Of Detail\"), VisibleIf(nameof(ShowGeometry))")
        bool SloppyOptimization = false;
        // Only used if Sloppy is false. Target error is an approximate measure of the deviation from the original mesh using distance normalized to [0..1] range (e.g. 1e-2f means that simplifier will try to maintain the error to be below 1% of the mesh extents).
        API_FIELD(Attributes="EditorOrder(1150), EditorDisplay(\"Level Of Detail\"), VisibleIf(nameof(SloppyOptimization), true), VisibleIf(nameof(ShowGeometry)), Limit(0.01f, 1, 0.001f)")
        float LODTargetError = 0.05f;

    public: // Materials

        // If checked, the importer will create materials for model meshes as specified in the file.
        API_FIELD(Attributes="EditorOrder(400), EditorDisplay(\"Materials\"), VisibleIf(nameof(ShowGeometry))")
        bool ImportMaterials = true;
        // If checked, the importer will create the model's materials as instances of a base material.
        API_FIELD(Attributes = "EditorOrder(401), EditorDisplay(\"Materials\"), VisibleIf(nameof(ImportMaterials)), VisibleIf(nameof(ShowGeometry))")
        bool ImportMaterialsAsInstances = false;
        // The material used as the base material that will be instanced as the imported model's material.
        API_FIELD(Attributes = "EditorOrder(402), EditorDisplay(\"Materials\"), VisibleIf(nameof(ImportMaterialsAsInstances)), VisibleIf(nameof(ShowGeometry))")
        AssetReference<MaterialBase> InstanceToImportAs;
        // If checked, the importer will import texture files used by the model and any embedded texture resources.
        API_FIELD(Attributes="EditorOrder(410), EditorDisplay(\"Materials\"), VisibleIf(nameof(ShowGeometry))")
        bool ImportTextures = true;
        // If checked, the importer will try to keep the model's current overridden material slots, instead of importing materials from the source file.
        API_FIELD(Attributes="EditorOrder(420), EditorDisplay(\"Materials\", \"Keep Overridden Materials\"), VisibleIf(nameof(ShowGeometry))")
        bool RestoreMaterialsOnReimport = true;
        // If checked, the importer will not reimport any material from this model which already exist in the sub-asset folder.
        API_FIELD(Attributes = "EditorOrder(421), EditorDisplay(\"Materials\", \"Skip Existing Materials\"), VisibleIf(nameof(ShowGeometry))")
        bool SkipExistingMaterialsOnReimport = true;

    public: // SDF

        // If checked, enables generation of Signed Distance Field (SDF).
        API_FIELD(Attributes="EditorOrder(1500), EditorDisplay(\"SDF\"), VisibleIf(nameof(ShowModel))")
        bool GenerateSDF = false;
        // Resolution scale for generated Signed Distance Field (SDF) texture. Higher values improve accuracy but increase memory usage and reduce performance.
        API_FIELD(Attributes="EditorOrder(1510), EditorDisplay(\"SDF\"), VisibleIf(nameof(ShowModel)), Limit(0.0001f, 100.0f)")
        float SDFResolution = 1.0f;

    public: // Splitting

        // If checked, the imported mesh/animations are split into separate assets. Used if ObjectIndex is set to -1.
        API_FIELD(Attributes="EditorOrder(2000), EditorDisplay(\"Splitting\"), VisibleIf(nameof(ShowSplitting))")
        bool SplitObjects = false;
        // The zero-based index for the mesh/animation clip to import. If the source file has more than one mesh/animation it can be used to pick a desired object. Default -1 imports all objects.
        API_FIELD(Attributes="EditorOrder(2010), EditorDisplay(\"Splitting\"), VisibleIf(nameof(ShowSplitting))")
        int32 ObjectIndex = -1;

    public: // Other

        // If specified, the specified folder will be used as sub-directory name for automatically imported sub assets such as textures and materials. Set to whitespace (single space) to import to the same directory.
        API_FIELD(Attributes="EditorOrder(3030), EditorDisplay(\"Other\")")
        String SubAssetFolder = TEXT("");

    public: // Internals

        // Internal flags for objects to import.
        ImportDataTypes ImportTypes = ImportDataTypes::None;

        struct CachedData
        {
            ModelData* Data = nullptr;
            void* MeshesByName = nullptr;
        };
        // Cached model data - used when performing nested importing (eg. via objects splitting). Allows to read and process source file only once and use those results for creation of multiple assets (permutation via ObjectIndex).
        CachedData* Cached = nullptr;

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
    static bool ImportData(const String& path, ModelData& data, Options& options, String& errorMsg);

    /// <summary>
    /// Imports the model.
    /// </summary>
    /// <param name="path">The file path.</param>
    /// <param name="data">The output data.</param>
    /// <param name="options">The import options.</param>
    /// <param name="errorMsg">The error message container.</param>
    /// <param name="autoImportOutput">The output folder for the additional imported data - optional. Used to auto-import textures and material assets.</param>
    /// <returns>True if fails, otherwise false.</returns>
    static bool ImportModel(const String& path, ModelData& data, Options& options, String& errorMsg, const String& autoImportOutput = String::Empty);

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
    static void CalculateBoneOffsetMatrix(const Array<SkeletonNode>& nodes, Matrix& offsetMatrix, int32 nodeIndex);
#if USE_ASSIMP
    static bool ImportDataAssimp(const String& path, ModelData& data, Options& options, String& errorMsg);
#endif
#if USE_AUTODESK_FBX_SDK
	static bool ImportDataAutodeskFbxSdk(const String& path, ModelData& data, Options& options, String& errorMsg);
#endif
#if USE_OPEN_FBX
    static bool ImportDataOpenFBX(const String& path, ModelData& data, Options& options, String& errorMsg);
#endif
#endif
};

#endif
