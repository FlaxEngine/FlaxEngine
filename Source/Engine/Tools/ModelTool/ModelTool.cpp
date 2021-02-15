// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#if COMPILE_WITH_MODEL_TOOL

#include "ModelTool.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Types/DateTime.h"
#include "Engine/Core/Types/Pair.h"
#include "Engine/Graphics/Models/SkeletonUpdater.h"
#include "Engine/Graphics/Models/SkeletonMapping.h"
#include "Engine/Core/Utilities.h"
#include "Engine/Core/Types/StringView.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Utilities/RectPack.h"
#include "Engine/Tools/TextureTool/TextureTool.h"
#include "Engine/ContentImporters/AssetsImportingManager.h"
#include "Engine/ContentImporters/CreateMaterial.h"
#include "Editor/Utilities/EditorUtilities.h"
#include <ThirdParty/meshoptimizer/meshoptimizer.h>

void RemoveNamespace(String& name)
{
    const int32 namespaceStart = name.Find(':');
    if (namespaceStart != -1)
        name = name.Substring(namespaceStart + 1);
}

bool ModelTool::ImportData(const String& path, ImportedModelData& data, Options options, String& errorMsg)
{
    // Validate options
    options.Scale = Math::Clamp(options.Scale, 0.0001f, 100000.0f);
    options.SmoothingNormalsAngle = Math::Clamp(options.SmoothingNormalsAngle, 0.0f, 175.0f);
    options.SmoothingTangentsAngle = Math::Clamp(options.SmoothingTangentsAngle, 0.0f, 45.0f);
    options.FramesRange.Y = Math::Max(options.FramesRange.Y, options.FramesRange.X);
    options.DefaultFrameRate = Math::Max(0.0f, options.DefaultFrameRate);
    options.SamplingRate = Math::Max(0.0f, options.SamplingRate);

    // Validate path
    // Note: Assimp/Autodesk supports only ANSI characters in imported file path
    StringAnsi importPath;
    String tmpPath;
    if (path.IsANSI() == false)
    {
        // Use temporary file
        LOG(Warning, "Model Tool doesn't support importing files from paths using non ASNI characters. Using temporary file.");
        FileSystem::GetTempFilePath(tmpPath);
        if (tmpPath.IsANSI() == false || FileSystem::CopyFile(tmpPath, path))
        {
            errorMsg = TEXT("Path with non ANSI characters is invalid.");
            return true;
        }
        importPath = tmpPath.ToStringAnsi();
    }
    else
    {
        importPath = path.ToStringAnsi();
    }

    // Call importing backend
#if (USE_AUTODESK_FBX_SDK || USE_OPEN_FBX) && USE_ASSIMP
    if (path.EndsWith(TEXT(".fbx"), StringSearchCase::IgnoreCase))
    {
#if USE_AUTODESK_FBX_SDK
        if (ImportDataAutodeskFbxSdk(importPath.Get(), data, options, errorMsg))
            return true;
#elif USE_OPEN_FBX
        if (ImportDataOpenFBX(importPath.Get(), data, options, errorMsg))
            return true;
#endif
    }
    else
    {
        if (ImportDataAssimp(importPath.Get(), data, options, errorMsg))
            return true;
    }
#elif USE_ASSIMP
    if (ImportDataAssimp(importPath.Get(), data, options, errorMsg))
        return true;
#elif USE_AUTODESK_FBX_SDK
	if (ImportDataAutodeskFbxSdk(importPath.Get(), data, options, errorMsg))
		return true;
#elif USE_OPEN_FBX
	if (ImportDataOpenFBX(importPath.Get(), data, options, errorMsg))
		return true;
#else
#error Cannot use Model Tool without any importing backend!
#endif

    // Remove temporary file
    if (tmpPath.HasChars() && FileSystem::FileExists(tmpPath))
    {
        FileSystem::DeleteFile(tmpPath);
    }

    // TODO: check model LODs sequence (eg. {LOD0, LOD2, LOD5} is invalid)

    // Remove namespace prefixes from the nodes names
    {
        for (auto& node : data.Nodes)
        {
            RemoveNamespace(node.Name);
        }
        for (auto& node : data.Skeleton.Nodes)
        {
            RemoveNamespace(node.Name);
        }
        for (auto& channel : data.Animation.Channels)
        {
            RemoveNamespace(channel.NodeName);
        }
        for (auto& lod : data.LODs)
        {
            for (auto& mesh : lod.Meshes)
            {
                RemoveNamespace(mesh->Name);
                for (auto& blendShape : mesh->BlendShapes)
                {
                    RemoveNamespace(blendShape.Name);
                }
            }
        }
    }

    // Validate the animation channels
    if (data.Animation.Channels.HasItems())
    {
        // Validate bone animations uniqueness
        auto& channels = data.Animation.Channels;
        for (int32 i = 0; i < channels.Count(); i++)
        {
            for (int32 j = i + 1; j < channels.Count(); j++)
            {
                if (channels[i].NodeName == channels[j].NodeName)
                {
                    LOG(Warning, "Animation uses two nodes with the same name ({0}). Removing duplicated channel.", channels[i].NodeName);
                    channels.RemoveAtKeepOrder(j);
                    j--;
                }
            }
        }

        // Remove channels/animations with empty tracks
        if (options.SkipEmptyCurves)
        {
            for (int32 i = 0; i < channels.Count(); i++)
            {
                auto& channel = channels[i];

                // Remove identity curves (with single keyframe and no actual animated change)
                if (channel.Position.GetKeyframes().Count() == 1 && channel.Position.GetKeyframes()[0].Value.IsZero())
                {
                    channel.Position.Clear();
                }
                if (channel.Rotation.GetKeyframes().Count() == 1 && channel.Rotation.GetKeyframes()[0].Value.IsIdentity())
                {
                    channel.Rotation.Clear();
                }
                if (channel.Scale.GetKeyframes().Count() == 1 && channel.Scale.GetKeyframes()[0].Value.IsOne())
                {
                    channel.Scale.Clear();
                }

                // Remove whole channel if has no effective data
                if (channel.Position.IsEmpty() && channel.Rotation.IsEmpty() && channel.Scale.IsEmpty())
                {
                    LOG(Warning, "Removing empty animation channel ({0}).", channel.NodeName);
                    channels.RemoveAtKeepOrder(i);
                }
            }
        }
    }

    // Flip normals of the imported geometry
    if (options.FlipNormals && data.Types & ImportDataTypes::Geometry)
    {
        for (auto& lod : data.LODs)
        {
            for (auto& mesh : lod.Meshes)
            {
                for (auto& n : mesh->Normals)
                    n *= -1;
            }
        }
    }

    return false;
}

