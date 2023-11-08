// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#if COMPILE_WITH_MODEL_TOOL && USE_OPEN_FBX

#include "ModelTool.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/DeleteMe.h"
#include "Engine/Core/Math/Mathd.h"
#include "Engine/Core/Math/Matrix.h"
#include "Engine/Core/Collections/Sorting.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Tools/TextureTool/TextureTool.h"
#include "Engine/Platform/File.h"

#define OPEN_FBX_CONVERT_SPACE 1

// Import OpenFBX library
// Source: https://github.com/nem0/OpenFBX
#include <ThirdParty/OpenFBX/ofbx.h>
#include <memory>

Float2 ToFloat2(const ofbx::Vec2& v)
{
    return Float2((float)v.x, (float)v.y);
}

Float2 ToFloat2(const ofbx::Vec3& v)
{
    return Float2((float)v.x, (float)v.y);
}

Float3 ToFloat3(const ofbx::Vec3& v)
{
    return Float3((float)v.x, (float)v.y, (float)v.z);
}

Color ToColor(const ofbx::Vec4& v)
{
    return Color((float)v.x, (float)v.y, (float)v.z, (float)v.w);
}

Color ToColor(const ofbx::Color& v)
{
    return Color(v.r, v.g, v.b, 1.0f);
}

Quaternion ToQuaternion(const ofbx::Quat& v)
{
    return Quaternion((float)v.x, (float)v.y, (float)v.z, (float)v.w);
}

Matrix ToMatrix(const ofbx::Matrix& mat)
{
    Matrix result;
    for (int32 i = 0; i < 16; i++)
        result.Raw[i] = (float)mat.m[i];
    return result;
}

struct FbxNode
{
    int32 ParentIndex;
    Transform LocalTransform;
    String Name;
    int32 LodIndex;
    const ofbx::Object* FbxObj;
};

struct FbxBone
{
    int32 NodeIndex;
    int32 ParentBoneIndex;
    const ofbx::Object* FbxObj;
    Matrix OffsetMatrix;

    bool operator<(const FbxBone& other) const
    {
        return NodeIndex < other.NodeIndex;
    }
};

struct OpenFbxImporterData
{
    const ofbx::IScene* Scene;
    std::unique_ptr<ofbx::IScene> ScenePtr;
    const String Path;
    const ModelTool::Options& Options;

    ofbx::GlobalSettings GlobalSettings;
#if OPEN_FBX_CONVERT_SPACE
    Quaternion RootConvertRotation = Quaternion::Identity;
    Float3 Up;
    Float3 Front;
    Float3 Right;
    bool ConvertRH;
#else
    static constexpr bool ConvertRH = false;
#endif
    float FrameRate;

    Array<FbxNode> Nodes;
    Array<FbxBone> Bones;
    Array<const ofbx::Material*> Materials;
    Array<MaterialSlotEntry> ImportedMaterials;

    OpenFbxImporterData(const char* path, const ModelTool::Options& options, ofbx::IScene* scene)
        : Scene(scene)
        , ScenePtr(scene)
        , Path(path)
        , Options(options)
        , GlobalSettings(*scene->getGlobalSettings())
#if OPEN_FBX_CONVERT_SPACE
        , ConvertRH(GlobalSettings.CoordAxis == ofbx::CoordSystem_RightHanded)
#endif
        , Nodes(static_cast<int32>(scene->getMeshCount() * 4.0f))
    {
        float frameRate = scene->getSceneFrameRate();
        if (frameRate <= 0 || GlobalSettings.TimeMode == ofbx::FrameRate_DEFAULT)
        {
            frameRate = Options.DefaultFrameRate;
            if (frameRate <= 0)
                frameRate = 30.0f;
        }
        FrameRate = frameRate;
#if OPEN_FBX_CONVERT_SPACE
        const float coordAxisSign = GlobalSettings.CoordAxis == ofbx::CoordSystem_LeftHanded ? -1.0f : +1.0f;
        switch (GlobalSettings.UpAxis)
        {
        case ofbx::UpVector_AxisX:
            Up = Float3((float)GlobalSettings.UpAxisSign, 0, 0);
            switch (GlobalSettings.FrontAxis)
            {
            case ofbx::FrontVector_ParityEven:
                // Up: X, Front: Y, Right: Z
                Front = Float3(0, (float)GlobalSettings.FrontAxisSign, 0);
                Right = Float3(0, 0, coordAxisSign);
                break;
            case ofbx::FrontVector_ParityOdd:
                // Up: X, Front: Z, Right: Y
                Front = Float3(0, 0, (float)GlobalSettings.FrontAxisSign);
                Right = Float3(0, coordAxisSign, 0);
                break;
            default: ;
            }
            break;
        case ofbx::UpVector_AxisY:
            Up = Float3(0, (float)GlobalSettings.UpAxisSign, 0);
            switch (GlobalSettings.FrontAxis)
            {
            case ofbx::FrontVector_ParityEven:
                // Up: Y, Front: X, Right: Z
                Front = Float3((float)GlobalSettings.FrontAxisSign, 0, 0);
                Right = Float3(0, 0, coordAxisSign);
                break;
            case ofbx::FrontVector_ParityOdd:
                // Up: Y, Front: Z, Right: X
                Front = Float3(0, 0, (float)GlobalSettings.FrontAxisSign);
                Right = Float3(coordAxisSign, 0, 0);
                break;
            default: ;
            }
            break;
        case ofbx::UpVector_AxisZ:
            Up = Float3(0, 0, (float)GlobalSettings.UpAxisSign);
            switch (GlobalSettings.FrontAxis)
            {
            case ofbx::FrontVector_ParityEven:
                // Up: Z, Front: X, Right: Y
                Front = Float3((float)GlobalSettings.FrontAxisSign, 0, 0);
                Right = Float3(0, coordAxisSign, 0);
                break;
            case ofbx::FrontVector_ParityOdd:
                // Up: Z, Front: Y, Right: X
                Front = Float3((float)GlobalSettings.FrontAxisSign, 0, 0);
                Right = Float3(coordAxisSign, 0, 0);
                break;
            default: ;
            }
            break;
        default: ;
        }
#endif
    }

