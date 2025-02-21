// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#if COMPILE_WITH_MODEL_TOOL && USE_OPEN_FBX

#include "ModelTool.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Mathd.h"
#include "Engine/Core/Math/Matrix.h"
#include "Engine/Core/Math/Plane.h"
#include "Engine/Core/Collections/Sorting.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Tools/TextureTool/TextureTool.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Platform/File.h"

#define OPEN_FBX_CONVERT_SPACE 1
#if BUILD_DEBUG
#define OPEN_FBX_GET_CACHE_LIST(arrayName, varName, size) data.arrayName.Resize(size, false); auto& varName = data.arrayName
#else
#define OPEN_FBX_GET_CACHE_LIST(arrayName, varName, size) data.arrayName.Resize(size, false); auto* varName = data.arrayName.Get()
#endif

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

Matrix ToMatrix(const ofbx::DMatrix& mat)
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

    Array<int> TriangulatedIndicesCache;
    Array<Int4> BlendIndicesCache;
    Array<Float4> BlendWeightsCache;
    Array<Float2> TriangulatePointsCache;
    Array<int> TriangulateIndicesCache;
    Array<int> TriangulateEarIndicesCache;

    OpenFbxImporterData(const String& path, const ModelTool::Options& options, ofbx::IScene* scene)
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

    bool ImportMaterialTexture(ModelData& result, const ofbx::Material* mat, ofbx::Texture::TextureType textureType, int32& textureIndex, TextureEntry::TypeHint type) const
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

    int32 AddMaterial(ModelData& result, const ofbx::Material* mat)
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

            if (mat && EnumHasAnyFlags(Options.ImportTypes, ImportDataTypes::Materials))
            {
                material.Diffuse.Color = ToColor(mat->getDiffuseColor());
                material.Emissive.Color = ToColor(mat->getEmissiveColor()) * (float)mat->getEmissiveFactor();
                material.Roughness.Value = MaterialSlotEntry::ShininessToRoughness((float)mat->getShininess());

                if (EnumHasAnyFlags(Options.ImportTypes, ImportDataTypes::Textures))
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
    auto* skin = mesh ? mesh->getSkin() : nullptr;
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
    return aMesh->getGeometryData().getPositions().count == 0;
}