// Disabled by default (not finished and Assimp importer outputs nodes in a fine order)
#define USE_SKELETON_NODES_SORTING 0

#if USE_SKELETON_NODES_SORTING

bool SortDepths(const Pair<int32, int32>& a, const Pair<int32, int32>& b)
{
	return a.First < b.First;
}

void CreateLinearListFromTree(Array<SkeletonNode>& nodes, Array<int32>& mapping)
{
    // Customized breadth first tree algorithm (each node has no direct reference to the children so we build the cache for the nodes depth level)
	const int32 count = nodes.Count();
	Array<Pair<int32, int32>> depths(count); // Pair.First = Depth, Pair.Second = Node Index
	depths.SetSize(count);
	depths.Set(-1);
	for (int32 i = 0; i < count; i++)
	{
		// Skip evaluated nodes
		if (depths[i].First != -1)
			continue;

		// Find the first node with calculated depth and get the distance to it
		int32 end = i;
		int32 lastDepth;
		int32 relativeDepth = 0;
		do
		{
			lastDepth = depths[end].First;
			end = nodes[end].ParentIndex;
			relativeDepth++;
		} while (end != -1 && lastDepth == -1);

		// Set the depth (second item is the node index)
		depths[i] = MakePair(lastDepth + relativeDepth, i);
	}
	for (int32 i = 0; i < count; i++)
	{
		// Strange divide by 2 but works
		depths[i].First = depths[i].First >> 1;
	}

	// Order nodes by depth O(n*log(n))
	depths.Sort(SortDepths);

	// Extract nodes mapping O(n^2)
	mapping.EnsureCapacity(count, false);
	mapping.SetSize(count);
	for (int32 i = 0; i < count; i++)
	{
		int32 newIndex = -1;
		for (int32 j = 0; j < count; j++)
		{
			if (depths[j].Second == i)
			{
				newIndex = j;
				break;
			}
		}
		ASSERT(newIndex != -1);
		mapping[i] = newIndex;
	}
}

#endif

template<typename T>
void OptimizeCurve(LinearCurve<T>& curve)
{
    auto& oldKeyframes = curve.GetKeyframes();
    const int32 keyCount = oldKeyframes.Count();
    LinearCurve<T>::KeyFrameCollection newKeyframes(keyCount);
    bool lastWasEqual = false;

    for (int32 i = 0; i < keyCount; i++)
    {
        bool isEqual = false;
        const auto& curKey = oldKeyframes[i];
        if (i > 0)
        {
            const auto& prevKey = newKeyframes.Last();
            isEqual = Math::NearEqual(prevKey.Value, curKey.Value);
        }

        // More than two keys in a row are equal, remove the middle key by replacing it with this one
        if (lastWasEqual && isEqual)
        {
            auto& prevKey = newKeyframes.Last();
            prevKey = curKey;
            continue;
        }

        newKeyframes.Add(curKey);
        lastWasEqual = isEqual;
    }

    // Special case if animation has only two the same keyframes after cleaning
    if (newKeyframes.Count() == 2 && Math::NearEqual(newKeyframes[0].Value, newKeyframes[1].Value))
    {
        newKeyframes.RemoveAt(1);
    }

    // Special case if animation has only one identity keyframe (does not introduce any animation)
    if (newKeyframes.Count() == 1 && Math::NearEqual(newKeyframes[0].Value, curve.GetDefaultValue()))
    {
        newKeyframes.RemoveAt(0);
    }

    // Update keyframes if size changed
    if (keyCount != newKeyframes.Count())
    {
        curve.SetKeyframes(newKeyframes);
    }
}

void* MeshOptAllocate(size_t size)
{
    return Allocator::Allocate(size);
}

void MeshOptDeallocate(void* ptr)
{
    Allocator::Free(ptr);
}

