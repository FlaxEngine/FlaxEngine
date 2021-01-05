// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#if COMPILE_WITH_MODEL_TOOL && USE_ASSIMP

#include "ModelTool.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Matrix.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Tools/TextureTool/TextureTool.h"

// Import Assimp library
// Source: https://github.com/assimp/assimp
#define ASSIMP_BUILD_NO_EXPORT
#include <ThirdParty/assimp/Importer.hpp>
#include <ThirdParty/assimp/types.h>
#include <ThirdParty/assimp/config.h>
#include <ThirdParty/assimp/scene.h>
#include <ThirdParty/assimp/version.h>
#include <ThirdParty/assimp/postprocess.h>
#include <ThirdParty/assimp/LogStream.hpp>
#include <ThirdParty/assimp/DefaultLogger.hpp>
#include <ThirdParty/assimp/Logger.hpp>
using namespace Assimp;

class AssimpLogStream : public LogStream
{
public:

    AssimpLogStream()
    {
        DefaultLogger::create("");
        DefaultLogger::get()->attachStream(this);
    }

    ~AssimpLogStream()
    {
        DefaultLogger::get()->detatchStream(this);
        DefaultLogger::kill();
    }

public:

    void write(const char* message) override
    {
        String s(message);
        s.Replace('\n', ' ');
        LOG(Info, "[Assimp]: {0}", s);
    }
};

Vector2 ToVector2(const aiVector2D& v)
{
    return Vector2(v.x, v.y);
}

Vector2 ToVector2(const aiVector3D& v)
{
    return Vector2(v.x, v.y);
}

Vector3 ToVector3(const aiVector3D& v)
{
    return Vector3(v.x, v.y, v.z);
}

Color ToColor(const aiColor3D& v)
{
    return Color(v.r, v.g, v.b, 1.0f);
}

Color ToColor(const aiColor4D& v)
{
    return Color(v.r, v.g, v.b, v.a);
}

Quaternion ToQuaternion(const aiQuaternion& v)
{
    return Quaternion(v.x, v.y, v.z, v.w);
}

Matrix ToMatrix(const aiMatrix4x4& mat)
{
    return Matrix(mat.a1, mat.b1, mat.c1, mat.d1,
                  mat.a2, mat.b2, mat.c2, mat.d2,
                  mat.a3, mat.b3, mat.c3, mat.d3,
                  mat.a4, mat.b4, mat.c4, mat.d4);
}

struct AssimpNode
{
    /// <summary>
    /// The parent index. The root node uses value -1.
    /// </summary>
    int32 ParentIndex;

    /// <summary>
    /// The local transformation of the bone, relative to parent bone.
    /// </summary>
    Transform LocalTransform;

    /// <summary>
    /// The name of this bone.
    /// </summary>
    String Name;

    /// <summary>
    /// The LOD index of the data in this node (used to separate meshes across different level of details).
    /// </summary>
    int32 LodIndex;
};

struct AssimpBone
{
    /// <summary>
    /// The index of the related node.
    /// </summary>
    int32 NodeIndex;

    /// <summary>
    /// The parent bone index. The root bone uses value -1.
    /// </summary>
    int32 ParentBoneIndex;

    /// <summary>
    /// The name of this bone.
    /// </summary>
    String Name;

    /// <summary>
    /// The matrix that transforms from mesh space to bone space in bind pose.
    /// </summary>
    Matrix OffsetMatrix;

    bool operator<(const AssimpBone& other) const
    {
        return NodeIndex < other.NodeIndex;
    }
};

struct AssimpImporterData
{
    ImportedModelData& Model;
    const String Path;
    const aiScene* Scene;
    const ModelTool::Options& Options;

    Array<AssimpNode> Nodes;
    Array<AssimpBone> Bones;
    Dictionary<int32, Array<int32>> MeshIndexToNodeIndex;

    AssimpImporterData(const char* path, ImportedModelData& model, const ModelTool::Options& options, const aiScene* scene)
        : Model(model)
        , Path(path)
        , Scene(scene)
        , Options(options)
        , Nodes(static_cast<int32>(scene->mNumMeshes * 4.0f))
        , MeshIndexToNodeIndex(static_cast<int32>(scene->mNumMeshes * 8.0f))
    {
    }