bool ImportBones(OpenFbxImporterData& data, String& errorMsg)
{
    // Check all meshes
    const int meshCount = data.Scene->getMeshCount();
    for (int i = 0; i < meshCount; i++)
    {
        const auto aMesh = data.Scene->getMesh(i);
        const ofbx::Skin* skin = aMesh->getSkin();
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

int Triangulate(OpenFbxImporterData& data, const ofbx::GeometryData& geom, const ofbx::GeometryPartition::Polygon& polygon, int* triangulatedIndices)
{
    if (polygon.vertex_count < 3)
        return 0;
    else if (polygon.vertex_count == 3)
    {
        triangulatedIndices[0] = polygon.from_vertex;
        triangulatedIndices[1] = polygon.from_vertex + 1;
        triangulatedIndices[2] = polygon.from_vertex + 2;
        return 3;
    }
    else if (polygon.vertex_count == 4)
    {
        triangulatedIndices[0] = polygon.from_vertex + 0;
        triangulatedIndices[1] = polygon.from_vertex + 1;
        triangulatedIndices[2] = polygon.from_vertex + 2;
        triangulatedIndices[3] = polygon.from_vertex + 0;
        triangulatedIndices[4] = polygon.from_vertex + 2;
        triangulatedIndices[5] = polygon.from_vertex + 3;
        return 6;
    }

    const ofbx::Vec3Attributes& positions = geom.getPositions();
    Float3 normal = ToFloat3(geom.getNormals().get(polygon.from_vertex));

    // Check if the polygon is convex
    int lastSign = 0;
    bool isConvex = true;
    for (int i = 0; i < polygon.vertex_count; i++)
    {
        Float3 v1 = ToFloat3(positions.get(polygon.from_vertex + i));
        Float3 v2 = ToFloat3(positions.get(polygon.from_vertex + (i + 1) % polygon.vertex_count));
        Float3 v3 = ToFloat3(positions.get(polygon.from_vertex + (i + 2) % polygon.vertex_count));

        // The winding order of all triangles must be same for polygon to be considered convex
        int sign;
        Float3 c = Float3::Cross(v1 - v2, v3 - v2);
        if (c.LengthSquared() == 0.0f)
            continue;
        else if (Math::NotSameSign(c.X, normal.X) || Math::NotSameSign(c.Y, normal.Y) || Math::NotSameSign(c.Z, normal.Z))
            sign = 1;
        else
            sign = -1;
        if ((sign < 0 && lastSign > 0) || (sign > 0 && lastSign < 0))
        {
            isConvex = false;
            break;
        }
        lastSign += sign;
    }

    // Fast-path for convex case
    if (isConvex)
    {
        for (int i = 0; i < polygon.vertex_count - 2; i++)
        {
            triangulatedIndices[i * 3 + 0] = polygon.from_vertex;
            triangulatedIndices[i * 3 + 1] = polygon.from_vertex + (i + 1) % polygon.vertex_count;
            triangulatedIndices[i * 3 + 2] = polygon.from_vertex + (i + 2) % polygon.vertex_count;
        }
        return 3 * (polygon.vertex_count - 2);
    }

    // Setup arrays for temporary data (TODO: maybe double-linked list is more optimal?)
    auto& points = data.TriangulatePointsCache;
    auto& indices = data.TriangulateIndicesCache;
    auto& earIndices = data.TriangulateEarIndicesCache;
    points.Clear();
    indices.Clear();
    earIndices.Clear();
    points.EnsureCapacity(polygon.vertex_count, false);
    indices.EnsureCapacity(polygon.vertex_count, false);
    earIndices.EnsureCapacity(3 * (polygon.vertex_count - 2), false);

    // Project points to a plane, choose two arbitrary axises
    const Float3 u = Float3::Cross(normal, Math::Abs(normal.X) > Math::Abs(normal.Y) ? Float3::Up : Float3::Right).GetNormalized();
    const Float3 v = Float3::Cross(normal, u).GetNormalized();
    for (int i = 0; i < polygon.vertex_count; i++)
    {
        const Float3 point = ToFloat3(positions.get(polygon.from_vertex + i));
        const Float3 projectedPoint = Float3::ProjectOnPlane(point, normal);
        const Float2 pointOnPlane = Float2(
            projectedPoint.X * u.X + projectedPoint.Y * u.Y + projectedPoint.Z * u.Z,
            projectedPoint.X * v.X + projectedPoint.Y * v.Y + projectedPoint.Z * v.Z);

        points.Add(pointOnPlane);
        indices.Add(i);
    }

    // Triangulate non-convex polygons using simple ear-clipping algorithm (https://nils-olovsson.se/articles/ear_clipping_triangulation/)
    const int maxIterations = indices.Count() * 10; // Safe guard to prevent infinite loop
    int index = 0;
    while (indices.Count() > 3 && index < maxIterations)
    {
        const int i1 = index % indices.Count();
        const int i2 = (index + 1) % indices.Count();
        const int i3 = (index + 2) % indices.Count();
        const Float2 p1 = points[indices[i1]];
        const Float2 p2 = points[indices[i2]];
        const Float2 p3 = points[indices[i3]];

        // TODO: Skip triangles with very sharp angles?

        // Skip reflex vertices
        if (Float2::Cross(p2 - p1, p3 - p1) < 0.0f)
        {
            index++;
            continue;
        }

        // The triangle is considered to be an "ear" when no other points reside inside the triangle
        bool isEar = true;
        for (int j = 0; j < indices.Count(); j++)
        {
            if (j == i1 || j == i2 || j == i3)
                continue;
            const Float2 candidate = points[indices[j]];
            if (CollisionsHelper::IsPointInTriangle(candidate, p1, p2, p3))
            {
                isEar = false;
                break;
            }
        }
        if (!isEar)
        {
            index++;
            continue;
        }

        // Add an ear and remove the tip point from evaluation
        earIndices.Add(indices[i1]);
        earIndices.Add(indices[i2]);
        earIndices.Add(indices[i3]);

		// Remove midpoint of the ear from the loop
        indices.RemoveAtKeepOrder(i2);
    }

    // Last ear
    earIndices.Add(indices[0]);
    earIndices.Add(indices[1]);
    earIndices.Add(indices[2]);
    
    // Write any degenerate triangles (eg. if points are duplicated within a list)
    for (int32 i = 3; i < indices.Count(); i += 3)
    {
        earIndices.Add(indices[i + 0]);
        earIndices.Add(indices[(i + 1) % indices.Count()]);
        earIndices.Add(indices[(i + 2) % indices.Count()]);
    }

    // Copy ears into triangles
    for (int32 i = 0; i < earIndices.Count(); i++)
        triangulatedIndices[i] = polygon.from_vertex + (earIndices[i] % polygon.vertex_count);

    // Ensure that we've written enough ears
    ASSERT(earIndices.Count() == 3 * (polygon.vertex_count - 2));
    return earIndices.Count();
}

bool ProcessMesh(ModelData& result, OpenFbxImporterData& data, const ofbx::Mesh* aMesh, MeshData& mesh, String& errorMsg, int partitionIndex)
{
    PROFILE_CPU();
    mesh.Name = aMesh->name;
    ZoneText(*mesh.Name, mesh.Name.Length());
    const ofbx::GeometryData& geometryData = aMesh->getGeometryData();
    const ofbx::GeometryPartition& partition = geometryData.getPartition(partitionIndex);
    const int vertexCount = partition.triangles_count * 3;
    const ofbx::Vec3Attributes& positions = geometryData.getPositions();
    const ofbx::Vec2Attributes& uvs = geometryData.getUVs();
    const ofbx::Vec3Attributes& normals = geometryData.getNormals();
    const ofbx::Vec3Attributes& tangents = geometryData.getTangents();
    const ofbx::Vec4Attributes& colors = geometryData.getColors();
    const ofbx::Skin* skin = aMesh->getSkin();
    const ofbx::BlendShape* blendShape = aMesh->getBlendShape();
    OPEN_FBX_GET_CACHE_LIST(TriangulatedIndicesCache, triangulatedIndices, vertexCount);

    // Properties
    const ofbx::Material* aMaterial = nullptr;
    if (aMesh->getMaterialCount() > 0)
        aMaterial = aMesh->getMaterial(partitionIndex);
    mesh.MaterialSlotIndex = data.AddMaterial(result, aMaterial);

    // Vertex positions
    mesh.Positions.Resize(vertexCount, false);
    {
        int numIndicesTotal = 0;
        for (int i = 0; i < partition.polygon_count; i++)
        {
            int numIndices = Triangulate(data, geometryData, partition.polygons[i], &triangulatedIndices[numIndicesTotal]);
            for (int j = numIndicesTotal; j < numIndicesTotal + numIndices; j++)
                mesh.Positions.Get()[j] = ToFloat3(positions.get(triangulatedIndices[j]));
            numIndicesTotal += numIndices;
        }
        ASSERT(numIndicesTotal == vertexCount);
    }

    // Indices (dummy index buffer)
    mesh.Indices.Resize(vertexCount, false);
    for (int i = 0; i < vertexCount; i++)
        mesh.Indices.Get()[i] = i;

    // Texture coordinates
    if (uvs.values)
    {
        mesh.UVs.Resize(vertexCount, false);
        for (int i = 0; i < vertexCount; i++)
            mesh.UVs.Get()[i] = ToFloat2(uvs.get(triangulatedIndices[i]));
        if (data.ConvertRH)
        {
            for (int32 v = 0; v < vertexCount; v++)
                mesh.UVs[v].Y = 1.0f - mesh.UVs[v].Y;
        }
    }

    // Normals
    if (data.Options.CalculateNormals || !normals.values)
    {
        if (mesh.GenerateNormals(data.Options.SmoothingNormalsAngle))
        {
            errorMsg = TEXT("Failed to generate normals.");
            return true;
        }
    }
    else if (normals.values)
    {
        mesh.Normals.Resize(vertexCount, false);
        for (int i = 0; i < vertexCount; i++)
            mesh.Normals.Get()[i] = ToFloat3(normals.get(triangulatedIndices[i]));
        if (data.ConvertRH)
        {
            // Mirror normals along the Z axis
            for (int32 i = 0; i < vertexCount; i++)
                mesh.Normals.Get()[i].Z *= -1.0f;
        }
    }

    // Tangents
    if ((data.Options.CalculateTangents || !tangents.values) && mesh.UVs.HasItems())
    {
        // Generated after full mesh data conversion
    }
    else if (tangents.values)
    {
        mesh.Tangents.Resize(vertexCount, false);
        for (int i = 0; i < vertexCount; i++)
            mesh.Tangents.Get()[i] = ToFloat3(tangents.get(triangulatedIndices[i]));
        if (data.ConvertRH)
        {
            // Mirror tangents along the Z axis
            for (int32 i = 0; i < vertexCount; i++)
                mesh.Tangents.Get()[i].Z *= -1.0f;
        }
    }

    // Reverse winding order
    if (data.Options.ReverseWindingOrder)
    {
        uint32* meshIndices = mesh.Indices.Get();
        Float3* meshPositions = mesh.Positions.Get();
        Float3* meshNormals = mesh.Normals.HasItems() ? mesh.Normals.Get() : nullptr;
        Float3* meshTangents = mesh.Tangents.HasItems() ? mesh.Tangents.Get() : nullptr;

        for (int i = 0; i < vertexCount; i += 3) {
            Swap(meshIndices[i + 1], meshIndices[i + 2]);
            Swap(meshPositions[i + 1], meshPositions[i + 2]);
            if (meshNormals)
                Swap(meshNormals[i + 1], meshNormals[i + 2]);
            if (meshTangents)
                Swap(meshTangents[i + 1], meshTangents[i + 2]);
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
        const auto lightmapUVs = geometryData.getUVs(inputChannelIndex);
        if (lightmapUVs.values)
        {
            mesh.LightmapUVs.Resize(vertexCount, false);
            for (int i = 0; i < vertexCount; i++)
                mesh.LightmapUVs.Get()[i] = ToFloat2(lightmapUVs.get(triangulatedIndices[i]));
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
    if (data.Options.ImportVertexColors && colors.values)
    {
        mesh.Colors.Resize(vertexCount, false);
        for (int i = 0; i < vertexCount; i++)
            mesh.Colors.Get()[i] = ToColor(colors.get(triangulatedIndices[i]));
    }

    // Blend Indices and Blend Weights
    if (skin && skin->getClusterCount() > 0 && EnumHasAnyFlags(data.Options.ImportTypes, ImportDataTypes::Skeleton))
    {
        OPEN_FBX_GET_CACHE_LIST(BlendIndicesCache, blendIndices, positions.values_count);
        OPEN_FBX_GET_CACHE_LIST(BlendWeightsCache, blendWeights, positions.values_count);
        data.BlendIndicesCache.SetAll(Int4::Zero);
        data.BlendWeightsCache.SetAll(Float4::Zero);

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
                int vtxIndex = clusterIndices[j];
                float vtxWeight = (float)clusterWeights[j];
                if (vtxWeight <= 0 || vtxIndex < 0 || vtxIndex >= positions.values_count)
                    continue;
                Int4& indices = blendIndices[vtxIndex];
                Float4& weights = blendWeights[vtxIndex];

                for (int32 k = 0; k < 4; k++)
                {
                    if (vtxWeight >= weights.Raw[k])
                    {
                        // Move lower weights by one down
                        for (int32 l = 2; l >= k; l--)
                        {
                            indices.Raw[l + 1] = indices.Raw[l];
                            weights.Raw[l + 1] = weights.Raw[l];
                        }

                        // Set bone influence
                        indices.Raw[k] = boneIndex;
                        weights.Raw[k] = vtxWeight;
                        break;
                    }
                }
            }
        }

        // Remap blend values to triangulated data
        mesh.BlendIndices.Resize(vertexCount, false);
        mesh.BlendWeights.Resize(vertexCount, false);
        for (int i = 0; i < vertexCount; i++)
        {
            const int idx = positions.indices[triangulatedIndices[i]];
            mesh.BlendIndices.Get()[i] = blendIndices[idx];
            mesh.BlendWeights.Get()[i] = blendWeights[idx];
        }

        mesh.NormalizeBlendWeights();
    }

    // Blend Shapes
    if (blendShape && blendShape->getBlendShapeChannelCount() > 0 && EnumHasAnyFlags(data.Options.ImportTypes, ImportDataTypes::Skeleton) && data.Options.ImportBlendShapes)
    {
        mesh.BlendShapes.EnsureCapacity(blendShape->getBlendShapeChannelCount());
        for (int32 channelIndex = 0; channelIndex < blendShape->getBlendShapeChannelCount(); channelIndex++)
        {
            const ofbx::BlendShapeChannel* channel = blendShape->getBlendShapeChannel(channelIndex);

            // Use the last shape
            const int targetShapeCount = channel->getShapeCount();
            if (targetShapeCount == 0)
                continue;
            const ofbx::Shape* shape = channel->getShape(targetShapeCount - 1);
            const ofbx::Vec3* shapeVertices = shape->getVertices();
            const ofbx::Vec3* shapeNormals = shape->getNormals();
            const int* shapeIndices = shape->getIndices();
            const int shapeVertexCount = shape->getVertexCount();
            const int shapeIndexCount = shape->getIndexCount();
            if (shapeVertexCount != shapeIndexCount)
            {
                LOG(Error, "Blend shape '{0}' in mesh '{1}' has different amount of vertices ({2}) and indices ({3})", String(shape->name), mesh.Name, shapeVertexCount, shapeIndexCount);
                continue;
            }

            BlendShape& blendShapeData = mesh.BlendShapes.AddOne();
            blendShapeData.Name = shape->name;
            blendShapeData.Weight = channel->getShapeCount() > 1 ? (float)(channel->getDeformPercent() / 100.0) : 1.0f;
            blendShapeData.Vertices.EnsureCapacity(shapeIndexCount);

            for (int32 i = 0; i < shapeIndexCount; i++)
            {
		        int shapeIndex = shapeIndices[i];
                BlendShapeVertex v;
                v.PositionDelta = ToFloat3(shapeVertices[i]);
                v.NormalDelta = shapeNormals ? ToFloat3(shapeNormals[i]) : Float3::Zero;
                for (int32 vertexIndex = 0; vertexIndex < vertexCount; vertexIndex++)
                {
                    int sourceIndex = positions.indices[triangulatedIndices[vertexIndex]];
                    if (sourceIndex == shapeIndex)
                    {
                        // Add blend shape vertex
                        v.VertexIndex = vertexIndex;
                        blendShapeData.Vertices.Add(v);
                    }
                }
            }
        }
    }

    if (data.ConvertRH)
    {
        // Mirror positions along the Z axis
        for (int32 i = 0; i < vertexCount; i++)
            mesh.Positions.Get()[i].Z *= -1.0f;
        for (auto& blendShapeData : mesh.BlendShapes)
        {
            for (auto& v : blendShapeData.Vertices)
            {
                v.PositionDelta.Z *= -1.0f;
                v.NormalDelta.Z *= -1.0f;
            }
        }
    }

    // Build solid index buffer (remove duplicated vertices)
    mesh.BuildIndexBuffer();

    if (data.ConvertRH)
    {
        // Invert the order
        for (int32 i = 0; i < mesh.Indices.Count(); i += 3)
            Swap(mesh.Indices.Get()[i], mesh.Indices.Get()[i + 2]);
    }

    if ((data.Options.CalculateTangents || !tangents.values) && mesh.UVs.HasItems())
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

bool ImportMesh(ModelData& result, OpenFbxImporterData& data, const ofbx::Mesh* aMesh, String& errorMsg, int partitionIndex)
{
    PROFILE_CPU();

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
    if (ProcessMesh(result, data, aMesh, *meshData, errorMsg, partitionIndex))
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

bool ImportMesh(int32 index, ModelData& result, OpenFbxImporterData& data, String& errorMsg)
{
    const auto aMesh = data.Scene->getMesh(index);
    if (IsMeshInvalid(aMesh))
        return false;

    const auto& geomData = aMesh->getGeometryData();
    for (int i = 0; i < geomData.getPartitionCount(); i++)
    {
        const auto& partition = geomData.getPartition(i);
        if (partition.polygon_count == 0)
            continue;

        if (ImportMesh(result, data, aMesh, errorMsg, i))
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
    ofbx::DVec3 Translation;
    ofbx::DVec3 Rotation;
    ofbx::DVec3 Scaling;
};

void ExtractKeyframePosition(const ofbx::Object* bone, ofbx::DVec3& trans, const Frame& localFrame, Float3& keyframe)
{
    const Matrix frameTrans = ToMatrix(bone->evalLocal(trans, localFrame.Rotation, localFrame.Scaling));
    keyframe = frameTrans.GetTranslation();
}

void ExtractKeyframeRotation(const ofbx::Object* bone, ofbx::DVec3& trans, const Frame& localFrame, Quaternion& keyframe)
{
    const Matrix frameTrans = ToMatrix(bone->evalLocal(localFrame.Translation, trans, { 1.0, 1.0, 1.0 }));
    Quaternion::RotationMatrix(frameTrans, keyframe);
}

void ExtractKeyframeScale(const ofbx::Object* bone, ofbx::DVec3& trans, const Frame& localFrame, Float3& keyframe)
{
    // Fix empty scale case
    if (Math::IsZero(trans.x) && Math::IsZero(trans.y) && Math::IsZero(trans.z))
        trans = { 1.0, 1.0, 1.0 };

    const Matrix frameTrans = ToMatrix(bone->evalLocal(localFrame.Translation, { 0.0, 0.0, 0.0 }, trans));
    keyframe = frameTrans.GetScaleVector();
}

template<typename T>
void ImportCurve(const ofbx::AnimationCurveNode* curveNode, LinearCurve<T>& curve, AnimInfo& info, void (*ExtractKeyframe)(const ofbx::Object*, ofbx::DVec3&, const Frame&, T&))
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

        ofbx::DVec3 trans = curveNode->getNodeLocalTransform(t);
        ExtractKeyframe(bone, trans, localFrame, key.Value);
    }
}

void ImportAnimation(int32 index, ModelData& data, OpenFbxImporterData& importerData)
{
    const ofbx::AnimationStack* stack = importerData.Scene->getAnimationStack(index);
    const ofbx::AnimationLayer* layer = stack->getLayer(0);
    const ofbx::TakeInfo* takeInfo = importerData.Scene->getTakeInfo(stack->name);
    if (takeInfo == nullptr)
        return;

    // Initialize animation animation keyframes sampling
    const float frameRate = importerData.FrameRate;
    const double localDuration = takeInfo->local_time_to - takeInfo->local_time_from;
    if (localDuration <= ZeroTolerance)
        return;

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
        return;

    // Setup animation descriptor
    auto& animation = data.Animations.AddOne();
    animation.Duration = (double)(int32)(localDuration * frameRate + 0.5f);
    animation.FramesPerSecond = frameRate;
    char nameData[256];
    takeInfo->name.toString(nameData);
    animation.Name = nameData;
    animation.Name = animation.Name.TrimTrailing();
    if (animation.Name.IsEmpty())
        animation.Name = String(layer->name);
    animation.Channels.Resize(animatedNodes.Count(), false);
    AnimInfo info;
    info.TimeStart = takeInfo->local_time_from;
    info.TimeEnd = takeInfo->local_time_to;
    info.Duration = localDuration;
    info.FramesCount = (int32)animation.Duration;
    info.SamplingPeriod = 1.0f / frameRate;

    // Import curves
    for (int32 i = 0; i < animatedNodes.Count(); i++)
    {
        const int32 nodeIndex = animatedNodes[i];
        auto& aNode = importerData.Nodes[nodeIndex];
        auto& anim = animation.Channels[i];

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
        for (auto& anim : animation.Channels)
        {
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

bool ModelTool::ImportDataOpenFBX(const String& path, ModelData& data, Options& options, String& errorMsg)
{
    // Import file
    Array<byte> fileData;
    if (File::ReadAllBytes(path, fileData))
    {
        errorMsg = TEXT("Cannot load file.");
        return true;
    }
    ofbx::LoadFlags loadFlags = ofbx::LoadFlags::NONE;
    if (EnumHasAnyFlags(options.ImportTypes, ImportDataTypes::Geometry))
    {
        if (!options.ImportBlendShapes)
            loadFlags |= ofbx::LoadFlags::IGNORE_BLEND_SHAPES;
    }
    else
    {
        loadFlags |= ofbx::LoadFlags::IGNORE_GEOMETRY | ofbx::LoadFlags::IGNORE_BLEND_SHAPES;
    }
    if (EnumHasNoneFlags(options.ImportTypes, ImportDataTypes::Materials))
        loadFlags |= ofbx::LoadFlags::IGNORE_MATERIALS;
    if (EnumHasNoneFlags(options.ImportTypes, ImportDataTypes::Textures))
        loadFlags |= ofbx::LoadFlags::IGNORE_TEXTURES;
    if (EnumHasNoneFlags(options.ImportTypes, ImportDataTypes::Animations))
        loadFlags |= ofbx::LoadFlags::IGNORE_ANIMATIONS;
    ofbx::IScene* scene;
    {
        PROFILE_CPU_NAMED("ofbx::load");
        scene = ofbx::load(fileData.Get(), fileData.Count(), (ofbx::u16)loadFlags);
    }
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
    OpenFbxImporterData context(path, options, scene);
    auto& globalSettings = context.GlobalSettings;
    ProcessNodes(context, scene->getRoot(), -1);

    // Apply model scene global scale factor
    context.Nodes[0].LocalTransform = Transform(Vector3::Zero, Quaternion::Identity, globalSettings.UnitScaleFactor) * context.Nodes[0].LocalTransform;

    // Log scene info
    LOG(Info, "Loaded FBX model, Frame Rate: {0}, Unit Scale Factor: {1}", context.FrameRate, globalSettings.UnitScaleFactor);
    LOG(Info, "{0}, {1}, {2}", String(globalInfo.AppName), String(globalInfo.AppVersion), String(globalInfo.AppVendor));
    LOG(Info, "Up: {1}{0}", globalSettings.UpAxis == ofbx::UpVector_AxisX ? TEXT("X") : globalSettings.UpAxis == ofbx::UpVector_AxisY ? TEXT("Y") : TEXT("Z"), globalSettings.UpAxisSign == 1 ? TEXT("+") : TEXT("-"));
    LOG(Info, "Front: {1}{0}", globalSettings.FrontAxis == ofbx::FrontVector_ParityEven ? TEXT("ParityEven") : TEXT("ParityOdd"), globalSettings.FrontAxisSign == 1 ? TEXT("+") : TEXT("-"));
    LOG(Info, "{0} Handed{1}", globalSettings.CoordAxis == ofbx::CoordSystem_RightHanded ? TEXT("Right") : TEXT("Left"), globalSettings.CoordAxisSign == 1 ? TEXT("") : TEXT(" (negative)"));
#if OPEN_FBX_CONVERT_SPACE
    LOG(Info, "Imported scene: Up={0}, Front={1}, Right={2}", context.Up, context.Front, context.Right);
#endif

    // Extract embedded textures
    if (EnumHasAnyFlags(options.ImportTypes, ImportDataTypes::Textures))
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
    if (context.Up == Float3(1, 0, 0) && context.Front == Float3(0, 0, 1) && context.Right == Float3(0, 1, 0))
    {
        context.RootConvertRotation = Quaternion::Euler(0, 180, 0);
    }
    else if (context.Up == Float3(0, 1, 0) && context.Front == Float3(-1, 0, 0) && context.Right == Float3(0, 0, 1))
    {
        context.RootConvertRotation = Quaternion::Euler(90, -90, 0);
    }
    /*Float3 engineUp(0, 1, 0);
    Float3 engineFront(0, 0, 1);
    Float3 engineRight(-1, 0, 0);*/
    /*Float3 engineUp(1, 0, 0);
    Float3 engineFront(0, 0, 1);
    Float3 engineRight(0, 1, 0);
    if (context.Up != engineUp || context.Front != engineFront || context.Right != engineRight)
    {
        LOG(Info, "Converting imported scene nodes to match engine coordinates system");
        context.RootConvertRotation = Quaternion::GetRotationFromTo(context.Up, engineUp, engineUp);
        //context.RootConvertRotation *= Quaternion::GetRotationFromTo(rotation * context.Right, engineRight, engineRight);
        //context.RootConvertRotation *= Quaternion::GetRotationFromTo(rotation * context.Front, engineFront, engineFront);
    }*/
    /*Float3 hackUp = FbxVectorFromAxisAndSign(globalSettings.UpAxis, globalSettings.UpAxisSign);
    if (hackUp == Float3::UnitX)
        context.RootConvertRotation = Quaternion::Euler(-90, 0, 0);
    else if (hackUp == Float3::UnitZ)
        context.RootConvertRotation = Quaternion::Euler(90, 0, 0);*/
    if (!context.RootConvertRotation.IsIdentity())
    {
        for (auto& node : context.Nodes)
        {
            if (node.ParentIndex == -1)
            {
                node.LocalTransform.Orientation = context.RootConvertRotation * node.LocalTransform.Orientation;
                break;
            }
        }
    }
#endif

    // Build final skeleton bones hierarchy before importing meshes
    if (EnumHasAnyFlags(options.ImportTypes, ImportDataTypes::Skeleton))
    {
        if (ImportBones(context, errorMsg))
        {
            LOG(Warning, "Failed to import skeleton bones.");
            return true;
        }
        Sorting::QuickSort(context.Bones);
    }

    // Import geometry (meshes and materials)
    if (EnumHasAnyFlags(options.ImportTypes, ImportDataTypes::Geometry) && context.Scene->getMeshCount() > 0)
    {
        const int meshCount = context.Scene->getMeshCount();
        for (int32 meshIndex = 0; meshIndex < meshCount; meshIndex++)
        {
            if (ImportMesh(meshIndex, data, context, errorMsg))
                return true;
        }
    }

    // Import skeleton
    if (EnumHasAnyFlags(options.ImportTypes, ImportDataTypes::Skeleton))
    {
        data.Skeleton.Nodes.Resize(context.Nodes.Count(), false);
        for (int32 i = 0; i < context.Nodes.Count(); i++)
        {
            auto& node = data.Skeleton.Nodes[i];
            auto& aNode = context.Nodes[i];

            node.Name = aNode.Name;
            node.ParentIndex = aNode.ParentIndex;
            node.LocalTransform = aNode.LocalTransform;
        }

        data.Skeleton.Bones.Resize(context.Bones.Count(), false);
        for (int32 i = 0; i < context.Bones.Count(); i++)
        {
            auto& bone = data.Skeleton.Bones[i];
            auto& aBone = context.Bones[i];
            const auto boneNodeIndex = aBone.NodeIndex;

            // Find the parent bone
            int32 parentBoneIndex = -1;
            for (int32 j = context.Nodes[boneNodeIndex].ParentIndex; j != -1; j = context.Nodes[j].ParentIndex)
            {
                parentBoneIndex = context.FindBone(j);
                if (parentBoneIndex != -1)
                    break;
            }
            aBone.ParentBoneIndex = parentBoneIndex;

            const auto parentBoneNodeIndex = aBone.ParentBoneIndex == -1 ? -1 : context.Bones[aBone.ParentBoneIndex].NodeIndex;

            bone.ParentIndex = aBone.ParentBoneIndex;
            bone.NodeIndex = aBone.NodeIndex;
            bone.LocalTransform = CombineTransformsFromNodeIndices(context.Nodes, parentBoneNodeIndex, boneNodeIndex);
            bone.OffsetMatrix = aBone.OffsetMatrix;
        }
    }

    // Import animations
    if (EnumHasAnyFlags(options.ImportTypes, ImportDataTypes::Animations))
    {
        const int animCount = context.Scene->getAnimationStackCount();
        for (int32 animIndex = 0; animIndex < animCount; animIndex++)
        {
            ImportAnimation(animIndex, data, context);
        }
    }

    // Import nodes
    if (EnumHasAnyFlags(options.ImportTypes, ImportDataTypes::Nodes))
    {
        data.Nodes.Resize(context.Nodes.Count());
        for (int32 i = 0; i < context.Nodes.Count(); i++)
        {
            auto& node = data.Nodes[i];
            auto& aNode = context.Nodes[i];

            node.Name = aNode.Name;
            node.ParentIndex = aNode.ParentIndex;
            node.LocalTransform = aNode.LocalTransform;
        }
    }

    return false;
}

#endif