bool ModelTool::ImportModel(const String& path, ModelData& meshData, Options options, String& errorMsg, const String& autoImportOutput)
{
    LOG(Info, "Importing model from \'{0}\'", path);
    const auto startTime = DateTime::NowUTC();

    // Import data
    ImportDataTypes importDataTypes;
    switch (options.Type)
    {
    case ModelType::Model:
        importDataTypes = ImportDataTypes::Geometry | ImportDataTypes::Nodes | ImportDataTypes::Textures;
        if (options.ImportMaterials)
            importDataTypes |= ImportDataTypes::Materials;
        if (options.ImportTextures)
            importDataTypes |= ImportDataTypes::Textures;
        break;
    case ModelType::SkinnedModel:
        importDataTypes = ImportDataTypes::Geometry | ImportDataTypes::Nodes | ImportDataTypes::Skeleton;
        if (options.ImportMaterials)
            importDataTypes |= ImportDataTypes::Materials;
        if (options.ImportTextures)
            importDataTypes |= ImportDataTypes::Textures;
        break;
    case ModelType::Animation:
        importDataTypes = ImportDataTypes::Animations;
        break;
    default:
        return true;
    }
    ImportedModelData data(importDataTypes);
    if (ImportData(path, data, options, errorMsg))
        return true;

    // Validate result data
    switch (options.Type)
    {
    case ModelType::Model:
    {
        // Validate
        if (data.LODs.IsEmpty() || data.LODs[0].Meshes.IsEmpty())
        {
            errorMsg = TEXT("Imported model has no valid geometry.");
            return true;
        }

        LOG(Info, "Imported model has {0} LODs, {1} meshes (in LOD0) and {2} materials", data.LODs.Count(), data.LODs[0].Meshes.Count(), data.Materials.Count());
        break;
    }
    case ModelType::SkinnedModel:
    {
        // Special case if imported model has no bones but has valid skeleton and meshes.
        // We assume that every mesh uses a single bone. Copy nodes to bones.
        if (data.Skeleton.Bones.IsEmpty() && Math::IsInRange(data.Skeleton.Nodes.Count(), 1, MAX_BONES_PER_MODEL))
        {
            data.Skeleton.Bones.Resize(data.Skeleton.Nodes.Count());
            for (int32 i = 0; i < data.Skeleton.Nodes.Count(); i++)
            {
                auto& node = data.Skeleton.Nodes[i];
                auto& bone = data.Skeleton.Bones[i];

                bone.ParentIndex = node.ParentIndex;
                bone.NodeIndex = i;
                bone.LocalTransform = node.LocalTransform;

                Matrix t = Matrix::Identity;
                int32 idx = bone.NodeIndex;
                do
                {
                    t *= data.Skeleton.Nodes[idx].LocalTransform.GetWorld();
                    idx = data.Skeleton.Nodes[idx].ParentIndex;
                } while (idx != -1);
                t.Invert();
                bone.OffsetMatrix = t;
            }
        }

        // Validate
        if (data.LODs.IsEmpty() || data.LODs[0].Meshes.IsEmpty())
        {
            errorMsg = TEXT("Imported model has no valid geometry.");
            return true;
        }
        if (data.Skeleton.Nodes.IsEmpty() || data.Skeleton.Bones.IsEmpty())
        {
            errorMsg = TEXT("Imported model has no skeleton.");
            return true;
        }
        if (data.Skeleton.Bones.Count() > MAX_BONES_PER_MODEL)
        {
            errorMsg = String::Format(TEXT("Imported model skeleton has too many bones. Imported: {0}, maximum supported: {1}. Please optimize your asset."), data.Skeleton.Bones.Count(), MAX_BONES_PER_MODEL);
            return true;
        }
        if (data.LODs.Count() > 1)
        {
            LOG(Warning, "Imported skinned model has more than one LOD. Removing the lower LODs. Only single one is supported.");
            data.LODs.Resize(1);
        }
        for (int32 i = 0; i < data.LODs[0].Meshes.Count(); i++)
        {
            const auto mesh = data.LODs[0].Meshes[i];
            if (mesh->BlendIndices.IsEmpty() || mesh->BlendWeights.IsEmpty())
            {
                LOG(Warning, "Imported mesh \'{0}\' has missing skinning data. It may result in invalid rendering.", mesh->Name);

                auto indices = Int4::Zero;
                auto weights = Vector4::UnitX;

                // Check if use a single bone for skinning
                auto nodeIndex = data.Skeleton.FindNode(mesh->Name);
                auto boneIndex = data.Skeleton.FindBone(nodeIndex);
                if (boneIndex != -1)
                {
                    LOG(Warning, "Using auto-detected bone {0} (index {1})", data.Skeleton.Nodes[nodeIndex].Name, boneIndex);
                    indices.X = boneIndex;
                }

                mesh->BlendIndices.Resize(mesh->Positions.Count());
                mesh->BlendWeights.Resize(mesh->Positions.Count());
                mesh->BlendIndices.SetAll(indices);
                mesh->BlendWeights.SetAll(weights);
            }
#if BUILD_DEBUG
            else
            {
                auto& indices = mesh->BlendIndices;
                for (int32 j = 0; j < indices.Count(); j++)
                {
                    const int32 min = indices[j].MinValue();
                    const int32 max = indices[j].MaxValue();
                    if (min < 0 || max >= data.Skeleton.Bones.Count())
                    {
                        LOG(Warning, "Imported mesh \'{0}\' has invalid blend indices. It may result in invalid rendering.", mesh->Name);
                    }
                }

                auto& weights = mesh->BlendWeights;
                for (int32 j = 0; j < weights.Count(); j++)
                {
                    const float sum = weights[j].SumValues();
                    if (Math::Abs(sum - 1.0f) > ZeroTolerance)
                    {
                        LOG(Warning, "Imported mesh \'{0}\' has invalid blend weights. It may result in invalid rendering.", mesh->Name);
                    }
                }
            }
#endif
        }

        LOG(Info, "Imported skeleton has {0} bones, {3} nodes, {1} meshes and {2} material", data.Skeleton.Bones.Count(), data.LODs[0].Meshes.Count(), data.Materials.Count(), data.Nodes.Count());
        break;
    }
    case ModelType::Animation:
    {
        // Validate
        if (data.Animation.Channels.IsEmpty())
        {
            errorMsg = TEXT("Imported file has no valid animations.");
            return true;
        }

        LOG(Info, "Imported animation has {0} channels, duration: {1} frames, frames per second: {2}", data.Animation.Channels.Count(), data.Animation.Duration, data.Animation.FramesPerSecond);
        break;
    }
    }

    // Prepare textures
    Array<String> importedFileNames;
    for (int32 i = 0; i < data.Textures.Count(); i++)
    {
        auto& texture = data.Textures[i];

        // Auto-import textures
        if (autoImportOutput.IsEmpty() || (data.Types & ImportDataTypes::Textures) == 0 || texture.FilePath.IsEmpty())
            continue;
        auto filename = StringUtils::GetFileNameWithoutExtension(texture.FilePath);
        for (int32 j = filename.Length() - 1; j >= 0; j--)
        {
            if (EditorUtilities::IsInvalidPathChar(filename[j]))
                filename[j] = ' ';
        }
        if (importedFileNames.Contains(filename))
        {
            int32 counter = 1;
            do
            {
                filename = StringUtils::GetFileNameWithoutExtension(texture.FilePath) + TEXT(" ") + StringUtils::ToString(counter);
                counter++;
            } while (importedFileNames.Contains(filename));
        }
        importedFileNames.Add(filename);
        auto assetPath = autoImportOutput / filename + ASSET_FILES_EXTENSION_WITH_DOT;
        TextureTool::Options textureOptions;
        switch (texture.Type)
        {
        case TextureEntry::TypeHint::ColorRGB:
            textureOptions.Type = TextureFormatType::ColorRGB;
            break;
        case TextureEntry::TypeHint::ColorRGBA:
            textureOptions.Type = TextureFormatType::ColorRGBA;
            break;
        case TextureEntry::TypeHint::Normals:
            textureOptions.Type = TextureFormatType::NormalMap;
            break;
        default: ;
        }
        AssetsImportingManager::ImportIfEdited(texture.FilePath, assetPath, texture.AssetID, &textureOptions);
    }

    // Prepare material
    for (int32 i = 0; i < data.Materials.Count(); i++)
    {
        auto& material = data.Materials[i];

        if (material.Name.IsEmpty())
            material.Name = TEXT("Material ") + StringUtils::ToString(i);

        // Auto-import materials
        if (autoImportOutput.IsEmpty() || (data.Types & ImportDataTypes::Materials) == 0 || !material.UsesProperties())
            continue;
        auto filename = material.Name;
        for (int32 j = filename.Length() - 1; j >= 0; j--)
        {
            if (EditorUtilities::IsInvalidPathChar(filename[j]))
                filename[j] = ' ';
        }
        if (importedFileNames.Contains(filename))
        {
            int32 counter = 1;
            do
            {
                filename = material.Name + TEXT(" ") + StringUtils::ToString(counter);
                counter++;
            } while (importedFileNames.Contains(filename));
        }
        importedFileNames.Add(filename);
        auto assetPath = autoImportOutput / filename + ASSET_FILES_EXTENSION_WITH_DOT;
        CreateMaterial::Options materialOptions;
        materialOptions.Diffuse.Color = material.Diffuse.Color;
        if (material.Diffuse.TextureIndex != -1)
            materialOptions.Diffuse.Texture = data.Textures[material.Diffuse.TextureIndex].AssetID;
        materialOptions.Diffuse.HasAlphaMask = material.Diffuse.HasAlphaMask;
        materialOptions.Emissive.Color = material.Emissive.Color;
        if (material.Emissive.TextureIndex != -1)
            materialOptions.Emissive.Texture = data.Textures[material.Emissive.TextureIndex].AssetID;
        materialOptions.Opacity.Value = material.Opacity.Value;
        if (material.Opacity.TextureIndex != -1)
            materialOptions.Opacity.Texture = data.Textures[material.Opacity.TextureIndex].AssetID;
        if (material.Normals.TextureIndex != -1)
            materialOptions.Normals.Texture = data.Textures[material.Normals.TextureIndex].AssetID;
        if (material.TwoSided | material.Diffuse.HasAlphaMask)
            materialOptions.Info.CullMode = CullMode::TwoSided;
        if (!Math::IsOne(material.Opacity.Value) || material.Opacity.TextureIndex != -1)
            materialOptions.Info.BlendMode = MaterialBlendMode::Transparent;
        AssetsImportingManager::Create(AssetsImportingManager::CreateMaterialTag, assetPath, material.AssetID, &materialOptions);
    }

    // Prepare import transformation
    Transform importTransform(options.Translation, options.Rotation, Vector3(options.Scale));
    if (options.CenterGeometry && data.LODs.HasItems() && data.LODs[0].Meshes.HasItems())
    {
        // Calculate the bounding box (use LOD0 as a reference)
        BoundingBox box = data.LODs[0].GetBox();
        importTransform.Translation -= box.GetCenter();
    }
    const bool applyImportTransform = !importTransform.IsIdentity();

    // Post-process imported data based on a target asset type
    if (options.Type == ModelType::Model)
    {
        if (data.Nodes.IsEmpty())
        {
            errorMsg = TEXT("Missing model nodes.");
            return true;
        }

        // Apply the import transformation
        if (applyImportTransform)
        {
            // Transform the root node using the import transformation
            auto& root = data.Nodes[0];
            root.LocalTransform = importTransform.LocalToWorld(root.LocalTransform);
        }

        // Perform simple nodes mapping to single node (will transform meshes to model local space)
        SkeletonMapping<ImportedModelData::Node> skeletonMapping(data.Nodes, nullptr);

        // Refresh skeleton updater with model skeleton
        SkeletonUpdater<ImportedModelData::Node> hierarchyUpdater(data.Nodes);
        hierarchyUpdater.UpdateMatrices();

        // Move meshes in the new nodes
        for (int32 lodIndex = 0; lodIndex < data.LODs.Count(); lodIndex++)
        {
            for (int32 meshIndex = 0; meshIndex < data.LODs[lodIndex].Meshes.Count(); meshIndex++)
            {
                auto& mesh = *data.LODs[lodIndex].Meshes[meshIndex];

                // Check if there was a remap using model skeleton
                if (skeletonMapping.SourceToSource[mesh.NodeIndex] != mesh.NodeIndex)
                {
                    // Transform vertices
                    const auto transformationMatrix = hierarchyUpdater.CombineMatricesFromNodeIndices(skeletonMapping.SourceToSource[mesh.NodeIndex], mesh.NodeIndex);
                    if (!transformationMatrix.IsIdentity())
                        mesh.TransformBuffer(transformationMatrix);
                }

                // Update new node index using real asset skeleton
                mesh.NodeIndex = skeletonMapping.SourceToTarget[mesh.NodeIndex];
            }
        }

        // For generated lightmap UVs coordinates needs to be moved so all meshes are in unique locations in [0-1]x[0-1] coordinates space
        if (options.LightmapUVsSource == ModelLightmapUVsSource::Generate && data.LODs.HasItems() && data.LODs[0].Meshes.Count() > 1)
        {
            // Use weight-based coordinates space placement and rect-pack to allocate more space for bigger meshes in the model lightmap chart
            int32 lodIndex = 0;
            auto& lod = data.LODs[lodIndex];

            // Build list of meshes with their area
            struct LightmapUVsPack : RectPack<LightmapUVsPack, float>
            {
                LightmapUVsPack(float x, float y, float width, float height)
                    : RectPack<LightmapUVsPack, float>(x, y, width, height)
                {
                }

                void OnInsert()
                {
                }
            };
            struct MeshEntry
            {
                MeshData* Mesh;
                float Area;
                float Size;
                LightmapUVsPack* Slot;
            };
            Array<MeshEntry> entries;
            entries.Resize(lod.Meshes.Count());
            float areaSum = 0;
            for (int32 meshIndex = 0; meshIndex < lod.Meshes.Count(); meshIndex++)
            {
                auto& entry = entries[meshIndex];
                entry.Mesh = lod.Meshes[meshIndex];
                entry.Area = entry.Mesh->CalculateTrianglesArea();
                entry.Size = Math::Sqrt(entry.Area);
                areaSum += entry.Area;
            }

            if (areaSum > ZeroTolerance)
            {
                // Pack all surfaces into atlas
                float atlasSize = Math::Sqrt(areaSum) * 1.02f;
                int32 triesLeft = 10;
                while (triesLeft--)
                {
                    bool failed = false;
                    const float chartsPadding = (4.0f / 256.0f) * atlasSize;
                    LightmapUVsPack root(chartsPadding, chartsPadding, atlasSize - chartsPadding, atlasSize - chartsPadding);
                    for (auto& entry : entries)
                    {
                        entry.Slot = root.Insert(entry.Size, entry.Size, chartsPadding);
                        if (entry.Slot == nullptr)
                        {
                            // Failed to insert surface, increase atlas size and try again
                            atlasSize *= 1.5f;
                            failed = true;
                            break;
                        }
                    }

                    if (!failed)
                    {
                        // Transform meshes lightmap UVs into the slots in the whole atlas
                        const float atlasSizeInv = 1.0f / atlasSize;
                        for (const auto& entry : entries)
                        {
                            Vector2 uvOffset(entry.Slot->X * atlasSizeInv, entry.Slot->Y * atlasSizeInv);
                            Vector2 uvScale((entry.Slot->Width - chartsPadding) * atlasSizeInv, (entry.Slot->Height - chartsPadding) * atlasSizeInv);
                            // TODO: SIMD
                            for (auto& uv : entry.Mesh->LightmapUVs)
                            {
                                uv = uv * uvScale + uvOffset;
                            }
                        }
                        break;
                    }
                }
            }
        }
    }
    else if (options.Type == ModelType::SkinnedModel)
    {
        // Process blend shapes
        for (auto& lod : data.LODs)
        {
            for (auto& mesh : lod.Meshes)
            {
                for (int32 blendShapeIndex = mesh->BlendShapes.Count() - 1; blendShapeIndex >= 0; blendShapeIndex--)
                {
                    auto& blendShape = mesh->BlendShapes[blendShapeIndex];

                    // Remove blend shape vertices with empty deltas
                    for (int32 i = blendShape.Vertices.Count() - 1; i >= 0; i--)
                    {
                        auto& v = blendShape.Vertices[i];
                        if (v.PositionDelta.IsZero() && v.NormalDelta.IsZero())
                        {
                            blendShape.Vertices.RemoveAt(i);
                        }
                    }

                    // Remove empty blend shapes
                    if (blendShape.Vertices.IsEmpty() || blendShape.Name.IsEmpty())
                    {
                        LOG(Info, "Removing empty blend shape '{0}' from mesh '{1}'", blendShape.Name, mesh->Name);
                        mesh->BlendShapes.RemoveAt(blendShapeIndex);
                    }
                }
            }
        }

        // Ensure that root node is at index 0
        int32 rootIndex = -1;
        for (int32 i = 0; i < data.Skeleton.Nodes.Count(); i++)
        {
            const auto idx = data.Skeleton.Nodes[i].ParentIndex;
            if (idx == -1 && rootIndex == -1)
            {
                // Found root
                rootIndex = i;
            }
            else if (idx == -1)
            {
                // Found multiple roots
                errorMsg = TEXT("Imported skeleton has more than one root node.");
                return true;
            }
        }
        if (rootIndex == -1)
        {
            // Missing root node (more additional validation that possible error)
            errorMsg = TEXT("Imported skeleton has missing root node.");
            return true;
        }
        if (rootIndex != 0)
        {
            // Map the root node to index 0 (more optimized for runtime)
            LOG(Warning, "Imported skeleton root node is not at index 0. Performing the remmaping.");
            const int32 prevRootIndex = rootIndex;
            rootIndex = 0;
            Swap(data.Skeleton.Nodes[rootIndex], data.Skeleton.Nodes[prevRootIndex]);
            for (int32 i = 0; i < data.Skeleton.Nodes.Count(); i++)
            {
                auto& node = data.Skeleton.Nodes[i];
                if (node.ParentIndex == prevRootIndex)
                    node.ParentIndex = rootIndex;
                else if (node.ParentIndex == rootIndex)
                    node.ParentIndex = prevRootIndex;
            }
            for (int32 i = 0; i < data.Skeleton.Bones.Count(); i++)
            {
                auto& bone = data.Skeleton.Bones[i];
                if (bone.NodeIndex == prevRootIndex)
                    bone.NodeIndex = rootIndex;
                else if (bone.NodeIndex == rootIndex)
                    bone.NodeIndex = prevRootIndex;
            }
        }

#if BUILD_DEBUG
        // Validate that nodes and bones hierarchies are valid (no cyclic references because its mean to be a tree)
        {
            for (int32 i = 0; i < data.Skeleton.Nodes.Count(); i++)
            {
                int32 j = i;
                int32 testsLeft = data.Skeleton.Nodes.Count();
                do
                {
                    j = data.Skeleton.Nodes[j].ParentIndex;
                } while (j != -1 && testsLeft-- > 0);
                if (testsLeft <= 0)
                {
                    Platform::Fatal(TEXT("Skeleton importer issue!"));
                }
            }
            for (int32 i = 0; i < data.Skeleton.Bones.Count(); i++)
            {
                int32 j = i;
                int32 testsLeft = data.Skeleton.Bones.Count();
                do
                {
                    j = data.Skeleton.Bones[j].ParentIndex;
                } while (j != -1 && testsLeft-- > 0);
                if (testsLeft <= 0)
                {
                    Platform::Fatal(TEXT("Skeleton importer issue!"));
                }
            }
            for (int32 i = 0; i < data.Skeleton.Bones.Count(); i++)
            {
                if (data.Skeleton.Bones[i].NodeIndex == -1)
                {
                    Platform::Fatal(TEXT("Skeleton importer issue!"));
                }
            }
        }
#endif

        // Apply the import transformation
        if (applyImportTransform)
        {
            // Transform the root node using the import transformation
            auto& root = data.Skeleton.RootNode();
            root.LocalTransform = importTransform.LocalToWorld(root.LocalTransform);

            // Apply import transform on meshes
            Matrix importTransformMatrix;
            importTransform.GetWorld(importTransformMatrix);
            for (int32 lodIndex = 0; lodIndex < data.LODs.Count(); lodIndex++)
            {
                for (int32 meshIndex = 0; meshIndex < data.LODs[lodIndex].Meshes.Count(); meshIndex++)
                {
                    data.LODs[lodIndex].Meshes[meshIndex]->TransformBuffer(importTransformMatrix);
                }
            }

            // Apply import transform on root bones
            for (int32 i = 0; i < data.Skeleton.Bones.Count(); i++)
            {
                auto& bone = data.Skeleton.Bones[i];
                if (bone.ParentIndex == -1)
                {
                    bone.LocalTransform = importTransform.LocalToWorld(bone.LocalTransform);
                }
            }
        }

        // Perform simple nodes mapping to single node (will transform meshes to model local space)
        SkeletonMapping<ImportedModelData::Node> skeletonMapping(data.Nodes, nullptr);

        // Refresh skeleton updater with model skeleton
        SkeletonUpdater<ImportedModelData::Node> hierarchyUpdater(data.Nodes);
        hierarchyUpdater.UpdateMatrices();

        // Move meshes in the new nodes
        for (int32 lodIndex = 0; lodIndex < data.LODs.Count(); lodIndex++)
        {
            for (int32 meshIndex = 0; meshIndex < data.LODs[lodIndex].Meshes.Count(); meshIndex++)
            {
                auto& mesh = *data.LODs[lodIndex].Meshes[meshIndex];

                // Check if there was a remap using model skeleton
                if (skeletonMapping.SourceToSource[mesh.NodeIndex] != mesh.NodeIndex)
                {
                    // Transform vertices
                    const auto transformationMatrix = hierarchyUpdater.CombineMatricesFromNodeIndices(skeletonMapping.SourceToSource[mesh.NodeIndex], mesh.NodeIndex);
                    if (!transformationMatrix.IsIdentity())
                        mesh.TransformBuffer(transformationMatrix);
                }

                // Update new node index using real asset skeleton
                mesh.NodeIndex = skeletonMapping.SourceToTarget[mesh.NodeIndex];
            }
        }

        // TODO: allow to link skeleton asset to model to retarget model bones skeleton for an animation
        // use SkeletonMapping<SkeletonBone> to map bones?

        // Calculate offset matrix (inverse bind pose transform) for every bone manually
        {
            for (int32 i = 0; i < data.Skeleton.Bones.Count(); i++)
            {
                Matrix t = Matrix::Identity;
                int32 idx = data.Skeleton.Bones[i].NodeIndex;
                do
                {
                    t *= data.Skeleton.Nodes[idx].LocalTransform.GetWorld();
                    idx = data.Skeleton.Nodes[idx].ParentIndex;
                } while (idx != -1);
                t.Invert();
                data.Skeleton.Bones[i].OffsetMatrix = t;
            }
        }

#if USE_SKELETON_NODES_SORTING
        // Sort skeleton nodes and bones hierarchy (parents first)
		// Then it can be used with a simple linear loop update
		{
			const int32 nodesCount = data.Skeleton.Nodes.Count();
			const int32 bonesCount = data.Skeleton.Bones.Count();
			Array<int32> mapping;
			CreateLinearListFromTree(data.Skeleton.Nodes, mapping);
			for (int32 i = 0; i < nodesCount; i++)
			{
				auto& node = data.Skeleton.Nodes[i];
				node.ParentIndex = mapping[node.ParentIndex];
			}
			for (int32 i = 0; i < bonesCount; i++)
			{
				auto& bone = data.Skeleton.Bones[i];
				bone.NodeIndex = mapping[bone.NodeIndex];
			}
		}
		reorder_nodes_and_test_it_out!
#endif

        // TODO: remove nodes that don't have bone attached and all child nodes don' have any bones
    }
    else if (options.Type == ModelType::Animation)
    {
        // Trim the animation keyframes range if need to
        if (options.Duration == AnimationDuration::Custom)
        {
            const float start = (float)(options.FramesRange.X / data.Animation.FramesPerSecond);
            const float end = (float)(options.FramesRange.Y / data.Animation.FramesPerSecond);
            for (int32 i = 0; i < data.Animation.Channels.Count(); i++)
            {
                auto& anim = data.Animation.Channels[i];

                anim.Position.Trim(start, end);
                anim.Rotation.Trim(start, end);
                anim.Scale.Trim(start, end);
            }
            data.Animation.Duration = (end - start) * data.Animation.FramesPerSecond;
        }

        // Change the sampling rate if need to
        if (!Math::IsZero(options.SamplingRate))
        {
            const float timeScale = (float)(data.Animation.FramesPerSecond / options.SamplingRate);
            if (!Math::IsOne(timeScale))
            {
                data.Animation.FramesPerSecond = options.SamplingRate;
                for (int32 i = 0; i < data.Animation.Channels.Count(); i++)
                {
                    auto& anim = data.Animation.Channels[i];

                    anim.Position.TransformTime(timeScale, 0.0f);
                    anim.Rotation.TransformTime(timeScale, 0.0f);
                    anim.Scale.TransformTime(timeScale, 0.0f);
                }
            }
        }

        // Optimize the keyframes
        if (options.OptimizeKeyframes)
        {
            const int32 before = data.Animation.GetKeyframesCount();
            for (int32 i = 0; i < data.Animation.Channels.Count(); i++)
            {
                auto& anim = data.Animation.Channels[i];

                // Optimize keyframes
                OptimizeCurve(anim.Position);
                OptimizeCurve(anim.Rotation);
                OptimizeCurve(anim.Scale);

                // Remove empty channels
                if (anim.GetKeyframesCount() == 0)
                {
                    data.Animation.Channels.RemoveAt(i--);
                }
            }
            const int32 after = data.Animation.GetKeyframesCount();
            LOG(Info, "Optimized {0} animation keyframe(s). Before: {1}, after: {2}, Ratio: {3}%", before - after, before, after, Utilities::RoundTo2DecimalPlaces((float)after / before));
        }

        data.Animation.EnableRootMotion = options.EnableRootMotion;
        data.Animation.RootNodeName = options.RootNodeName;
    }

    // Merge meshes with the same parent nodes, material and skinning
    if (options.MergeMeshes)
    {
        int32 meshesMerged = 0;

        for (int32 lodIndex = 0; lodIndex < data.LODs.Count(); lodIndex++)
        {
            auto& meshes = data.LODs[lodIndex].Meshes;

            // Group meshes that can be merged together
            typedef Pair<int32, int32> MeshGroupKey;
            const std::function<MeshGroupKey(MeshData* const&)> f = [](MeshData* const& x) -> MeshGroupKey
            {
                return MeshGroupKey(x->NodeIndex, x->MaterialSlotIndex);
            };
            Array<IGrouping<MeshGroupKey, MeshData*>> meshesByGroup;
            ArrayExtensions::GroupBy(meshes, f, meshesByGroup);

            for (int32 groupIndex = 0; groupIndex < meshesByGroup.Count(); groupIndex++)
            {
                auto& group = meshesByGroup[groupIndex];
                if (group.Count() <= 1)
                    continue;

                // Merge group meshes with the first one
                auto targetMesh = group[0];
                for (int32 i = 1; i < group.Count(); i++)
                {
                    auto mesh = group[i];
                    meshes.Remove(mesh);
                    targetMesh->Merge(*mesh);
                    meshesMerged++;
                    Delete(mesh);
                }
            }
        }

        if (meshesMerged)
        {
            LOG(Info, "Merged {0} meshes", meshesMerged);
        }
    }

    // Automatic LOD generation
    if (options.GenerateLODs && data.LODs.HasItems() && options.TriangleReduction < 1.0f - ZeroTolerance)
    {
        auto lodStartTime = DateTime::NowUTC();
        meshopt_setAllocator(MeshOptAllocate, MeshOptDeallocate);
        float triangleReduction = Math::Saturate(options.TriangleReduction);
        int32 lodCount = Math::Max(options.LODCount, data.LODs.Count());
        int32 baseLOD = Math::Clamp(options.BaseLOD, 0, lodCount - 1);
        data.LODs.Resize(lodCount);
        int32 generatedLod = 0, baseLodTriangleCount = 0, baseLodVertexCount = 0;
        for (auto& mesh : data.LODs[baseLOD].Meshes)
        {
            baseLodTriangleCount += mesh->Indices.Count() / 3;
            baseLodVertexCount += mesh->Positions.Count();
        }
        for (int32 lodIndex = Math::Clamp(baseLOD + 1, 1, lodCount - 1); lodIndex < lodCount; lodIndex++)
        {
            auto& dstLod = data.LODs[lodIndex];
            const auto& srcLod = data.LODs[lodIndex - 1];

            int32 lodTriangleCount = 0, lodVertexCount = 0;
            dstLod.Meshes.Resize(srcLod.Meshes.Count());
            for (int32 meshIndex = 0; meshIndex < dstLod.Meshes.Count(); meshIndex++)
            {
                auto& dstMesh = dstLod.Meshes[meshIndex] = New<MeshData>();
                const auto& srcMesh = srcLod.Meshes[meshIndex];

                // Setup mesh
                dstMesh->MaterialSlotIndex = srcMesh->MaterialSlotIndex;
                dstMesh->NodeIndex = srcMesh->NodeIndex;
                dstMesh->Name = srcMesh->Name;

                // Simplify mesh using meshoptimizer
                int32 srcMeshIndexCount = srcMesh->Indices.Count();
                int32 srcMeshVertexCount = srcMesh->Positions.Count();
                int32 dstMeshIndexCountTarget = int32(srcMeshIndexCount * triangleReduction) / 3 * 3;
                Array<unsigned int> indices;
                indices.Resize(dstMeshIndexCountTarget);
                int32 dstMeshIndexCount = (int32)meshopt_simplifySloppy(indices.Get(), srcMesh->Indices.Get(), srcMeshIndexCount, (const float*)srcMesh->Positions.Get(), srcMeshVertexCount, sizeof(Vector3), dstMeshIndexCountTarget);
                indices.Resize(dstMeshIndexCount);
                if (dstMeshIndexCount == 0)
                    continue;

                // Generate simplified vertex buffer remapping table (use only vertices from LOD index buffer)
                Array<unsigned int> remap;
                remap.Resize(srcMeshVertexCount);
                int32 dstMeshVertexCount = (int32)meshopt_optimizeVertexFetchRemap(remap.Get(), indices.Get(), dstMeshIndexCount, srcMeshVertexCount);

                // Remap index buffer
                dstMesh->Indices.Resize(dstMeshIndexCount);
                meshopt_remapIndexBuffer(dstMesh->Indices.Get(), indices.Get(), dstMeshIndexCount, remap.Get());

                // Remap vertex buffer
#define REMAP_VERTEX_BUFFER(name, type) \
    if (srcMesh->name.HasItems()) \
    { \
        ASSERT(srcMesh->name.Count() == srcMeshVertexCount); \
        dstMesh->name.Resize(dstMeshVertexCount); \
        meshopt_remapVertexBuffer(dstMesh->name.Get(), srcMesh->name.Get(), srcMeshVertexCount, sizeof(type), remap.Get()); \
    }
                REMAP_VERTEX_BUFFER(Positions, Vector3);
                REMAP_VERTEX_BUFFER(UVs, Vector2);
                REMAP_VERTEX_BUFFER(Normals, Vector3);
                REMAP_VERTEX_BUFFER(Tangents, Vector3);
                REMAP_VERTEX_BUFFER(Tangents, Vector3);
                REMAP_VERTEX_BUFFER(LightmapUVs, Vector2);
                REMAP_VERTEX_BUFFER(Colors, Color);
                REMAP_VERTEX_BUFFER(BlendIndices, Int4);
                REMAP_VERTEX_BUFFER(BlendWeights, Vector4);
#undef REMAP_VERTEX_BUFFER

                // Remap blend shapes
                dstMesh->BlendShapes.Resize(srcMesh->BlendShapes.Count());
                for (int32 blendShapeIndex = 0; blendShapeIndex < srcMesh->BlendShapes.Count(); blendShapeIndex++)
                {
                    const auto& srcBlendShape = srcMesh->BlendShapes[blendShapeIndex];
                    auto& dstBlendShape = dstMesh->BlendShapes[blendShapeIndex];

                    dstBlendShape.Name = srcBlendShape.Name;
                    dstBlendShape.Weight = srcBlendShape.Weight;
                    dstBlendShape.Vertices.EnsureCapacity(srcBlendShape.Vertices.Count());
                    for (int32 i = 0; i < srcBlendShape.Vertices.Count(); i++)
                    {
                        auto v = srcBlendShape.Vertices[i];
                        v.VertexIndex = remap[v.VertexIndex];
                        if (v.VertexIndex != ~0u)
                        {
                            dstBlendShape.Vertices.Add(v);
                        }
                    }
                }

                // Remove empty blend shapes
                for (int32 blendShapeIndex = dstMesh->BlendShapes.Count() - 1; blendShapeIndex >= 0; blendShapeIndex--)
                {
                    if (dstMesh->BlendShapes[blendShapeIndex].Vertices.IsEmpty())
                        dstMesh->BlendShapes.RemoveAt(blendShapeIndex);
                }

                // Optimize generated LOD
                meshopt_optimizeVertexCache(dstMesh->Indices.Get(), dstMesh->Indices.Get(), dstMeshIndexCount, dstMeshVertexCount);
                meshopt_optimizeOverdraw(dstMesh->Indices.Get(), dstMesh->Indices.Get(), dstMeshIndexCount, (const float*)dstMesh->Positions.Get(), dstMeshVertexCount, sizeof(Vector3), 1.05f);

                lodTriangleCount += dstMeshIndexCount / 3;
                lodVertexCount += dstMeshVertexCount;
                generatedLod++;
            }

            // Remove empty meshes
            for (int32 i = dstLod.Meshes.Count() - 1; i >= 0; i--)
            {
                if (dstLod.Meshes[i]->Indices.IsEmpty())
                    dstLod.Meshes.RemoveAt(i--);
            }

            LOG(Info, "Generated LOD{0}: triangles: {1} ({2}% of base LOD), verticies: {3} ({4}% of base LOD)",
                lodIndex,
                lodTriangleCount, (int32)(lodTriangleCount * 100 / baseLodTriangleCount),
                lodVertexCount, (int32)(lodVertexCount * 100 / baseLodVertexCount));
        }
        if (generatedLod)
        {
            auto lodEndTime = DateTime::NowUTC();
            LOG(Info, "Generated LODs for {1} meshes in {0} ms", static_cast<int32>((lodEndTime - lodStartTime).GetTotalMilliseconds()), generatedLod);
        }
    }

    // Export imported data to the output container (we reduce vertex data copy operations to minimum)
    {
        meshData.Textures.Swap(data.Textures);
        meshData.Materials.Swap(data.Materials);
        meshData.LODs.Resize(data.LODs.Count(), false);
        for (int32 i = 0; i < data.LODs.Count(); i++)
        {
            auto& dst = meshData.LODs[i];
            auto& src = data.LODs[i];

            dst.Meshes = src.Meshes;
        }
        meshData.Skeleton.Swap(data.Skeleton);
        meshData.Animation.Swap(data.Animation);

        // Clear meshes from imported data (we link them to result model data). This reduces amount of allocations.
        data.LODs.Resize(0);
    }

    // Calculate blend shapes vertices ranges
    for (auto& lod : meshData.LODs)
    {
        for (auto& mesh : lod.Meshes)
        {
            for (auto& blendShape : mesh->BlendShapes)
            {
                // Compute min/max for used vertex indices
                blendShape.MinVertexIndex = MAX_uint32;
                blendShape.MaxVertexIndex = 0;
                blendShape.UseNormals = false;
                for (int32 i = 0; i < blendShape.Vertices.Count(); i++)
                {
                    auto& v = blendShape.Vertices[i];
                    blendShape.MinVertexIndex = Math::Min(blendShape.MinVertexIndex, v.VertexIndex);
                    blendShape.MaxVertexIndex = Math::Max(blendShape.MaxVertexIndex, v.VertexIndex);
                    blendShape.UseNormals |= !v.NormalDelta.IsZero();
                }
            }
        }
    }

    const auto endTime = DateTime::NowUTC();
    LOG(Info, "Model file imported in {0} ms", static_cast<int32>((endTime - startTime).GetTotalMilliseconds()));

    return false;
}