    int32 FindNode(const String& name, StringSearchCase caseSensitivity = StringSearchCase::CaseSensitive)
    {
        for (int32 i = 0; i < Nodes.Count(); i++)
        {
            if (Nodes[i].Name.Compare(name, caseSensitivity) == 0)
                return i;
        }
        return -1;
    }

    int32 FindBone(const String& name, StringSearchCase caseSensitivity = StringSearchCase::CaseSensitive)
    {
        for (int32 i = 0; i < Bones.Count(); i++)
        {
            if (Bones[i].Name.Compare(name, caseSensitivity) == 0)
                return i;
        }
        return -1;
    }

    int32 FindBone(const int32 nodeIndex)
    {
        for (int32 i = 0; i < Bones.Count(); i++)
        {
            if (Bones[i].NodeIndex == nodeIndex)
                return i;
        }
        return -1;
    }
};

void ProcessNodes(AssimpImporterData& data, aiNode* aNode, int32 parentIndex)
{
    const int32 nodeIndex = data.Nodes.Count();

    // Assign the index of the node to the index of the mesh
    for (unsigned i = 0; i < aNode->mNumMeshes; i++)
    {
        int meshIndex = aNode->mMeshes[i];
        data.MeshIndexToNodeIndex[meshIndex].Add(nodeIndex);
    }

    // Create node
    AssimpNode node;
    node.ParentIndex = parentIndex;
    node.Name = aNode->mName.C_Str();

    // Pick node LOD index
    if (parentIndex == -1 || !data.Options.ImportLODs)
    {
        node.LodIndex = 0;
    }
    else
    {
        node.LodIndex = data.Nodes[parentIndex].LodIndex;
        if (node.LodIndex == 0)
        {
            node.LodIndex = ModelTool::DetectLodIndex(node.Name);
        }
        ASSERT(Math::IsInRange(node.LodIndex, 0, MODEL_MAX_LODS - 1));
    }

    Matrix transform = ToMatrix(aNode->mTransformation);
    transform.Decompose(node.LocalTransform);
    data.Nodes.Add(node);

    // Process the children
    for (unsigned i = 0; i < aNode->mNumChildren; i++)
    {
        ProcessNodes(data, aNode->mChildren[i], nodeIndex);
    }
}