    bool ImportMaterialTexture(ImportedModelData& result, const ofbx::Material* mat, ofbx::Texture::TextureType textureType, int32& textureIndex, TextureEntry::TypeHint type) const
    {
        const ofbx::Texture* tex = mat->getTexture(textureType);
        if (tex)
        {
            // Find texture file path
            ofbx::DataView aFilename = tex->getRelativeFileName();
            if (aFilename == "")
                aFilename = tex->getFileName();
            char filenameData[256];
            aFilename.toString(filenameData);
            const String filename(filenameData);
            String path;
            if (ModelTool::FindTexture(Path, filename, path))
                return true;

            // Check if already used
            textureIndex = 0;
            while (textureIndex < result.Textures.Count())
            {
                if (result.Textures[textureIndex].FilePath == path)
                    return true;
                textureIndex++;
            }

            // Import texture
            auto& texture = result.Textures.AddOne();
            texture.FilePath = path;
            texture.Type = type;
            texture.AssetID = Guid::Empty;
            return true;
        }
        return false;
    }

    int32 AddMaterial(ImportedModelData& result, const ofbx::Material* mat)
    {
        int32 index = Materials.Find(mat);
        if (index == -1)
        {
            index = Materials.Count();
            Materials.Add(mat);
            auto& material = ImportedMaterials.AddOne();
            material.AssetID = Guid::Empty;
            if (mat)
                material.Name = String(mat->name).TrimTrailing();

            if (mat && EnumHasAnyFlags(result.Types, ImportDataTypes::Materials))
            {
                material.Diffuse.Color = ToColor(mat->getDiffuseColor());

                if (EnumHasAnyFlags(result.Types, ImportDataTypes::Textures))
                {
                    ImportMaterialTexture(result, mat, ofbx::Texture::DIFFUSE, material.Diffuse.TextureIndex, TextureEntry::TypeHint::ColorRGB);
                    ImportMaterialTexture(result, mat, ofbx::Texture::EMISSIVE, material.Emissive.TextureIndex, TextureEntry::TypeHint::ColorRGB);
                    ImportMaterialTexture(result, mat, ofbx::Texture::NORMAL, material.Normals.TextureIndex, TextureEntry::TypeHint::Normals);

                    // FBX don't always store normal maps inside the object
                    if (material.Diffuse.TextureIndex != -1 && material.Normals.TextureIndex == -1)
                    {
                        // If missing, try to locate a normal map in the same path as the diffuse
                        const String srcFolder = String(StringUtils::GetDirectoryName(result.Textures[material.Diffuse.TextureIndex].FilePath));
                        const String srcName = StringUtils::GetFileNameWithoutExtension(result.Textures[material.Diffuse.TextureIndex].FilePath);
                        String srcSearch;
                        const int32 num = srcName.FindLast('_');
                        String srcSmallName = srcName;
                        if (num != -1)
                            srcSmallName = srcName.Substring(0, num);

                        bool isNormal = false;
                        for (int32 iExt = 0; iExt < 6; iExt++)
                        {
                            String sExit = TEXT(".dds");
                            if (iExt == 1)
                                sExit = TEXT(".png");
                            else if (iExt == 2)
                                sExit = TEXT(".jpg");
                            else if (iExt == 3)
                                sExit = TEXT(".jpeg");
                            else if (iExt == 4)
                                sExit = TEXT(".tif");
                            else if (iExt == 5)
                                sExit = TEXT(".tga");
                            for (int32 i = 0; i < 5; i++)
                            {
                                String sFind = TEXT("_normal" + sExit);
                                if (i == 1)
                                    sFind = TEXT("_n" + sExit);
                                else if (i == 2)
                                    sFind = TEXT("_nm" + sExit);
                                else if (i == 3)
                                    sFind = TEXT("_nmp" + sExit);
                                else if (i == 4)
                                    sFind = TEXT("_nor" + sExit);
                                srcSearch = srcFolder + TEXT("/") + srcSmallName + sFind;
                                if (FileSystem::FileExists(srcSearch))
                                {
                                    isNormal = true;
                                    break;
                                }
                            }
                            if (isNormal)
                                break;
                        }
                        if (isNormal)
                        {
                            auto& texture = result.Textures.AddOne();
                            texture.FilePath = srcSearch;
                            texture.Type = TextureEntry::TypeHint::Normals;
                            texture.AssetID = Guid::Empty;
                            material.Normals.TextureIndex = result.Textures.Count() - 1;
                        }
                    }

                    if (material.Diffuse.TextureIndex != -1)
                    {
                        // Detect using alpha mask in diffuse texture
                        material.Diffuse.HasAlphaMask = TextureTool::HasAlpha(result.Textures[material.Diffuse.TextureIndex].FilePath);
                        if (material.Diffuse.HasAlphaMask)
                            result.Textures[material.Diffuse.TextureIndex].Type = TextureEntry::TypeHint::ColorRGBA;
                    }
                }
            }
        }
        const auto& importedMaterial = ImportedMaterials[index];
        for (int32 i = 0; i < result.Materials.Count(); i++)
        {
            if (result.Materials[i].Name == importedMaterial.Name)
                return i;
        }
        result.Materials.Add(importedMaterial);
        return result.Materials.Count() - 1;
    }

