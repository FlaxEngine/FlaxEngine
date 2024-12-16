// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#if COMPILE_WITH_MODEL_TOOL && USE_AUTODESK_FBX_SDK

#include "ModelTool.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Math/Matrix.h"
#include "Engine/Threading/Threading.h"

// Import Autodesk FBX SDK
#define FBXSDK_NEW_API
#include <fbxsdk.h>

class FbxSdkManager
{
public:

    static FbxManager* Manager;
    static CriticalSection Locker;

    static void Init()
    {
        if (Manager == nullptr)
        {
            LOG_STR(Info, String("Autodesk FBX SDK " FBXSDK_VERSION_STRING_FULL));

            Manager = FbxManager::Create();
            if (Manager == nullptr)
            {
                LOG(Fatal, "Autodesk FBX SDK failed to initialize.");
                return;
            }

            FbxIOSettings* ios = FbxIOSettings::Create(Manager, IOSROOT);
            ios->SetBoolProp(IMP_FBX_TEXTURE, false);
            ios->SetBoolProp(IMP_FBX_GOBO, false);
            Manager->SetIOSettings(ios);
        }
    }
};

FbxManager* FbxSdkManager::Manager = nullptr;
CriticalSection FbxSdkManager::Locker;

Matrix ToFlaxType(const FbxAMatrix& value)
{
    Matrix native;
    for (int32 row = 0; row < 4; row++)
        for (int32 col = 0; col < 4; col++)
            native.Values[row][col] = (float)value[col][row];
    return native;
}

Float3 ToFlaxType(const FbxVector4& value)
{
    Float3 native;
    native.X = (float)value[0];
    native.Y = (float)value[1];
    native.Z = (float)value[2];
    return native;
}

Float3 ToFlaxType(const FbxDouble3& value)
{
    Float3 native;
    native.X = (float)value[0];
    native.Y = (float)value[1];
    native.Z = (float)value[2];
    return native;
}

Float2 ToFlaxType(const FbxVector2& value)
{
    Float2 native;
    native.X = (float)value[0];
    native.Y = 1 - (float)value[1];
    return native;
}

Color ToFlaxType(const FbxColor& value)
{
    Color native;
    native.R = (float)value[0];
    native.G = (float)value[1];
    native.B = (float)value[2];
    native.A = (float)value[3];
    return native;
}

int ToFlaxType(const int& value)
{
    return value;
}

/// <summary>
/// Represents a single node in the FBX transform hierarchy.
/// </summary>
struct Node
{
    /// <summary>
    /// The parent index. The root node uses value -1.
    /// </summary>
    int32 ParentIndex;

    /// <summary>
    /// The local transformation of the node, relative to parent node.
    /// </summary>
    Transform LocalTransform;

    /// <summary>
    /// The name of this node.
    /// </summary>
    String Name;

    /// <summary>
    /// The LOD index of the data in this node (used to separate meshes across different level of details).
    /// </summary>
    int32 LodIndex;

    Matrix GeomTransform;
    Matrix WorldTransform;
    FbxNode* FbxNode;
};

struct Bone
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
};

struct ImporterData
{
    ImportedModelData& Model;
    const FbxScene* Scene;
    const ModelTool::Options& Options;

    Array<Node> Nodes;
    Array<Bone> Bones;

    Dictionary<FbxMesh*, MeshData*> Meshes;
    Array<FbxSurfaceMaterial*> Materials;

    ImporterData(ImportedModelData& model, const ModelTool::Options& options, const FbxScene* scene)
        : Model(model)
        , Scene(scene)
        , Options(options)
        , Nodes(256)
        , Meshes(256)
        , Materials(64)
    {
    }