bool ProcessMesh(AssimpImporterData& data, const aiMesh* aMesh, MeshData& mesh, String& errorMsg)
{
    // Properties
    mesh.Name = aMesh->mName.C_Str();
    mesh.MaterialSlotIndex = aMesh->mMaterialIndex;

    // Vertex positions
    mesh.Positions.Set((const Vector3*)aMesh->mVertices, aMesh->mNumVertices);

    // Texture coordinates
    if (aMesh->mTextureCoords[0])
    {
        mesh.UVs.Resize(aMesh->mNumVertices, false);
        aiVector3D* a = aMesh->mTextureCoords[0];
        for (uint32 v = 0; v < aMesh->mNumVertices; v++)
        {
            mesh.UVs[v] = *(Vector2*)a;
            a++;
        }
    }

    // Normals
    if (aMesh->mNormals)
    {
        mesh.Normals.Set((const Vector3*)aMesh->mNormals, aMesh->mNumVertices);
    }

    // Tangents
    if (aMesh->mTangents)
    {
        mesh.Tangents.Set((const Vector3*)aMesh->mTangents, aMesh->mNumVertices);
    }

    // Indices
    const int32 indicesCount = aMesh->mNumFaces * 3;
    mesh.Indices.Resize(indicesCount, false);
    for (unsigned faceIndex = 0, i = 0; faceIndex < aMesh->mNumFaces; faceIndex++)
    {
        const auto face = &aMesh->mFaces[faceIndex];
        if (face->mNumIndices != 3)
        {
            errorMsg = TEXT("All faces in a mesh must be trangles!");
            return true;
        }

        mesh.Indices[i++] = face->mIndices[0];
        mesh.Indices[i++] = face->mIndices[1];
        mesh.Indices[i++] = face->mIndices[2];
    }

    // Lightmap UVs
    if (data.Options.LightmapUVsSource == ModelLightmapUVsSource::Disable)
    {
        // No lightmap UVs
    }
    else if (data.Options.LightmapUVsSource == ModelLightmapUVsSource::Generate)
    {
        // Generate lightmap UVs
        if (mesh.GenerateLightmapUVs())
        {
            LOG(Error, "Failed to generate lightmap uvs");
        }
    }
    else
    {
        // Select input channel index
        int32 inputChannelIndex;
        switch (data.Options.LightmapUVsSource)
        {
        case ModelLightmapUVsSource::Channel0:
            inputChannelIndex = 0;
            break;
        case ModelLightmapUVsSource::Channel1:
            inputChannelIndex = 1;
            break;
        case ModelLightmapUVsSource::Channel2:
            inputChannelIndex = 2;
            break;
        case ModelLightmapUVsSource::Channel3:
            inputChannelIndex = 3;
            break;
        default:
            inputChannelIndex = INVALID_INDEX;
            break;
        }

        // Check if has that channel texcoords
        if (inputChannelIndex >= 0 && inputChannelIndex < AI_MAX_NUMBER_OF_TEXTURECOORDS && aMesh->mTextureCoords[inputChannelIndex])
        {
            mesh.LightmapUVs.Resize(aMesh->mNumVertices, false);
            aiVector3D* a = aMesh->mTextureCoords[inputChannelIndex];
            for (uint32 v = 0; v < aMesh->mNumVertices; v++)
            {
                mesh.LightmapUVs[v] = *(Vector2*)a;
                a++;
            }
        }
        else
        {
            LOG(Warning, "Cannot import model lightmap uvs. Missing texcoords channel {0}.", inputChannelIndex);
        }
    }

    // Vertex Colors
    if (data.Options.ImportVertexColors && aMesh->mColors[0])
    {
        mesh.Colors.Resize(aMesh->mNumVertices, false);
        aiColor4D* a = aMesh->mColors[0];
        for (uint32 v = 0; v < aMesh->mNumVertices; v++)
        {
            mesh.Colors[v] = *(Color*)a;
            a++;
        }
    }

    // Blend Indices and Blend Weights
    if (aMesh->mNumBones > 0 && aMesh->mBones && data.Model.Types & ImportDataTypes::Skeleton)
    {
        const int32 vertexCount = mesh.Positions.Count();
        mesh.BlendIndices.Resize(vertexCount);
        mesh.BlendWeights.Resize(vertexCount);
        mesh.BlendIndices.SetAll(Int4::Zero);
        mesh.BlendWeights.SetAll(Vector4::Zero);

        // Build skinning clusters and fill controls points data structure
        for (unsigned boneId = 0; boneId < aMesh->mNumBones; boneId++)
        {
            const auto aBone = aMesh->mBones[boneId];

            // Find the node where the bone is mapped - based on the name
            const String boneName(aBone->mName.C_Str());
            const int32 nodeIndex = data.FindNode(boneName);
            if (nodeIndex == -1)
            {
                LOG(Warning, "Invalid mesh bone linkage. Mesh: {0}, bone: {1}. Skipping...", mesh.Name, boneName);
                continue;
            }

            // Create bone if missing
            int32 boneIndex = data.FindBone(boneName);
            if (boneIndex == -1)
            {
                // Find the parent bone
                int32 parentBoneIndex = -1;
                for (int32 i = nodeIndex; i != -1; i = data.Nodes[i].ParentIndex)
                {
                    parentBoneIndex = data.FindBone(i);
                    if (parentBoneIndex != -1)
                        break;
                }

                // Add bone
                boneIndex = data.Bones.Count();
                data.Bones.EnsureCapacity(Math::Max(128, boneIndex + 16));
                data.Bones.Resize(boneIndex + 1);
                auto& bone = data.Bones[boneIndex];

                // Setup bone
                bone.Name = boneName;
                bone.NodeIndex = nodeIndex;
                bone.ParentBoneIndex = parentBoneIndex;
                bone.OffsetMatrix = ToMatrix(aBone->mOffsetMatrix);
            }

            // Apply the bone influences
            for (unsigned vtxWeightId = 0; vtxWeightId < aBone->mNumWeights; vtxWeightId++)
            {
                const auto vtxWeight = aBone->mWeights[vtxWeightId];

                if (vtxWeight.mWeight <= 0 || vtxWeight.mVertexId >= (unsigned)vertexCount)
                    continue;

                auto& indices = mesh.BlendIndices[vtxWeight.mVertexId];
                auto& weights = mesh.BlendWeights[vtxWeight.mVertexId];

                for (int32 k = 0; k < 4; k++)
                {
                    if (vtxWeight.mWeight >= weights.Raw[k])
                    {
                        for (int32 l = 2; l >= k; l--)
                        {
                            indices.Raw[l + 1] = indices.Raw[l];
                            weights.Raw[l + 1] = weights.Raw[l];
                        }

                        indices.Raw[k] = boneIndex;
                        weights.Raw[k] = vtxWeight.mWeight;
                        break;
                    }
                }
            }
        }

        mesh.NormalizeBlendWeights();
    }

    // Blend Shapes
    if (aMesh->mNumAnimMeshes > 0 && data.Model.Types & ImportDataTypes::Skeleton && data.Options.ImportBlendShapes)
    {
        mesh.BlendShapes.EnsureCapacity(aMesh->mNumAnimMeshes);
        for (unsigned int animMeshIndex = 0; animMeshIndex < aMesh->mNumAnimMeshes; animMeshIndex++)
        {
            const aiAnimMesh* aAnimMesh = aMesh->mAnimMeshes[animMeshIndex];

            BlendShape& blendShapeData = mesh.BlendShapes.AddOne();
            blendShapeData.Name = aAnimMesh->mName.C_Str();
            if (blendShapeData.Name.IsEmpty())
                blendShapeData.Name = mesh.Name + TEXT("_blend_shape_") + StringUtils::ToString(animMeshIndex);
            blendShapeData.Weight = aAnimMesh->mWeight;
            blendShapeData.Vertices.Resize(aAnimMesh->mNumVertices);
            for (int32 i = 0; i < blendShapeData.Vertices.Count(); i++)
                blendShapeData.Vertices[i].VertexIndex = i;

            const aiVector3D* shapeVertices = aAnimMesh->mVertices;
            if (shapeVertices)
            {
                for (int32 i = 0; i < blendShapeData.Vertices.Count(); i++)
                    blendShapeData.Vertices[i].PositionDelta = ToVector3(shapeVertices[i]) - mesh.Positions[i];
            }
            else
            {
                for (int32 i = 0; i < blendShapeData.Vertices.Count(); i++)
                    blendShapeData.Vertices[i].PositionDelta = Vector3::Zero;
            }

            const aiVector3D* shapeNormals = aAnimMesh->mNormals;
            if (shapeNormals)
            {
                for (int32 i = 0; i < blendShapeData.Vertices.Count(); i++)
                    blendShapeData.Vertices[i].NormalDelta = ToVector3(shapeNormals[i]) - mesh.Normals[i];
            }
            else
            {
                for (int32 i = 0; i < blendShapeData.Vertices.Count(); i++)
                    blendShapeData.Vertices[i].NormalDelta = Vector3::Zero;
            }
        }
    }

    return false;
}