int32 ModelTool::DetectLodIndex(const String& nodeName)
{
    const int32 index = nodeName.FindLast(TEXT("LOD"));
    if (index != -1)
    {
        int32 num;
        if (!StringUtils::Parse(nodeName.Get() + index + 3, &num))
        {
            if (num >= 0 && num < MODEL_MAX_LODS)
            {
                return num;
            }

            LOG(Warning, "Invalid mesh level of detail index at node \'{0}\'. Maximum supported amount of LODs is {1}.", nodeName, MODEL_MAX_LODS);
        }
    }

    return 0;
}

bool ModelTool::FindTexture(const String& sourcePath, const String& file, String& path)
{
    const String sourceFolder = StringUtils::GetDirectoryName(sourcePath);
    path = sourceFolder / file;
    if (!FileSystem::FileExists(path))
    {
        const String filename = StringUtils::GetFileName(file);
        path = sourceFolder / filename;
        if (!FileSystem::FileExists(path))
        {
            path = sourceFolder / TEXT("textures") / filename;
            if (!FileSystem::FileExists(path))
            {
                path = sourceFolder / TEXT("Textures") / filename;
                if (!FileSystem::FileExists(path))
                {
                    path = sourceFolder / TEXT("texture") / filename;
                    if (!FileSystem::FileExists(path))
                    {
                        path = sourceFolder / TEXT("Texture") / filename;
                        if (!FileSystem::FileExists(path))
                        {
                            path = sourceFolder / TEXT("../textures") / filename;
                            if (!FileSystem::FileExists(path))
                            {
                                path = sourceFolder / TEXT("../Textures") / filename;
                                if (!FileSystem::FileExists(path))
                                {
                                    path = sourceFolder / TEXT("../texture") / filename;
                                    if (!FileSystem::FileExists(path))
                                    {
                                        path = sourceFolder / TEXT("../Texture") / filename;
                                        if (!FileSystem::FileExists(path))
                                        {
                                            return true;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    FileSystem::NormalizePath(path);
    return false;
}

#endif