    int32 FindNode(FbxNode* fbxNode)
    {
        for (int32 i = 0; i < Nodes.Count(); i++)
        {
            if (Nodes[i].FbxNode == fbxNode)
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

void ProcessNodes(ImporterData& data, FbxNode* fbxNode, int32 parentIndex)
{
    const int32 nodeIndex = data.Nodes.Count();

    Float3 translation = ToFlaxType(fbxNode->EvaluateLocalTranslation(FbxTime(0)));
    Float3 rotationEuler = ToFlaxType(fbxNode->EvaluateLocalRotation(FbxTime(0)));
    Float3 scale = ToFlaxType(fbxNode->EvaluateLocalScaling(FbxTime(0)));
    Quaternion rotation = Quaternion::Euler(rotationEuler);

    // Create node
    Node node;
    node.ParentIndex = parentIndex;
    node.Name = String(fbxNode->GetNameWithoutNameSpacePrefix().Buffer());
    node.LocalTransform = Transform(translation, rotation, scale);
    node.FbxNode = fbxNode;

    // Geometry transform is applied to geometry (mesh data) only, it is not inherited by children, so we store it separately
    Float3 geomTrans = ToFlaxType(fbxNode->GeometricTranslation.Get());
    Float3 geomRotEuler = ToFlaxType(fbxNode->GeometricRotation.Get());
    Float3 geomScale = ToFlaxType(fbxNode->GeometricScaling.Get());
    Quaternion geomRotation = Quaternion::Euler(geomRotEuler);
    Transform(geomTrans, geomRotation, geomScale).GetWorld(node.GeomTransform);

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

    if (parentIndex == -1)
    {
        node.LocalTransform.GetWorld(node.WorldTransform);
    }
    else
    {
        node.WorldTransform = data.Nodes[parentIndex].WorldTransform * node.LocalTransform.GetWorld();
    }
    data.Nodes.Add(node);

    // Process the children
    for (int i = 0; i < fbxNode->GetChildCount(); i++)
    {
        ProcessNodes(data, fbxNode->GetChild(i), nodeIndex);
    }
}

template<class TFBX, class TNative>
void ReadLayerData(FbxMesh* fbxMesh, FbxLayerElementTemplate<TFBX>& layer, Array<TNative>& output)
{
    if (layer.GetDirectArray().GetCount() == 0)
        return;

    int32 vertexCount = fbxMesh->GetControlPointsCount();
    int32 triangleCount = fbxMesh->GetPolygonCount();
    output.Resize(vertexCount);

    switch (layer.GetMappingMode())
    {
    case FbxLayerElement::eByControlPoint:
    {
        for (int vertexIndex = 0; vertexIndex < vertexCount; vertexIndex++)
        {
            int index = 0;
            if (layer.GetReferenceMode() == FbxGeometryElement::eDirect)
                index = vertexIndex;
            else if (layer.GetReferenceMode() == FbxGeometryElement::eIndexToDirect)
                index = layer.GetIndexArray().GetAt(vertexIndex);

            output[vertexIndex] = ToFlaxType(layer.GetDirectArray().GetAt(index));
        }
    }
    break;
    case FbxLayerElement::eByPolygonVertex:
    {
        int indexByPolygonVertex = 0;

        for (int polygonIndex = 0; polygonIndex < triangleCount; polygonIndex++)
        {
            const int polygonSize = fbxMesh->GetPolygonSize(polygonIndex);
            for (int i = 0; i < polygonSize; i++)
            {
                int index = 0;
                if (layer.GetReferenceMode() == FbxGeometryElement::eDirect)
                    index = indexByPolygonVertex;
                else if (layer.GetReferenceMode() == FbxGeometryElement::eIndexToDirect)
                    index = layer.GetIndexArray().GetAt(indexByPolygonVertex);

                int vertexIndex = fbxMesh->GetPolygonVertex(polygonIndex, i);
                output[vertexIndex] = ToFlaxType(layer.GetDirectArray().GetAt(index));

                indexByPolygonVertex++;
            }
        }
    }
    break;
    case FbxLayerElement::eAllSame:
    {
        output[0] = ToFlaxType(layer.GetDirectArray().GetAt(0));
        for (int vertexIndex = 1; vertexIndex < vertexCount; vertexIndex++)
        {
            output[vertexIndex] = output[0];
        }
    }
    break;
    default:
        LOG(Warning, "Unsupported layer mapping mode.");
        break;
    }
}

bool IsGroupMappingModeByEdge(FbxLayerElement* layerElement)
{
    return layerElement->GetMappingMode() == FbxLayerElement::eByEdge;
}

bool ProcessMesh(ImporterData& data, FbxMesh* fbxMesh, MeshData& mesh, String& errorMsg)
{
    // Properties
    mesh.Name = fbxMesh->GetName();
    mesh.MaterialSlotIndex = -1;
    if (fbxMesh->GetElementMaterial())
    {
        const auto materialIndices = &(fbxMesh->GetElementMaterial()->GetIndexArray());
        if (materialIndices)
        {
            mesh.MaterialSlotIndex = materialIndices->GetAt(0);
        }
    }

    int32 vertexCount = fbxMesh->GetControlPointsCount();
    int32 triangleCount = fbxMesh->GetPolygonCount();
    FbxVector4* controlPoints = fbxMesh->GetControlPoints();
    FbxGeometryElementNormal* normalElement = fbxMesh->GetElementNormal();
    FbxGeometryElementTangent* tangentElement = fbxMesh->GetElementTangent();

    // Regenerate data if necessary
    if (normalElement == nullptr || data.Options.CalculateNormals)
    {
        fbxMesh->GenerateNormals(true, false, false);
        normalElement = fbxMesh->GetElementNormal();
    }
    if (tangentElement == nullptr || data.Options.CalculateTangents)
    {
        fbxMesh->GenerateTangentsData(0, true);
        tangentElement = fbxMesh->GetElementTangent();
    }

    bool needEdgeIndexing = false;
    if (normalElement)
        needEdgeIndexing |= IsGroupMappingModeByEdge(normalElement);

    // Vertex positions
    mesh.Positions.Resize(vertexCount, false);
    for (int32 i = 0; i < vertexCount; i++)
    {
        mesh.Positions[i] = ToFlaxType(controlPoints[i]);
    }

    // Indices
    const int32 indexCount = triangleCount * 3;
    mesh.Indices.Resize(indexCount, false);
    int* fbxIndices = fbxMesh->GetPolygonVertices();
    for (int32 i = 0; i < indexCount; i++)
    {
        mesh.Indices[i] = fbxIndices[i];
    }

    if (data.Options.ReverseWindingOrder)
    {
        for (int32 i = 0; i < vertexCount; i += 3)
        {
            Swap(meshIndices[i + 1], meshIndices[i + 2]);
            Swap(meshPositions[i + 1], meshPositions[i + 2]);
            if (meshNormals)
                Swap(meshNormals[i + 1], meshNormals[i + 2]);
            if (meshTangents)
                Swap(meshTangents[i + 1], meshTangents[i + 2]);
        }
    }

    // Texture coordinates
    FbxGeometryElementUV* texcoords = fbxMesh->GetElementUV(0);
    if (texcoords)
    {
        ReadLayerData(fbxMesh, *texcoords, mesh.UVs);
    }

    // Normals
    if (normalElement)
    {
        ReadLayerData(fbxMesh, *normalElement, mesh.Normals);
    }

    // Tangents
    if (tangentElement)
    {
        ReadLayerData(fbxMesh, *tangentElement, mesh.Tangents);
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
            // TODO: we could propagate this message to Debug Console in editor? or create interface to gather some msgs from importing service
            LOG(Warning, "Failed to generate lightmap uvs");
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
        if (inputChannelIndex >= 0 && inputChannelIndex < fbxMesh->GetElementUVCount() && fbxMesh->GetElementUV(inputChannelIndex))
        {
            ReadLayerData(fbxMesh, *fbxMesh->GetElementUV(inputChannelIndex), mesh.LightmapUVs);
        }
        else
        {
            // TODO: we could propagate this message to Debug Console in editor? or create interface to gather some msgs from importing service
            LOG(Warning, "Cannot import model lightmap uvs. Missing texcoords channel {0}.", inputChannelIndex);
        }
    }

    // Vertex Colors
    if (data.Options.ImportVertexColors && fbxMesh->GetElementVertexColorCount() > 0)
    {
        auto vertexColorElement = fbxMesh->GetElementVertexColor(0);
        ReadLayerData(fbxMesh, *vertexColorElement, mesh.Colors);
    }

    // Blend Indices and Blend Weights
    const int skinDeformerCount = fbxMesh->GetDeformerCount(FbxDeformer::eSkin);
    if (skinDeformerCount > 0)
    {
        const int32 vertexCount = mesh.Positions.Count();
        mesh.BlendIndices.Resize(vertexCount);
        mesh.BlendWeights.Resize(vertexCount);
        mesh.BlendIndices.SetAll(Int4::Zero);
        mesh.BlendWeights.SetAll(Float4::Zero);

        for (int deformerIndex = 0; deformerIndex < skinDeformerCount; deformerIndex++)
        {
            FbxSkin* skin = FbxCast<FbxSkin>(fbxMesh->GetDeformer(deformerIndex, FbxDeformer::eSkin));
            int totalClusterCount = skin->GetClusterCount();

            for (int clusterIndex = 0; clusterIndex < totalClusterCount; ++clusterIndex)
            {
                FbxCluster* cluster = skin->GetCluster(clusterIndex);
                int indexCount = cluster->GetControlPointIndicesCount();
                if (indexCount == 0)
                    continue;
                FbxNode* link = cluster->GetLink();
                const String boneName(link->GetName());

                // Find the node where the bone is mapped - based on the name
                const int32 nodeIndex = data.FindNode(link);
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

                    FbxAMatrix transformMatrix;
                    FbxAMatrix transformLinkMatrix;
                    cluster->GetTransformMatrix(transformMatrix);
                    cluster->GetTransformLinkMatrix(transformLinkMatrix);
                    const auto globalBindposeInverseMatrix = transformLinkMatrix.Inverse() * transformMatrix;

                    // Setup bone
                    bone.Name = boneName;
                    bone.NodeIndex = nodeIndex;
                    bone.ParentBoneIndex = parentBoneIndex;
                    bone.OffsetMatrix = ToFlaxType(globalBindposeInverseMatrix);
                }

                // Apply the bone influences
                int* cluserIndices = cluster->GetControlPointIndices();
                double* cluserWeights = cluster->GetControlPointWeights();
                for (int j = 0; j < indexCount; j++)
                {
                    const int vtxWeightId = cluserIndices[j];

                    if (vtxWeightId >= vertexCount)
                        continue;

                    const auto vtxWeight = (float)cluserWeights[j];

                    if (vtxWeight <= 0 || isnan(vtxWeight) || isinf(vtxWeight))
                        continue;

                    auto& indices = mesh.BlendIndices[vtxWeightId];
                    auto& weights = mesh.BlendWeights[vtxWeightId];

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
        }

        mesh.NormalizeBlendWeights();
    }

    // Blend Shapes
    const int blendShapeDeformerCount = fbxMesh->GetDeformerCount(FbxDeformer::eBlendShape);
    if (blendShapeDeformerCount > 0 && data.Model.Types & ImportDataTypes::Skeleton && data.Options.ImportBlendShapes)
    {
        mesh.BlendShapes.EnsureCapacity(blendShapeDeformerCount);
        for (int deformerIndex = 0; deformerIndex < skinDeformerCount; deformerIndex++)
        {
            FbxBlendShape* blendShape = FbxCast<FbxBlendShape>(fbxMesh->GetDeformer(deformerIndex, FbxDeformer::eBlendShape));

            const int blendShapeChannelCount = blendShape->GetBlendShapeChannelCount();
            for (int32 channelIndex = 0; channelIndex < blendShapeChannelCount; channelIndex++)
            {
                FbxBlendShapeChannel* blendShapeChannel = blendShape->GetBlendShapeChannel(channelIndex);

                // Use last shape
                const int shapeCount = blendShapeChannel->GetTargetShapeCount();
                if (shapeCount == 0)
                    continue;
                FbxShape* shape = blendShapeChannel->GetTargetShape(shapeCount - 1);

                int shapeControlPointsCount = shape->GetControlPointsCount();
                if (shapeControlPointsCount != vertexCount)
                    continue;

                BlendShape& blendShapeData = mesh.BlendShapes.AddOne();
                blendShapeData.Name = blendShapeChannel->GetName();
                const auto dotPos = blendShapeData.Name.Find('.');
                if (dotPos != -1)
                    blendShapeData.Name = blendShapeData.Name.Substring(dotPos + 1);
                blendShapeData.Weight = blendShapeChannel->GetTargetShapeCount() > 1 ? (float)(blendShapeChannel->DeformPercent.Get() / 100.0) : 1.0f;

                FbxVector4* shapeControlPoints = shape->GetControlPoints();
                blendShapeData.Vertices.Resize(shapeControlPointsCount);
                for (int32 i = 0; i < blendShapeData.Vertices.Count(); i++)
                    blendShapeData.Vertices[i].VertexIndex = i;
                for (int i = 0; i < blendShapeData.Vertices.Count(); i++)
                    blendShapeData.Vertices[i].PositionDelta = ToFlaxType(shapeControlPoints[i] - controlPoints[i]);
                // TODO: support importing normals from blend shape
                for (int32 i = 0; i < blendShapeData.Vertices.Count(); i++)
                    blendShapeData.Vertices[i].NormalDelta = Float3::Zero;
            }
        }
    }

    // Flip the Y in texcoords
    for (int32 i = 0; i < mesh.UVs.Count(); i++)
        mesh.UVs[i].Y = 1.0f - mesh.UVs[i].Y;
    for (int32 i = 0; i < mesh.LightmapUVs.Count(); i++)
        mesh.LightmapUVs[i].Y = 1.0f - mesh.LightmapUVs[i].Y;

    // Handle missing material case (could never happen but it's better to be sure it will work)
    if (mesh.MaterialSlotIndex == -1)
    {
        mesh.MaterialSlotIndex = 0;
        LOG(Warning, "Mesh \'{0}\' has missing material slot.", mesh.Name);
    }

    return false;
}

bool ImportMesh(ImporterData& data, int32 nodeIndex, FbxMesh* fbxMesh, String& errorMsg)
{
    auto& model = data.Model;

    // Skip invalid meshes
    if (!fbxMesh->IsTriangleMesh() || fbxMesh->GetControlPointsCount() == 0 || fbxMesh->GetPolygonCount() == 0)
        return false;

    // Check if that mesh has been already imported (instanced geometry)
    MeshData* meshData = nullptr;
    if (data.Meshes.TryGet(fbxMesh, meshData) && meshData)
    {
        // Clone mesh
        meshData = New<MeshData>(*meshData);
    }
    else
    {
        // Import mesh data
        meshData = New<MeshData>();
        if (ProcessMesh(data, fbxMesh, *meshData, errorMsg))
            return true;
    }

    // Link mesh
    meshData->NodeIndex = nodeIndex;
    auto& node = data.Nodes[nodeIndex];
    const auto lodIndex = node.LodIndex;
    if (model.LODs.Count() <= lodIndex)
        model.LODs.Resize(lodIndex + 1);
    model.LODs[lodIndex].Meshes.Add(meshData);

    return false;
}

bool ImportMesh(ImporterData& data, int32 nodeIndex, String& errorMsg)
{
    auto fbxNode = data.Nodes[nodeIndex].FbxNode;

    // Process the node's attributes
    for (int i = 0; i < fbxNode->GetNodeAttributeCount(); i++)
    {
        auto attribute = fbxNode->GetNodeAttributeByIndex(i);
        if (!attribute)
            continue;

        switch (attribute->GetAttributeType())
        {
        case FbxNodeAttribute::eNurbs:
        case FbxNodeAttribute::eNurbsSurface:
        case FbxNodeAttribute::ePatch:
        {
            FbxGeometryConverter geomConverter(FbxSdkManager::Manager);
            attribute = geomConverter.Triangulate(attribute, true);

            if (attribute->GetAttributeType() == FbxNodeAttribute::eMesh)
            {
                FbxMesh* mesh = static_cast<FbxMesh*>(attribute);
                mesh->RemoveBadPolygons();

                if (ImportMesh(data, nodeIndex, mesh, errorMsg))
                    return true;
            }
        }
            break;
        case FbxNodeAttribute::eMesh:
        {
            FbxMesh* mesh = static_cast<FbxMesh*>(attribute);
            mesh->RemoveBadPolygons();

            if (!mesh->IsTriangleMesh())
            {
                FbxGeometryConverter geomConverter(FbxSdkManager::Manager);
                geomConverter.Triangulate(mesh, true);
                attribute = fbxNode->GetNodeAttribute();
                mesh = static_cast<FbxMesh*>(attribute);
            }

            if (ImportMesh(data, nodeIndex, mesh, errorMsg))
                return true;
        }
        break;
        default:
            break;
        }
    }

    return false;
}

bool ImportMeshes(ImporterData& data, String& errorMsg)
{
    for (int32 i = 0; i < data.Nodes.Count(); i++)
    {
        if (ImportMesh(data, i, errorMsg))
            return true;
    }

    return false;
}

/*
void ImportCurve(aiVectorKey* keys, uint32 keysCount, LinearCurve<Float3>& curve)
{
	if (keys == nullptr || keysCount == 0)
		return;

	const auto keyframes = curve.Resize(keysCount);

	for (uint32 i = 0; i < keysCount; i++)
	{
		auto& aKey = keys[i];
		auto& key = keyframes[i];

		key.Time = (float)aKey.mTime;
		key.Value = ToFlaxType(aKey.mValue);
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
*/

/// <summary>
/// Bakes the node transformations.
/// </summary>
/// <remarks>
/// FBX stores transforms in a more complex way than just translation-rotation-scale as used by Flax Engine.
/// Instead they also support rotations offsets and pivots, scaling pivots and more. We wish to bake all this data
/// into a standard transform so we can access it using node's local TRS properties (e.g. FbxNode::LclTranslation).
/// </remarks>
/// <param name="scene">The FBX scene.</param>
void BakeTransforms(FbxScene* scene)
{
    double frameRate = FbxTime::GetFrameRate(scene->GetGlobalSettings().GetTimeMode());

    Array<FbxNode*> todo;
    todo.Push(scene->GetRootNode());

    while (todo.HasItems())
    {
        FbxNode* node = todo.Pop();

        FbxVector4 zero(0, 0, 0);
        FbxVector4 one(1, 1, 1);

        // Activate pivot converting
        node->SetPivotState(FbxNode::eSourcePivot, FbxNode::ePivotActive);
        node->SetPivotState(FbxNode::eDestinationPivot, FbxNode::ePivotActive);

        // We want to set all these to 0 (1 for scale) and bake them into the transforms
        node->SetPostRotation(FbxNode::eDestinationPivot, zero);
        node->SetPreRotation(FbxNode::eDestinationPivot, zero);
        node->SetRotationOffset(FbxNode::eDestinationPivot, zero);
        node->SetScalingOffset(FbxNode::eDestinationPivot, zero);
        node->SetRotationPivot(FbxNode::eDestinationPivot, zero);
        node->SetScalingPivot(FbxNode::eDestinationPivot, zero);

        // We account for geometric properties separately during node traversal
        node->SetGeometricTranslation(FbxNode::eDestinationPivot, node->GetGeometricTranslation(FbxNode::eSourcePivot));
        node->SetGeometricRotation(FbxNode::eDestinationPivot, node->GetGeometricRotation(FbxNode::eSourcePivot));
        node->SetGeometricScaling(FbxNode::eDestinationPivot, node->GetGeometricScaling(FbxNode::eSourcePivot));

        // Flax assumes euler angles are in YXZ order
        node->SetRotationOrder(FbxNode::eDestinationPivot, FbxEuler::eOrderYXZ);

        // Keep interpolation as is
        node->SetQuaternionInterpolation(FbxNode::eDestinationPivot, node->GetQuaternionInterpolation(FbxNode::eSourcePivot));

        for (int i = 0; i < node->GetChildCount(); i++)
        {
            FbxNode* childNode = node->GetChild(i);
            todo.Push(childNode);
        }
    }

    scene->GetRootNode()->ConvertPivotAnimationRecursive(nullptr, FbxNode::eDestinationPivot, frameRate, false);
}

bool ModelTool::ImportDataAutodeskFbxSdk(const String& path, ImportedModelData& data, Options& options, String& errorMsg)
{
    ScopeLock lock(FbxSdkManager::Locker);

    // Initialize
    FbxSdkManager::Init();
    auto scene = FbxScene::Create(FbxSdkManager::Manager, "Scene");
    if (scene == nullptr)
    {
        errorMsg = TEXT("Failed to create FBX scene");
        return false;
    }

    // Import file
    bool importMeshes = (data.Types & ImportDataTypes::Geometry) != 0;
    bool importAnimations = (data.Types & ImportDataTypes::Animations) != 0;
    FbxImporter* importer = FbxImporter::Create(FbxSdkManager::Manager, "");
    auto ios = FbxSdkManager::Manager->GetIOSettings();
    ios->SetBoolProp(IMP_FBX_MODEL, importMeshes);
    ios->SetBoolProp(IMP_FBX_ANIMATION, importAnimations);
    if (!importer->Initialize(StringAnsi(path), -1, ios))
    {
        errorMsg = String::Format(TEXT("Failed to initialize FBX importer. {0}"), String(importer->GetStatus().GetErrorString()));
        return false;
    }
    if (!importer->Import(scene))
    {
        errorMsg = String::Format(TEXT("Failed to import FBX scene. {0}"), String(importer->GetStatus().GetErrorString()));
        importer->Destroy();
        return false;
    }
    {
        const FbxAxisSystem fileCoordSystem = scene->GetGlobalSettings().GetAxisSystem();
        FbxAxisSystem bsCoordSystem(FbxAxisSystem::eDirectX);
        if (fileCoordSystem != bsCoordSystem)
            bsCoordSystem.ConvertScene(scene);
    }
    importer->Destroy();
    importer = nullptr;

    BakeTransforms(scene);

    // TODO: optimizeMeshes

    // Process imported scene nodes
    ImporterData importerData(data, options, scene);
    ProcessNodes(importerData, scene->GetRootNode(), -1);

    // Add all materials
    for (int i = 0; i < scene->GetMaterialCount(); i++)
    {
        importerData.Materials.Add(scene->GetMaterial(i));
    }

    // Import geometry (meshes and materials)
    if (data.Types & ImportDataTypes::Geometry)
    {
        if (ImportMeshes(importerData, errorMsg))
        {
            LOG(Warning, "Failed to import meshes.");
            return true;
        }
    }

    // TODO: remove unused materials if meshes merging is disabled

    // Import skeleton
    if (data.Types & ImportDataTypes::Skeleton)
    {
        data.Skeleton.Nodes.Resize(importerData.Nodes.Count(), false);
        for (int32 i = 0; i < importerData.Nodes.Count(); i++)
        {
            auto& node = data.Skeleton.Nodes[i];
            auto& fbxNode = importerData.Nodes[i];

            node.Name = fbxNode.Name;
            node.ParentIndex = fbxNode.ParentIndex;
            node.LocalTransform = fbxNode.LocalTransform;
        }

        data.Skeleton.Bones.Resize(importerData.Bones.Count(), false);
        for (int32 i = 0; i < importerData.Bones.Count(); i++)
        {
            auto& bone = data.Skeleton.Bones[i];
            auto& fbxBone = importerData.Bones[i];

            const auto boneNodeIndex = fbxBone.NodeIndex;
            const auto parentBoneNodeIndex = fbxBone.ParentBoneIndex == -1 ? -1 : importerData.Bones[fbxBone.ParentBoneIndex].NodeIndex;

            bone.ParentIndex = fbxBone.ParentBoneIndex;
            bone.NodeIndex = fbxBone.NodeIndex;
            bone.LocalTransform = CombineTransformsFromNodeIndices(importerData.Nodes, parentBoneNodeIndex, boneNodeIndex);
            bone.OffsetMatrix = fbxBone.OffsetMatrix;
        }
    }
    /*
    // Import animations
    if (data.Types & ImportDataTypes::Animations)
    {
        if (scene->HasAnimations())
        {
            const auto animations = scene->mAnimations[0];
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
    */
    // Import nodes
    if (data.Types & ImportDataTypes::Nodes)
    {
        data.Nodes.Resize(importerData.Nodes.Count());
        for (int32 i = 0; i < importerData.Nodes.Count(); i++)
        {
            auto& node = data.Nodes[i];
            auto& aNode = importerData.Nodes[i];

            node.Name = aNode.Name;
            node.ParentIndex = aNode.ParentIndex;
            node.LocalTransform = aNode.LocalTransform;
        }
    }

    // Export materials info
    const int32 materialsCount = importerData.Materials.Count();
    data.Materials.Resize(materialsCount, false);
    for (int32 i = 0; i < importerData.Materials.Count(); i++)
    {
        auto& material = data.Materials[i];
        const auto fbxMaterial = importerData.Materials[i];

        material.Name = String(fbxMaterial->GetName()).TrimTrailing();
        material.MaterialID = Guid::Empty;
    }

    scene->Clear();

    return false;
}

#endif