bool ImportTexture(AssimpImporterData& data, aiString& aFilename, int32& textureIndex, TextureEntry::TypeHint type)
{
    // Find texture file path
    const String filename = String(aFilename.C_Str()).TrimTrailing();
    String path;
    if (ModelTool::FindTexture(data.Path, filename, path))
        return true;

    // Check if already used
    textureIndex = 0;
    while (textureIndex < data.Model.Textures.Count())
    {
        if (data.Model.Textures[textureIndex].FilePath == path)
            return true;
        textureIndex++;
    }

    // Import texture
    auto& texture = data.Model.Textures.AddOne();
    texture.FilePath = path;
    texture.Type = type;
    texture.AssetID = Guid::Empty;
    return true;
}

bool ImportMaterialTexture(AssimpImporterData& data, const aiMaterial* aMaterial, aiTextureType aTextureType, int32& textureIndex, TextureEntry::TypeHint type)
{
    aiString aFilename;
    return aMaterial->GetTexture(aTextureType, 0, &aFilename, nullptr, nullptr, nullptr, nullptr) == AI_SUCCESS &&
            ImportTexture(data, aFilename, textureIndex, type);
}

bool ImportMaterials(AssimpImporterData& data, String& errorMsg)
{
    const uint32 materialsCount = (uint32)data.Scene->mNumMaterials;
    data.Model.Materials.Resize(materialsCount, false);
    for (uint32 i = 0; i < materialsCount; i++)
    {
        auto& materialSlot = data.Model.Materials[i];
        const aiMaterial* aMaterial = data.Scene->mMaterials[i];

        aiString aName;
        if (aMaterial->Get(AI_MATKEY_NAME, aName) == AI_SUCCESS)
            materialSlot.Name = String(aName.C_Str()).TrimTrailing();
        materialSlot.AssetID = Guid::Empty;
        aiColor3D aColor;
        if (aMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, aColor) == AI_SUCCESS)
            materialSlot.Diffuse.Color = ToColor(aColor);
        bool aBoolean;
        if (aMaterial->Get(AI_MATKEY_TWOSIDED, aBoolean) == AI_SUCCESS)
            materialSlot.TwoSided = aBoolean;
        bool aFloat;
        if (aMaterial->Get(AI_MATKEY_OPACITY, aFloat) == AI_SUCCESS)
            materialSlot.Opacity.Value = aFloat;

        if (data.Model.Types & ImportDataTypes::Textures)
        {
            ImportMaterialTexture(data, aMaterial, aiTextureType_DIFFUSE, materialSlot.Diffuse.TextureIndex, TextureEntry::TypeHint::ColorRGB);
            ImportMaterialTexture(data, aMaterial, aiTextureType_EMISSIVE, materialSlot.Emissive.TextureIndex, TextureEntry::TypeHint::ColorRGB);
            ImportMaterialTexture(data, aMaterial, aiTextureType_NORMALS, materialSlot.Normals.TextureIndex, TextureEntry::TypeHint::Normals);
            ImportMaterialTexture(data, aMaterial, aiTextureType_OPACITY, materialSlot.Opacity.TextureIndex, TextureEntry::TypeHint::ColorRGBA);

            if (materialSlot.Diffuse.TextureIndex != -1)
            {
                // Detect using alpha mask in diffuse texture
                materialSlot.Diffuse.HasAlphaMask = TextureTool::HasAlpha(data.Model.Textures[materialSlot.Diffuse.TextureIndex].FilePath);
                if (materialSlot.Diffuse.HasAlphaMask)
                    data.Model.Textures[materialSlot.Diffuse.TextureIndex].Type = TextureEntry::TypeHint::ColorRGBA;
            }
        }
    }

    return false;
}