    int32 FindNode(const ofbx::Object* link)
    {
        for (int32 i = 0; i < Nodes.Count(); i++)
        {
            if (Nodes[i].FbxObj == link)
                return i;
        }
        return -1;
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

    int32 FindBone(const int32 nodeIndex)
    {
        for (int32 i = 0; i < Bones.Count(); i++)
        {
            if (Bones[i].NodeIndex == nodeIndex)
                return i;
        }
        return -1;
    }

    int32 FindBone(const ofbx::Object* link)
    {
        for (int32 i = 0; i < Bones.Count(); i++)
        {
            if (Bones[i].FbxObj == link)
                return i;
        }
        return -1;
    }
};

void ProcessNodes(OpenFbxImporterData& data, const ofbx::Object* aNode, int32 parentIndex)
{
    const int32 nodeIndex = data.Nodes.Count();

    // Create node
    FbxNode node;
    node.ParentIndex = parentIndex;
    node.Name = aNode->name;
    node.FbxObj = aNode;

    // Pick node LOD index
    if (parentIndex == -1 || !data.Options.ImportLODs)
    {
        node.LodIndex = 0;
    }
    else
    {
        node.LodIndex = data.Nodes[parentIndex].LodIndex;
        if (node.LodIndex == 0)
            node.LodIndex = ModelTool::DetectLodIndex(node.Name);
        ASSERT(Math::IsInRange(node.LodIndex, 0, MODEL_MAX_LODS - 1));
    }

    auto transform = ToMatrix(aNode->evalLocal(aNode->getLocalTranslation(), aNode->getLocalRotation()));
#if OPEN_FBX_CONVERT_SPACE
    if (data.ConvertRH)
    {
        // Mirror all base vectors at the local Z axis
        transform.M31 = -transform.M31;
        transform.M32 = -transform.M32;
        transform.M33 = -transform.M33;
        transform.M34 = -transform.M34;

        // Now invert the Z axis again to keep the matrix determinant positive
        // The local meshes will be inverted accordingly so that the result should look just fine again
        transform.M13 = -transform.M13;
        transform.M23 = -transform.M23;
        transform.M33 = -transform.M33;
        transform.M43 = -transform.M43;
    }
#endif
    transform.Decompose(node.LocalTransform);
    data.Nodes.Add(node);

    // Process the children
    int i = 0;
    while (ofbx::Object* child = aNode->resolveObjectLink(i))
    {
        if (child->isNode())
            ProcessNodes(data, child, nodeIndex);
        i++;
    }
}

Matrix GetOffsetMatrix(OpenFbxImporterData& data, const ofbx::Mesh* mesh, const ofbx::Object* node)
{
#if 1
    auto* skin = mesh ? mesh->getGeometry()->getSkin() : nullptr;
    if (skin)
    {
        for (int i = 0, c = skin->getClusterCount(); i < c; i++)
        {
            const ofbx::Cluster* cluster = skin->getCluster(i);
            if (cluster->getLink() == node)
            {
                return ToMatrix(cluster->getTransformLinkMatrix());
            }
        }
    }
	//return Matrix::Identity;
    return ToMatrix(node->getGlobalTransform());
#else
    Matrix t = Matrix::Identity;
    const int32 boneIdx = data.FindBone(node);
    int32 idx = data.Bones[boneIdx].NodeIndex;
    do
    {
        t *= data.Nodes[idx].LocalTransform.GetWorld();
        idx = data.Nodes[idx].ParentIndex;
    } while (idx != -1);
    return t;
#endif
}

bool IsMeshInvalid(const ofbx::Mesh* aMesh)
{
    return aMesh->getGeometry()->getVertexCount() == 0;
}

bool ImportBones(OpenFbxImporterData& data, String& errorMsg)
{
    // Check all meshes
    const int meshCount = data.Scene->getMeshCount();
    for (int i = 0; i < meshCount; i++)
    {
        const auto aMesh = data.Scene->getMesh(i);
        const auto aGeometry = aMesh->getGeometry();
        const ofbx::Skin* skin = aGeometry->getSkin();
        if (skin == nullptr || IsMeshInvalid(aMesh))
            continue;

        for (int clusterIndex = 0, clusterCount = skin->getClusterCount(); clusterIndex < clusterCount; clusterIndex++)
        {
            const ofbx::Cluster* cluster = skin->getCluster(clusterIndex);
            if (cluster->getIndicesCount() == 0)
                continue;
            const auto link = cluster->getLink();
            ASSERT(link != nullptr);

            // Create bone if missing
            int32 boneIndex = data.FindBone(link);
            if (boneIndex == -1)
            {
                // Find the node where the bone is mapped
                int32 nodeIndex = data.FindNode(link);
                if (nodeIndex == -1)
                {
                    nodeIndex = data.FindNode(String(link->name), StringSearchCase::IgnoreCase);
                    if (nodeIndex == -1)
                    {
                        LOG(Warning, "Invalid mesh bone linkage. Mesh: {0}, bone: {1}. Skipping...", String(aMesh->name), String(link->name));
                        continue;
                    }
                }

                // Add bone
                boneIndex = data.Bones.Count();
                data.Bones.EnsureCapacity(256);
                data.Bones.Resize(boneIndex + 1);
                auto& bone = data.Bones[boneIndex];

                // Setup bone
                bone.NodeIndex = nodeIndex;
                bone.ParentBoneIndex = -1;
                bone.FbxObj = link;
                bone.OffsetMatrix = GetOffsetMatrix(data, aMesh, link) * Matrix::Scaling(data.GlobalSettings.UnitScaleFactor);
                bone.OffsetMatrix.Invert();

                // Mirror offset matrices (RH to LH)
                if (data.ConvertRH)
                {
                    auto& m = bone.OffsetMatrix;
                    m.M13 = -m.M13;
                    m.M23 = -m.M23;
                    m.M43 = -m.M43;
                    m.M31 = -m.M31;
                    m.M32 = -m.M32;
                    m.M34 = -m.M34;
                }

                // Convert bone matrix if scene uses root transform
                if (!data.RootConvertRotation.IsIdentity())
                {
                    Matrix m;
                    Matrix::RotationQuaternion(data.RootConvertRotation, m);
                    m.Invert();
                    bone.OffsetMatrix = m * bone.OffsetMatrix;
                }
            }
        }
    }

    return false;
}

bool ProcessMesh(ImportedModelData& result, OpenFbxImporterData& data, const ofbx::Mesh* aMesh, MeshData& mesh, String& errorMsg, int32 triangleStart, int32 triangleEnd)
{
    // Prepare
    const int32 firstVertexOffset = triangleStart * 3;
    const int32 lastVertexOffset = triangleEnd * 3;
    const ofbx::Geometry* aGeometry = aMesh->getGeometry();
    const int vertexCount = lastVertexOffset - firstVertexOffset + 3;
    ASSERT(firstVertexOffset + vertexCount <= aGeometry->getVertexCount());
    const ofbx::Vec3* vertices = aGeometry->getVertices();
    const ofbx::Vec3* normals = aGeometry->getNormals();
    const ofbx::Vec3* tangents = aGeometry->getTangents();
    const ofbx::Vec4* colors = aGeometry->getColors();
    const ofbx::Vec2* uvs = aGeometry->getUVs();
    const ofbx::Skin* skin = aGeometry->getSkin();
    const ofbx::BlendShape* blendShape = aGeometry->getBlendShape();

    // Properties
    mesh.Name = aMesh->name;
    const ofbx::Material* aMaterial = nullptr;
    if (aMesh->getMaterialCount() > 0)
    {
        if (aGeometry->getMaterials())
            aMaterial = aMesh->getMaterial(aGeometry->getMaterials()[triangleStart]);
        else
            aMaterial = aMesh->getMaterial(0);
    }
    mesh.MaterialSlotIndex = data.AddMaterial(result, aMaterial);

    // Vertex positions
    mesh.Positions.Resize(vertexCount, false);
    for (int i = 0; i < vertexCount; i++)
        mesh.Positions.Get()[i] = ToFloat3(vertices[i + firstVertexOffset]);

    // Indices (dummy index buffer)
    if (vertexCount % 3 != 0)
    {
        errorMsg = TEXT("Invalid vertex count. It must be multiple of 3.");
        return true;
    }
    mesh.Indices.Resize(vertexCount, false);
    for (int i = 0; i < vertexCount; i++)
        mesh.Indices.Get()[i] = i;

    // Texture coordinates
    if (uvs)
    {
        mesh.UVs.Resize(vertexCount, false);
        for (int i = 0; i < vertexCount; i++)
            mesh.UVs.Get()[i] = ToFloat2(uvs[i + firstVertexOffset]);
        if (data.ConvertRH)
        {
            for (int32 v = 0; v < vertexCount; v++)
                mesh.UVs[v].Y = 1.0f - mesh.UVs[v].Y;
        }
    }

    // Normals
    if (data.Options.CalculateNormals || !normals)
    {
        if (mesh.GenerateNormals(data.Options.SmoothingNormalsAngle))
        {
            errorMsg = TEXT("Failed to generate normals.");
            return true;
        }
    }
    else if (normals)
    {
        mesh.Normals.Resize(vertexCount, false);
        for (int i = 0; i < vertexCount; i++)
            mesh.Normals.Get()[i] = ToFloat3(normals[i + firstVertexOffset]);
        if (data.ConvertRH)
        {
            // Mirror normals along the Z axis
            for (int32 i = 0; i < vertexCount; i++)
                mesh.Normals.Get()[i].Z *= -1.0f;
        }
    }

    // Tangents
    if ((data.Options.CalculateTangents || !tangents) && mesh.UVs.HasItems())
    {
        // Generated after full mesh data conversion
    }
    else if (tangents)
    {
        mesh.Tangents.Resize(vertexCount, false);
        for (int i = 0; i < vertexCount; i++)
            mesh.Tangents.Get()[i] = ToFloat3(tangents[i + firstVertexOffset]);
        if (data.ConvertRH)
        {
            // Mirror tangents along the Z axis
            for (int32 i = 0; i < vertexCount; i++)
                mesh.Tangents.Get()[i].Z *= -1.0f;
        }
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
        const auto lightmapUVs = aGeometry->getUVs(inputChannelIndex);
        if (lightmapUVs)
        {
            mesh.LightmapUVs.Resize(vertexCount, false);
            for (int i = 0; i < vertexCount; i++)
                mesh.LightmapUVs.Get()[i] = ToFloat2(lightmapUVs[i + firstVertexOffset]);
            if (data.ConvertRH)
            {
                for (int32 v = 0; v < vertexCount; v++)
                    mesh.LightmapUVs[v].Y = 1.0f - mesh.LightmapUVs[v].Y;
            }
        }
        else
        {
            LOG(Warning, "Cannot import model lightmap uvs. Missing texcoords channel {0}.", inputChannelIndex);
        }
    }

    // Vertex Colors
    if (data.Options.ImportVertexColors && colors)
    {
        mesh.Colors.Resize(vertexCount, false);
        for (int i = 0; i < vertexCount; i++)
            mesh.Colors.Get()[i] = ToColor(colors[i + firstVertexOffset]);
    }

    // Blend Indices and Blend Weights
    if (skin && skin->getClusterCount() > 0 && EnumHasAnyFlags(result.Types, ImportDataTypes::Skeleton))
    {
        mesh.BlendIndices.Resize(vertexCount);
        mesh.BlendWeights.Resize(vertexCount);
        mesh.BlendIndices.SetAll(Int4::Zero);
        mesh.BlendWeights.SetAll(Float4::Zero);

        for (int clusterIndex = 0, clusterCount = skin->getClusterCount(); clusterIndex < clusterCount; clusterIndex++)
        {
            const ofbx::Cluster* cluster = skin->getCluster(clusterIndex);
            if (cluster->getIndicesCount() == 0)
                continue;
            const auto link = cluster->getLink();
            ASSERT(link != nullptr);

            // Get bone (should be created earlier)
            const int32 boneIndex = data.FindBone(link);
            if (boneIndex == -1)
            {
                // Find the node where the bone is mapped
                const int32 nodeIndex = data.FindNode(link);
                if (nodeIndex == -1)
                    continue;

                errorMsg = TEXT("Missing bone");
                return true;
            }

            // Apply the bone influences
            const int* clusterIndices = cluster->getIndices();
            const double* clusterWeights = cluster->getWeights();
            for (int j = 0; j < cluster->getIndicesCount(); j++)
            {
                int vtxIndex = clusterIndices[j] - firstVertexOffset;
                float vtxWeight = (float)clusterWeights[j];

                if (vtxWeight <= 0 || vtxIndex < 0 || vtxIndex >= vertexCount)
                    continue;

                auto& indices = mesh.BlendIndices[vtxIndex];
                auto& weights = mesh.BlendWeights[vtxIndex];

                for (int32 k = 0; k < 4; k++)
                {
                    if (vtxWeight >= weights.Raw[k])
                    {
                        for (int32 l = 2; l >= k; l--)
                        {
                            indices.Raw[l + 1] = indices.Raw[l];
                            weights.Raw[l + 1] = weights.Raw[l];
                        }

                        indices.Raw[k] = boneIndex;
                        weights.Raw[k] = vtxWeight;
                        break;
                    }
                }
            }
        }

        mesh.NormalizeBlendWeights();
    }

    // Blend Shapes
    if (blendShape && blendShape->getBlendShapeChannelCount() > 0 && EnumHasAnyFlags(result.Types, ImportDataTypes::Skeleton) && data.Options.ImportBlendShapes)
    {
        mesh.BlendShapes.EnsureCapacity(blendShape->getBlendShapeChannelCount());
        for (int32 channelIndex = 0; channelIndex < blendShape->getBlendShapeChannelCount(); channelIndex++)
        {
            const ofbx::BlendShapeChannel* channel = blendShape->getBlendShapeChannel(channelIndex);

            // Use last shape
            const int targetShapeCount = channel->getShapeCount();
            if (targetShapeCount == 0)
                continue;
            const ofbx::Shape* shape = channel->getShape(targetShapeCount - 1);

            if (shape->getVertexCount() != aGeometry->getVertexCount())
            {
                LOG(Error, "Blend shape '{0}' in mesh '{1}' has different amount of vertices ({2}) than mesh ({3})", String(shape->name), mesh.Name, shape->getVertexCount(), aGeometry->getVertexCount());
                continue;
            }

            BlendShape& blendShapeData = mesh.BlendShapes.AddOne();
            blendShapeData.Name = shape->name;
            blendShapeData.Weight = channel->getShapeCount() > 1 ? (float)(channel->getDeformPercent() / 100.0) : 1.0f;

            blendShapeData.Vertices.Resize(vertexCount);
            for (int32 i = 0; i < blendShapeData.Vertices.Count(); i++)
                blendShapeData.Vertices.Get()[i].VertexIndex = i;

            auto shapeVertices = shape->getVertices();
            for (int32 i = 0; i < blendShapeData.Vertices.Count(); i++)
            {
                auto delta = ToFloat3(shapeVertices[i + firstVertexOffset]) - mesh.Positions.Get()[i];
                blendShapeData.Vertices.Get()[i].PositionDelta = delta;
            }

            auto shapeNormals = shape->getNormals();
            for (int32 i = 0; i < blendShapeData.Vertices.Count(); i++)
            {
                /*auto delta = ToFloat3(shapeNormals[i + firstVertexOffset]) - mesh.Normals[i];
                auto length = delta.Length();
                if (length > ZeroTolerance)
                    delta /= length;*/
                auto delta = Float3::Zero; // TODO: blend shape normals deltas fix when importing from fbx
                blendShapeData.Vertices.Get()[i].NormalDelta = delta;
            }
        }
    }

    if (data.ConvertRH)
    {
        // Mirror positions along the Z axis
        for (int32 i = 0; i < vertexCount; i++)
            mesh.Positions[i].Z *= -1.0f;
        for (auto& blendShapeData : mesh.BlendShapes)
        {
            for (auto& v : blendShapeData.Vertices)
                v.PositionDelta.Z *= -1.0f;
        }
    }

    // Build solid index buffer (remove duplicated vertices)
    mesh.BuildIndexBuffer();

    if (data.ConvertRH)
    {
        // Invert the order
        for (int32 i = 0; i < mesh.Indices.Count(); i += 3)
            Swap(mesh.Indices[i], mesh.Indices[i + 2]);
    }

    if ((data.Options.CalculateTangents || !tangents) && mesh.UVs.HasItems())
    {
        if (mesh.GenerateTangents(data.Options.SmoothingTangentsAngle))
        {
            errorMsg = TEXT("Failed to generate tangents.");
            return true;
        }
    }

    if (data.Options.OptimizeMeshes)
    {
        mesh.ImproveCacheLocality();
    }

    // Apply FBX Mesh geometry transformation
    /*const Matrix geometryTransform = ToMatrix(aMesh->getGeometricMatrix());
    if (!geometryTransform.IsIdentity())
    {
        mesh.TransformBuffer(geometryTransform);
    }*/

    // Get local transform for origin shifting translation
    auto translation = ToMatrix(aMesh->getGlobalTransform()).GetTranslation();
    auto scale = data.GlobalSettings.UnitScaleFactor;
    if (data.GlobalSettings.CoordAxis == ofbx::CoordSystem_RightHanded)
        mesh.OriginTranslation = scale * Vector3(translation.X, translation.Y, -translation.Z);
    else
        mesh.OriginTranslation = scale * Vector3(translation.X, translation.Y, translation.Z);
    
    auto rot = aMesh->getLocalRotation();
    auto quat = Quaternion::Euler(-(float)rot.x, -(float)rot.y, -(float)rot.z);
    mesh.OriginOrientation = quat;

    auto scaling = aMesh->getLocalScaling();
    mesh.Scaling = Vector3(scale * (float)scaling.x, scale * (float)scaling.y, scale * (float)scaling.z);
    return false;
}

bool ImportMesh(ImportedModelData& result, OpenFbxImporterData& data, const ofbx::Mesh* aMesh, String& errorMsg, int32 triangleStart, int32 triangleEnd)
{
    // Find the parent node
    int32 nodeIndex = data.FindNode(aMesh);

    // Special case for some models without nodes structure (only root but with some meshes inside)
    if (nodeIndex == -1 && data.Nodes[0].FbxObj->resolveObjectLink(0) == nullptr)
    {
        nodeIndex = data.Nodes.Count();

        // Create dummy node
        FbxNode node;
        node.ParentIndex = 0;
        node.Name = aMesh->name;
        node.FbxObj = nullptr;

        // Pick node LOD index
        if (!data.Options.ImportLODs)
        {
            node.LodIndex = 0;
        }
        else
        {
            node.LodIndex = data.Nodes[0].LodIndex;
            if (node.LodIndex == 0)
                node.LodIndex = ModelTool::DetectLodIndex(node.Name);
            ASSERT(Math::IsInRange(node.LodIndex, 0, MODEL_MAX_LODS - 1));
        }
        node.LocalTransform = Transform::Identity;
        data.Nodes.Add(node);
    }
    if (nodeIndex == -1)
    {
        LOG(Warning, "Invalid mesh linkage. Mesh: {0}. Skipping...", String(aMesh->name));
        return false;
    }

    // Import mesh data
    MeshData* meshData = New<MeshData>();
    if (ProcessMesh(result, data, aMesh, *meshData, errorMsg, triangleStart, triangleEnd))
        return true;

    // Link mesh
    auto& node = data.Nodes[nodeIndex];
    const int32 lodIndex = node.LodIndex;
    meshData->NodeIndex = nodeIndex;
    if (result.LODs.Count() <= lodIndex)
        result.LODs.Resize(lodIndex + 1);
    result.LODs[lodIndex].Meshes.Add(meshData);

    return false;
}

bool ImportMesh(int32 index, ImportedModelData& result, OpenFbxImporterData& data, String& errorMsg)
{
    const auto aMesh = data.Scene->getMesh(index);
    const auto aGeometry = aMesh->getGeometry();
    const auto trianglesCount = aGeometry->getVertexCount() / 3;
    if (IsMeshInvalid(aMesh))
        return false;

    if (aMesh->getMaterialCount() < 2 || !aGeometry->getMaterials())
    {
        // Fast path if mesh is using single material for all triangles
        if (ImportMesh(result, data, aMesh, errorMsg, 0, trianglesCount - 1))
            return true;
    }
    else
    {
        // Create mesh for each sequence of triangles that share the same material
        const auto materials = aGeometry->getMaterials();
        int32 rangeStart = 0;
        int32 rangeStartVal = materials[rangeStart];
        for (int32 triangleIndex = 1; triangleIndex < trianglesCount; triangleIndex++)
        {
            if (rangeStartVal != materials[triangleIndex])
            {
                if (ImportMesh(result, data, aMesh, errorMsg, rangeStart, triangleIndex - 1))
                    return true;

                // Start a new range
                rangeStart = triangleIndex;
                rangeStartVal = materials[triangleIndex];
            }
        }
        if (ImportMesh(result, data, aMesh, errorMsg, rangeStart, trianglesCount - 1))
            return true;
    }
    return false;
}

struct AnimInfo
{
    double TimeStart;
    double TimeEnd;
    double Duration;
    int32 FramesCount;
    float SamplingPeriod;
};

struct Frame
{
    ofbx::Vec3 Translation;
    ofbx::Vec3 Rotation;
    ofbx::Vec3 Scaling;
};

void ExtractKeyframePosition(const ofbx::Object* bone, ofbx::Vec3& trans, const Frame& localFrame, Float3& keyframe)
{
    const Matrix frameTrans = ToMatrix(bone->evalLocal(trans, localFrame.Rotation, localFrame.Scaling));
    keyframe = frameTrans.GetTranslation();
}

void ExtractKeyframeRotation(const ofbx::Object* bone, ofbx::Vec3& trans, const Frame& localFrame, Quaternion& keyframe)
{
    const Matrix frameTrans = ToMatrix(bone->evalLocal(localFrame.Translation, trans, { 1.0, 1.0, 1.0 }));
    Quaternion::RotationMatrix(frameTrans, keyframe);
}

void ExtractKeyframeScale(const ofbx::Object* bone, ofbx::Vec3& trans, const Frame& localFrame, Float3& keyframe)
{
    // Fix empty scale case
    if (Math::IsZero(trans.x) && Math::IsZero(trans.y) && Math::IsZero(trans.z))
        trans = { 1.0, 1.0, 1.0 };

    const Matrix frameTrans = ToMatrix(bone->evalLocal(localFrame.Translation, localFrame.Rotation, trans));
    keyframe = frameTrans.GetScaleVector();
}

template<typename T>
void ImportCurve(const ofbx::AnimationCurveNode* curveNode, LinearCurve<T>& curve, AnimInfo& info, void (*ExtractKeyframe)(const ofbx::Object*, ofbx::Vec3&, const Frame&, T&))
{
    if (curveNode == nullptr)
        return;
    const auto keyframes = curve.Resize(info.FramesCount);
    const auto bone = curveNode->getBone();
    Frame localFrame;
    localFrame.Translation = bone->getLocalTranslation();
    localFrame.Rotation = bone->getLocalRotation();
    localFrame.Scaling = bone->getLocalScaling();

    for (int32 i = 0; i < info.FramesCount; i++)
    {
        auto& key = keyframes[i];
        const double t = info.TimeStart + ((double)i / info.FramesCount) * info.Duration;

        key.Time = (float)i;

        ofbx::Vec3 trans = curveNode->getNodeLocalTransform(t);
        ExtractKeyframe(bone, trans, localFrame, key.Value);
    }
}

bool ImportAnimation(int32 index, ImportedModelData& data, OpenFbxImporterData& importerData)
{
    const ofbx::AnimationStack* stack = importerData.Scene->getAnimationStack(index);
    const ofbx::AnimationLayer* layer = stack->getLayer(0);
    const ofbx::TakeInfo* takeInfo = importerData.Scene->getTakeInfo(stack->name);
    if (takeInfo == nullptr)
        return true;

    // Initialize animation animation keyframes sampling
    const float frameRate = importerData.FrameRate;
    data.Animation.FramesPerSecond = frameRate;
    const double localDuration = takeInfo->local_time_to - takeInfo->local_time_from;
    if (localDuration <= ZeroTolerance)
        return true;
    data.Animation.Duration = (double)(int32)(localDuration * frameRate + 0.5f);
    AnimInfo info;
    info.TimeStart = takeInfo->local_time_from;
    info.TimeEnd = takeInfo->local_time_to;
    info.Duration = localDuration;
    info.FramesCount = (int32)data.Animation.Duration;
    info.SamplingPeriod = 1.0f / frameRate;

    // Count valid animation channels
    Array<int32> animatedNodes(importerData.Nodes.Count());
    for (int32 nodeIndex = 0; nodeIndex < importerData.Nodes.Count(); nodeIndex++)
    {
        auto& aNode = importerData.Nodes[nodeIndex];

        const ofbx::AnimationCurveNode* translationNode = layer->getCurveNode(*aNode.FbxObj, "Lcl Translation");
        const ofbx::AnimationCurveNode* rotationNode = layer->getCurveNode(*aNode.FbxObj, "Lcl Rotation");
        const ofbx::AnimationCurveNode* scalingNode = layer->getCurveNode(*aNode.FbxObj, "Lcl Scaling");

        if (translationNode || rotationNode || (scalingNode && importerData.Options.ImportScaleTracks))
            animatedNodes.Add(nodeIndex);
    }
    if (animatedNodes.IsEmpty())
        return true;
    data.Animation.Channels.Resize(animatedNodes.Count(), false);

    // Import curves
    for (int32 i = 0; i < animatedNodes.Count(); i++)
    {
        const int32 nodeIndex = animatedNodes[i];
        auto& aNode = importerData.Nodes[nodeIndex];
        auto& anim = data.Animation.Channels[i];

        const ofbx::AnimationCurveNode* translationNode = layer->getCurveNode(*aNode.FbxObj, "Lcl Translation");
        const ofbx::AnimationCurveNode* rotationNode = layer->getCurveNode(*aNode.FbxObj, "Lcl Rotation");
        const ofbx::AnimationCurveNode* scalingNode = layer->getCurveNode(*aNode.FbxObj, "Lcl Scaling");

        anim.NodeName = aNode.Name;

        ImportCurve(translationNode, anim.Position, info, ExtractKeyframePosition);
        ImportCurve(rotationNode, anim.Rotation, info, ExtractKeyframeRotation);
        if (importerData.Options.ImportScaleTracks)
            ImportCurve(scalingNode, anim.Scale, info, ExtractKeyframeScale);
    }

    if (importerData.ConvertRH)
    {
        for (int32 i = 0; i < data.Animation.Channels.Count(); i++)
        {
            auto& anim = data.Animation.Channels[i];
            auto& posKeys = anim.Position.GetKeyframes();
            auto& rotKeys = anim.Rotation.GetKeyframes();

            for (int32 k = 0; k < posKeys.Count(); k++)
            {
                posKeys[k].Value.Z *= -1.0f;
            }

            for (int32 k = 0; k < rotKeys.Count(); k++)
            {
                rotKeys[k].Value.X *= -1.0f;
                rotKeys[k].Value.Y *= -1.0f;
            }
        }
    }

    return false;
}

static Float3 FbxVectorFromAxisAndSign(int axis, int sign)
{
    switch (axis)
    {
    case 0:
        return { sign ? 1.f : -1.f, 0.f, 0.f };
    case 1:
        return { 0.f, sign ? 1.f : -1.f, 0.f };
    case 2:
        return { 0.f, 0.f, sign ? 1.f : -1.f };
    }

    return { 0.f, 0.f, 0.f };
}

bool ModelTool::ImportDataOpenFBX(const char* path, ImportedModelData& data, Options& options, String& errorMsg)
{
    auto context = (OpenFbxImporterData*)options.SplitContext;
    if (!context)
    {
        // Import file
        Array<byte> fileData;
        if (File::ReadAllBytes(String(path), fileData))
        {
            errorMsg = TEXT("Cannot load file.");
            return true;
        }
        ofbx::u64 loadFlags = 0;
        if (EnumHasAnyFlags(data.Types, ImportDataTypes::Geometry))
            loadFlags |= (ofbx::u64)ofbx::LoadFlags::TRIANGULATE;
        else
            loadFlags |= (ofbx::u64)ofbx::LoadFlags::IGNORE_GEOMETRY;
        if (!options.ImportBlendShapes)
            loadFlags |= (ofbx::u64)ofbx::LoadFlags::IGNORE_BLEND_SHAPES;
        ofbx::IScene* scene = ofbx::load(fileData.Get(), fileData.Count(), loadFlags);
        if (!scene)
        {
            errorMsg = ofbx::getError();
            return true;
        }
        fileData.Resize(0);

        // Tweak scene if exported by Blender
        auto& globalInfo = *scene->getGlobalInfo();
        if (StringAnsiView(globalInfo.AppName).StartsWith(StringAnsiView("Blender"), StringSearchCase::IgnoreCase))
        {
            auto ptr = const_cast<ofbx::GlobalSettings*>(scene->getGlobalSettings());
            ptr->UpAxis = (ofbx::UpVector)((int32)ptr->UpAxis + 1);
        }

        // Process imported scene
        context = New<OpenFbxImporterData>(path, options, scene);
        auto& globalSettings = context->GlobalSettings;
        ProcessNodes(*context, scene->getRoot(), -1);

        // Apply model scene global scale factor
        context->Nodes[0].LocalTransform = Transform(Vector3::Zero, Quaternion::Identity, globalSettings.UnitScaleFactor) * context->Nodes[0].LocalTransform;

        // Log scene info
        LOG(Info, "Loaded FBX model, Frame Rate: {0}, Unit Scale Factor: {1}", context->FrameRate, globalSettings.UnitScaleFactor);
        LOG(Info, "{0}, {1}, {2}", String(globalInfo.AppName), String(globalInfo.AppVersion), String(globalInfo.AppVendor));
        LOG(Info, "Up: {1}{0}", globalSettings.UpAxis == ofbx::UpVector_AxisX ? TEXT("X") : globalSettings.UpAxis == ofbx::UpVector_AxisY ? TEXT("Y") : TEXT("Z"), globalSettings.UpAxisSign == 1 ? TEXT("+") : TEXT("-"));
        LOG(Info, "Front: {1}{0}", globalSettings.FrontAxis == ofbx::FrontVector_ParityEven ? TEXT("ParityEven") : TEXT("ParityOdd"), globalSettings.FrontAxisSign == 1 ? TEXT("+") : TEXT("-"));
        LOG(Info, "{0} Handed{1}", globalSettings.CoordAxis == ofbx::CoordSystem_RightHanded ? TEXT("Right") : TEXT("Left"), globalSettings.CoordAxisSign == 1 ? TEXT("") : TEXT(" (negative)"));
#if OPEN_FBX_CONVERT_SPACE
        LOG(Info, "Imported scene: Up={0}, Front={1}, Right={2}", context->Up, context->Front, context->Right);
#endif

        // Extract embedded textures
        if (EnumHasAnyFlags(data.Types, ImportDataTypes::Textures))
        {
            String outputPath;
            for (int i = 0, c = scene->getEmbeddedDataCount(); i < c; i++)
            {
                const ofbx::DataView aEmbedded = scene->getEmbeddedData(i);
                ofbx::DataView aFilename = scene->getEmbeddedFilename(i);
                char filenameData[256];
                aFilename.toString(filenameData);
                if (outputPath.IsEmpty())
                {
                    String pathStr(path);
                    outputPath = String(StringUtils::GetDirectoryName(pathStr)) / TEXT("textures");
                    FileSystem::CreateDirectory(outputPath);
                }
                const String filenameStr(filenameData);
                String embeddedPath = outputPath / StringUtils::GetFileName(filenameStr);
                if (FileSystem::FileExists(embeddedPath))
                    continue;
                LOG(Info, "Extracing embedded resource to {0}", embeddedPath);
                if (File::WriteAllBytes(embeddedPath, aEmbedded.begin + 4, (int32)(aEmbedded.end - aEmbedded.begin - 4)))
                {
                    LOG(Error, "Failed to write data to file");
                }
            }
        }

#if OPEN_FBX_CONVERT_SPACE
        // Transform nodes to match the engine coordinates system - DirectX (UpVector = +Y, FrontVector = +Z, CoordSystem = -X (LeftHanded))
        if (context->Up == Float3(1, 0, 0) && context->Front == Float3(0, 0, 1) && context->Right == Float3(0, 1, 0))
        {
            context->RootConvertRotation = Quaternion::Euler(0, 180, 0);
        }
        else if (context->Up == Float3(0, 1, 0) && context->Front == Float3(-1, 0, 0) && context->Right == Float3(0, 0, 1))
        {
            context->RootConvertRotation = Quaternion::Euler(90, -90, 0);
        }
        /*Float3 engineUp(0, 1, 0);
        Float3 engineFront(0, 0, 1);
        Float3 engineRight(-1, 0, 0);*/
        /*Float3 engineUp(1, 0, 0);
        Float3 engineFront(0, 0, 1);
        Float3 engineRight(0, 1, 0);
        if (context->Up != engineUp || context->Front != engineFront || context->Right != engineRight)
        {
            LOG(Info, "Converting imported scene nodes to match engine coordinates system");
            context->RootConvertRotation = Quaternion::GetRotationFromTo(context->Up, engineUp, engineUp);
            //context->RootConvertRotation *= Quaternion::GetRotationFromTo(rotation * context->Right, engineRight, engineRight);
            //context->RootConvertRotation *= Quaternion::GetRotationFromTo(rotation * context->Front, engineFront, engineFront);
        }*/
        /*Float3 hackUp = FbxVectorFromAxisAndSign(globalSettings.UpAxis, globalSettings.UpAxisSign);
        if (hackUp == Float3::UnitX)
            context->RootConvertRotation = Quaternion::Euler(-90, 0, 0);
        else if (hackUp == Float3::UnitZ)
            context->RootConvertRotation = Quaternion::Euler(90, 0, 0);*/
        if (!context->RootConvertRotation.IsIdentity())
        {
            for (auto& node : context->Nodes)
            {
                if (node.ParentIndex == -1)
                {
                    node.LocalTransform.Orientation = context->RootConvertRotation * node.LocalTransform.Orientation;
                    break;
                }
            }
        }
#endif
    }
    DeleteMe<OpenFbxImporterData> contextCleanup(options.SplitContext ? nullptr : context);

    // Build final skeleton bones hierarchy before importing meshes
    if (EnumHasAnyFlags(data.Types, ImportDataTypes::Skeleton))
    {
        if (ImportBones(*context, errorMsg))
        {
            LOG(Warning, "Failed to import skeleton bones.");
            return true;
        }

        Sorting::QuickSort(context->Bones.Get(), context->Bones.Count());
    }

    // Import geometry (meshes and materials)
    if (EnumHasAnyFlags(data.Types, ImportDataTypes::Geometry) && context->Scene->getMeshCount() > 0)
    {
        const int meshCount = context->Scene->getMeshCount();
        if (options.SplitObjects && options.ObjectIndex == -1 && meshCount > 1)
        {
            // Import the first object within this call
            options.SplitObjects = false;
            options.ObjectIndex = 0;

            if (options.OnSplitImport.IsBinded())
            {
                // Split all animations into separate assets
                LOG(Info, "Splitting imported {0} meshes", meshCount);
                for (int32 i = 1; i < meshCount; i++)
                {
                    auto splitOptions = options;
                    splitOptions.ObjectIndex = i;
                    splitOptions.SplitContext = context;
                    const auto aMesh = context->Scene->getMesh(i);
                    const String objectName(aMesh->name);
                    options.OnSplitImport(splitOptions, objectName);
                }
            }
        }
        if (options.ObjectIndex != -1)
        {
            // Import the selected mesh
            const auto meshIndex = Math::Clamp<int32>(options.ObjectIndex, 0, meshCount - 1);
            if (ImportMesh(meshIndex, data, *context, errorMsg))
                return true;

            // Let the firstly imported mesh import all materials from all meshes (index 0 is importing all following ones before itself during splitting - see code above)
            if (options.ObjectIndex == 1)
            {
                for (int32 i = 0; i < meshCount; i++)
                {
                    const auto aMesh = context->Scene->getMesh(i);
                    if (i == 1 || IsMeshInvalid(aMesh))
                        continue;
                    for (int32 j = 0; j < aMesh->getMaterialCount(); j++)
                    {
                        const ofbx::Material* aMaterial = aMesh->getMaterial(j);
                        context->AddMaterial(data, aMaterial);
                    }
                }
            }
        }
        else
        {
            // Import all meshes
            for (int32 meshIndex = 0; meshIndex < meshCount; meshIndex++)
            {
                if (ImportMesh(meshIndex, data, *context, errorMsg))
                    return true;
            }
        }
    }

    // Import skeleton
    if (EnumHasAnyFlags(data.Types, ImportDataTypes::Skeleton))
    {
        data.Skeleton.Nodes.Resize(context->Nodes.Count(), false);
        for (int32 i = 0; i < context->Nodes.Count(); i++)
        {
            auto& node = data.Skeleton.Nodes[i];
            auto& aNode = context->Nodes[i];

            node.Name = aNode.Name;
            node.ParentIndex = aNode.ParentIndex;
            node.LocalTransform = aNode.LocalTransform;
        }

        data.Skeleton.Bones.Resize(context->Bones.Count(), false);
        for (int32 i = 0; i < context->Bones.Count(); i++)
        {
            auto& bone = data.Skeleton.Bones[i];
            auto& aBone = context->Bones[i];
            const auto boneNodeIndex = aBone.NodeIndex;

            // Find the parent bone
            int32 parentBoneIndex = -1;
            for (int32 j = context->Nodes[boneNodeIndex].ParentIndex; j != -1; j = context->Nodes[j].ParentIndex)
            {
                parentBoneIndex = context->FindBone(j);
                if (parentBoneIndex != -1)
                    break;
            }
            aBone.ParentBoneIndex = parentBoneIndex;

            const auto parentBoneNodeIndex = aBone.ParentBoneIndex == -1 ? -1 : context->Bones[aBone.ParentBoneIndex].NodeIndex;

            bone.ParentIndex = aBone.ParentBoneIndex;
            bone.NodeIndex = aBone.NodeIndex;
            bone.LocalTransform = CombineTransformsFromNodeIndices(context->Nodes, parentBoneNodeIndex, boneNodeIndex);
            bone.OffsetMatrix = aBone.OffsetMatrix;
        }
    }

    // Import animations
    if (EnumHasAnyFlags(data.Types, ImportDataTypes::Animations))
    {
        const int animCount = context->Scene->getAnimationStackCount();
        if (options.SplitObjects && options.ObjectIndex == -1 && animCount > 1)
        {
            // Import the first object within this call
            options.SplitObjects = false;
            options.ObjectIndex = 0;

            if (options.OnSplitImport.IsBinded())
            {
                // Split all animations into separate assets
                LOG(Info, "Splitting imported {0} animations", animCount);
                for (int32 i = 1; i < animCount; i++)
                {
                    auto splitOptions = options;
                    splitOptions.ObjectIndex = i;
                    splitOptions.SplitContext = context;
                    const ofbx::AnimationStack* stack = context->Scene->getAnimationStack(i);
                    const ofbx::AnimationLayer* layer = stack->getLayer(0);
                    const String objectName(layer->name);
                    options.OnSplitImport(splitOptions, objectName);
                }
            }
        }
        if (options.ObjectIndex != -1)
        {
            // Import selected animation
            const auto animIndex = Math::Clamp<int32>(options.ObjectIndex, 0, animCount - 1);
            ImportAnimation(animIndex, data, *context);
        }
        else
        {
            // Import first valid animation
            for (int32 animIndex = 0; animIndex < animCount; animIndex++)
            {
                if (!ImportAnimation(animIndex, data, *context))
                    break;
            }
        }
    }

    // Import nodes
    if (EnumHasAnyFlags(data.Types, ImportDataTypes::Nodes))
    {
        data.Nodes.Resize(context->Nodes.Count());
        for (int32 i = 0; i < context->Nodes.Count(); i++)
        {
            auto& node = data.Nodes[i];
            auto& aNode = context->Nodes[i];

            node.Name = aNode.Name;
            node.ParentIndex = aNode.ParentIndex;
            node.LocalTransform = aNode.LocalTransform;
        }
    }

    return false;
}

#endif