bool ImportMeshes(AssimpImporterData& data, String& errorMsg)
{
    for (unsigned i = 0; i < data.Scene->mNumMeshes; i++)
    {
        const auto aMesh = data.Scene->mMeshes[i];

        // Skip invalid meshes
        if (aMesh->mPrimitiveTypes != aiPrimitiveType_TRIANGLE || aMesh->mNumVertices == 0 || aMesh->mNumFaces == 0 || aMesh->mFaces[0].mNumIndices != 3)
            continue;

        // Skip unused meshes
        if (!data.MeshIndexToNodeIndex.ContainsKey(i))
            continue;

        // Import mesh data
        MeshData* meshData = New<MeshData>();
        if (ProcessMesh(data, aMesh, *meshData, errorMsg))
            return true;

        auto& nodesWithMesh = data.MeshIndexToNodeIndex[i];
        for (int32 j = 0; j < nodesWithMesh.Count(); j++)
        {
            const auto nodeIndex = nodesWithMesh[j];
            auto& node = data.Nodes[nodeIndex];
            const int32 lodIndex = node.LodIndex;

            // The first mesh instance uses meshData directly while others have to clone it
            if (j != 0)
            {
                meshData = New<MeshData>(*meshData);
            }

            // Link mesh
            meshData->NodeIndex = nodeIndex;
            if (data.Model.LODs.Count() <= lodIndex)
                data.Model.LODs.Resize(lodIndex + 1);
            data.Model.LODs[lodIndex].Meshes.Add(meshData);
        }
    }

    return false;
}

void ImportCurve(aiVectorKey* keys, uint32 keysCount, LinearCurve<Vector3>& curve)
{
    if (keys == nullptr || keysCount == 0)
        return;

    const auto keyframes = curve.Resize(keysCount);

    for (uint32 i = 0; i < keysCount; i++)
    {
        auto& aKey = keys[i];
        auto& key = keyframes[i];

        key.Time = (float)aKey.mTime;
        key.Value = ToVector3(aKey.mValue);
    }
}

void ImportCurve(aiQuatKey* keys, uint32 keysCount, LinearCurve<Quaternion>& curve)
{
    if (keys == nullptr || keysCount == 0)
        return;

    const auto keyframes = curve.Resize(keysCount);

    for (uint32 i = 0; i < keysCount; i++)
    {
        auto& aKey = keys[i];
        auto& key = keyframes[i];

        key.Time = (float)aKey.mTime;
        key.Value = ToQuaternion(aKey.mValue);
    }
}

static bool AssimpInited = false;

bool ModelTool::ImportDataAssimp(const char* path, ImportedModelData& data, const Options& options, String& errorMsg)
{
    // Prepare
    if (!AssimpInited)
    {
        AssimpInited = true;

        // Log Assimp version
        LOG(Info, "Assimp {0}.{1}.{2}", aiGetVersionMajor(), aiGetVersionMinor(), aiGetVersionRevision());
    }
    Importer importer;
    AssimpLogStream assimpLogStream;
    bool importMeshes = (data.Types & ImportDataTypes::Geometry) != 0;
    bool importAnimations = (data.Types & ImportDataTypes::Animations) != 0;

    // Setup import flags
    unsigned int flags =
            aiProcess_JoinIdenticalVertices |
            aiProcess_LimitBoneWeights |
            aiProcess_Triangulate |
            aiProcess_GenUVCoords |
            aiProcess_FindDegenerates |
            aiProcess_FindInvalidData |
            //aiProcess_ValidateDataStructure |
            aiProcess_ConvertToLeftHanded;
    if (importMeshes)
    {
        if (options.CalculateNormals)
            flags |= aiProcess_FixInfacingNormals | aiProcess_GenSmoothNormals;
        if (options.CalculateTangents)
            flags |= aiProcess_CalcTangentSpace;
        if (options.OptimizeMeshes)
            flags |= aiProcess_OptimizeMeshes | aiProcess_SplitLargeMeshes | aiProcess_ImproveCacheLocality;
        if (options.MergeMeshes)
            flags |= aiProcess_RemoveRedundantMaterials;
    }

    // Setup import options
    importer.SetPropertyFloat(AI_CONFIG_PP_GSN_MAX_SMOOTHING_ANGLE, options.SmoothingNormalsAngle);
    importer.SetPropertyFloat(AI_CONFIG_PP_CT_MAX_SMOOTHING_ANGLE, options.SmoothingTangentsAngle);
    //importer.SetPropertyInteger(AI_CONFIG_PP_SLM_TRIANGLE_LIMIT, MAX_uint16);
    importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_READ_CAMERAS, false);
    importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_READ_LIGHTS, false);
    importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_READ_TEXTURES, false);
    importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_READ_ANIMATIONS, importAnimations);
    //importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false); // TODO: optimize pivots when https://github.com/assimp/assimp/issues/1068 gets fixed
    importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_OPTIMIZE_EMPTY_ANIMATION_CURVES, true);

    // Import file
    const auto scene = importer.ReadFile(path, flags);
    if (scene == nullptr)
    {
        LOG_STR(Warning, String(importer.GetErrorString()));
        LOG_STR(Warning, String(path));
        LOG_STR(Warning, StringUtils::ToString(flags));
        errorMsg = importer.GetErrorString();
        return true;
    }

    // Process imported scene nodes
    AssimpImporterData assimpData(path, data, options, scene);
    ProcessNodes(assimpData, scene->mRootNode, -1);

    // Import materials
    if (data.Types & ImportDataTypes::Materials)
    {
        if (ImportMaterials(assimpData, errorMsg))
        {
            LOG(Warning, "Failed to import materials.");
            return true;
        }
    }

    // Import geometry
    if (data.Types & ImportDataTypes::Geometry)
    {
        if (ImportMeshes(assimpData, errorMsg))
        {
            LOG(Warning, "Failed to import meshes.");
            return true;
        }
    }

    // Import skeleton
    if (data.Types & ImportDataTypes::Skeleton)
    {
        data.Skeleton.Nodes.Resize(assimpData.Nodes.Count(), false);
        for (int32 i = 0; i < assimpData.Nodes.Count(); i++)
        {
            auto& node = data.Skeleton.Nodes[i];
            auto& aNode = assimpData.Nodes[i];

            node.Name = aNode.Name;
            node.ParentIndex = aNode.ParentIndex;
            node.LocalTransform = aNode.LocalTransform;
        }

        data.Skeleton.Bones.Resize(assimpData.Bones.Count(), false);
        for (int32 i = 0; i < assimpData.Bones.Count(); i++)
        {
            auto& bone = data.Skeleton.Bones[i];
            auto& aBone = assimpData.Bones[i];

            const auto boneNodeIndex = aBone.NodeIndex;
            const auto parentBoneNodeIndex = aBone.ParentBoneIndex == -1 ? -1 : assimpData.Bones[aBone.ParentBoneIndex].NodeIndex;

            bone.ParentIndex = aBone.ParentBoneIndex;
            bone.NodeIndex = aBone.NodeIndex;
            bone.LocalTransform = CombineTransformsFromNodeIndices(assimpData.Nodes, parentBoneNodeIndex, boneNodeIndex);
            bone.OffsetMatrix = aBone.OffsetMatrix;
        }
    }

    // Import animations
    if (data.Types & ImportDataTypes::Animations)
    {
        if (scene->HasAnimations())
        {
            const auto animIndex = Math::Clamp<int32>(options.AnimationIndex, 0, scene->mNumAnimations - 1);
            const auto animations = scene->mAnimations[animIndex];
            data.Animation.Channels.Resize(animations->mNumChannels, false);
            data.Animation.Duration = animations->mDuration;
            data.Animation.FramesPerSecond = animations->mTicksPerSecond != 0.0 ? animations->mTicksPerSecond : 25.0;

            for (unsigned i = 0; i < animations->mNumChannels; i++)
            {
                const auto aAnim = animations->mChannels[i];
                auto& anim = data.Animation.Channels[i];

                anim.NodeName = aAnim->mNodeName.C_Str();

                ImportCurve(aAnim->mPositionKeys, aAnim->mNumPositionKeys, anim.Position);
                ImportCurve(aAnim->mRotationKeys, aAnim->mNumRotationKeys, anim.Rotation);
                ImportCurve(aAnim->mScalingKeys, aAnim->mNumScalingKeys, anim.Scale);
            }
        }
        else
        {
            LOG(Warning, "Loaded scene has no animations");
        }
    }

    // Import nodes
    if (data.Types & ImportDataTypes::Nodes)
    {
        data.Nodes.Resize(assimpData.Nodes.Count());
        for (int32 i = 0; i < assimpData.Nodes.Count(); i++)
        {
            auto& node = data.Nodes[i];
            auto& aNode = assimpData.Nodes[i];

            node.Name = aNode.Name;
            node.ParentIndex = aNode.ParentIndex;
            node.LocalTransform = aNode.LocalTransform;
        }
    }

    return false;
}

#endif
