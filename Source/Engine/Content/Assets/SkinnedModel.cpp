// Copyright (c) Wojciech Figat. All rights reserved.

#include "SkinnedModel.h"
#include "Animation.h"
#include "Engine/Core/Log.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include "Engine/Streaming/StreamingGroup.h"
#include "Engine/Threading/Threading.h"
#include "Engine/Graphics/RenderTools.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/Models/Config.h"
#include "Engine/Graphics/Models/MeshDeformation.h"
#include "Engine/Graphics/Models/ModelInstanceEntry.h"
#include "Engine/Graphics/Shaders/GPUVertexLayout.h"
#include "Engine/Content/Content.h"
#include "Engine/Content/Factories/BinaryAssetFactory.h"
#include "Engine/Content/Upgraders/SkinnedModelAssetUpgrader.h"
#include "Engine/Debug/Exceptions/ArgumentOutOfRangeException.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Renderer/DrawCall.h"
#if USE_EDITOR
#include "Engine/Graphics/Models/ModelData.h"
#include "Engine/Serialization/MemoryWriteStream.h"
#endif

REGISTER_BINARY_ASSET_WITH_UPGRADER(SkinnedModel, "FlaxEngine.SkinnedModel", SkinnedModelAssetUpgrader, true);

SkinnedModel::SkinnedModel(const SpawnParams& params, const AssetInfo* info)
    : ModelBase(params, info, StreamingGroups::Instance()->SkinnedModels())
{
}

SkinnedModel::~SkinnedModel()
{
    ASSERT(_skeletonMappingCache.Count() == 0);
}

bool SkinnedModel::HasAnyLODInitialized() const
{
    return LODs.HasItems() && LODs.Last().HasAnyMeshInitialized();
}

Array<String> SkinnedModel::GetBlendShapes()
{
    Array<String> result;
    if (LODs.HasItems())
    {
        for (auto& mesh : LODs[0].Meshes)
        {
            for (auto& blendShape : mesh.BlendShapes)
            {
                if (!result.Contains(blendShape.Name))
                    result.Add(blendShape.Name);
            }
        }
    }
    return result;
}

SkinnedModel::SkeletonMapping SkinnedModel::GetSkeletonMapping(Asset* source, bool autoRetarget)
{
    SkeletonMapping mapping;
    mapping.TargetSkeleton = this;
    if (WaitForLoaded() || !source || source->WaitForLoaded())
        return mapping;
    ScopeLock lock(Locker);
    SkeletonMappingData mappingData;
    if (!_skeletonMappingCache.TryGet(source, mappingData))
    {
        PROFILE_CPU();

        // Initialize the mapping
        SkeletonRetarget* retarget = nullptr;
        const Guid sourceId = source->GetID();
        for (auto& e : _skeletonRetargets)
        {
            if (e.SourceAsset == sourceId)
            {
                retarget = &e;
                break;
            }
        }
        if (!retarget && !autoRetarget)
        {
            // Skip automatic retarget
            return mapping;
        }
        const int32 nodesCount = Skeleton.Nodes.Count();
        mappingData.NodesMapping = Span<int32>((int32*)Allocator::Allocate(nodesCount * sizeof(int32)), nodesCount);
        for (int32 i = 0; i < nodesCount; i++)
            mappingData.NodesMapping[i] = -1;
        if (const auto* sourceAnim = Cast<Animation>(source))
        {
            const auto& channels = sourceAnim->Data.Channels;
            if (retarget && retarget->SkeletonAsset)
            {
                // Map retarget skeleton nodes from animation channels
                if (auto* skeleton = Content::Load<SkinnedModel>(retarget->SkeletonAsset))
                {
                    const SkeletonMapping skeletonMapping = GetSkeletonMapping(skeleton);
                    mappingData.SourceSkeleton = skeleton;
                    if (skeletonMapping.NodesMapping.Length() == nodesCount)
                    {
                        const auto& nodes = skeleton->Skeleton.Nodes;
                        for (int32 j = 0; j < nodesCount; j++)
                        {
                            if (skeletonMapping.NodesMapping[j] != -1)
                            {
                                const Char* nodeName = nodes[skeletonMapping.NodesMapping[j]].Name.GetText();
                                for (int32 i = 0; i < channels.Count(); i++)
                                {
                                    if (StringUtils::CompareIgnoreCase(nodeName, channels[i].NodeName.GetText()) == 0)
                                    {
                                        mappingData.NodesMapping[j] = i;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
                else
                {
#if !BUILD_RELEASE
                    LOG(Error, "Missing asset {0} to use for skeleton mapping of {1}", retarget->SkeletonAsset, ToString());
#endif
                    return mapping;
                }
            }
            else
            {
                // Map animation channels to the skeleton nodes (by name)
                for (int32 i = 0; i < channels.Count(); i++)
                {
                    const NodeAnimationData& nodeAnim = channels[i];
                    for (int32 j = 0; j < nodesCount; j++)
                    {
                        if (StringUtils::CompareIgnoreCase(Skeleton.Nodes[j].Name.GetText(), nodeAnim.NodeName.GetText()) == 0)
                        {
                            mappingData.NodesMapping[j] = i;
                            break;
                        }
                    }
                }
            }
        }
        else if (const auto* sourceModel = Cast<SkinnedModel>(source))
        {
            if (retarget)
            {
                // Use nodes retargeting
                for (const auto& e : retarget->NodesMapping)
                {
                    const int32 dstIndex = Skeleton.FindNode(e.Key);
                    const int32 srcIndex = sourceModel->Skeleton.FindNode(e.Value);
                    if (dstIndex != -1 && srcIndex != -1)
                    {
                        mappingData.NodesMapping[dstIndex] = srcIndex;
                    }
                }
            }
            else
            {
                // Map source skeleton nodes to the target skeleton nodes (by name)
                const auto& nodes = sourceModel->Skeleton.Nodes;
                for (int32 i = 0; i < nodes.Count(); i++)
                {
                    const SkeletonNode& node = nodes[i];
                    for (int32 j = 0; j < nodesCount; j++)
                    {
                        if (StringUtils::CompareIgnoreCase(Skeleton.Nodes[j].Name.GetText(), node.Name.GetText()) == 0)
                        {
                            mappingData.NodesMapping[j] = i;
                            break;
                        }
                    }
                }
            }
        }
        else
        {
#if !BUILD_RELEASE
            LOG(Error, "Invalid asset type {0} to use for skeleton mapping of {1}", source->GetTypeName(), ToString());
#endif
        }

        // Add to cache
        _skeletonMappingCache.Add(source, mappingData);
        source->OnUnloaded.Bind<SkinnedModel, &SkinnedModel::OnSkeletonMappingSourceAssetUnloaded>(this);
#if USE_EDITOR
        source->OnReloading.Bind<SkinnedModel, &SkinnedModel::OnSkeletonMappingSourceAssetUnloaded>(this);
#endif
    }
    mapping.SourceSkeleton = mappingData.SourceSkeleton;
    mapping.NodesMapping = mappingData.NodesMapping;
    return mapping;
}

bool SkinnedModel::Intersects(const Ray& ray, const Matrix& world, Real& distance, Vector3& normal, SkinnedMesh** mesh, int32 lodIndex)
{
    if (LODs.Count() == 0)
        return false;
    return LODs[lodIndex].Intersects(ray, world, distance, normal, mesh);
}

bool SkinnedModel::Intersects(const Ray& ray, const Transform& transform, Real& distance, Vector3& normal, SkinnedMesh** mesh, int32 lodIndex)
{
    if (LODs.Count() == 0)
        return false;
    return LODs[lodIndex].Intersects(ray, transform, distance, normal, mesh);
}

BoundingBox SkinnedModel::GetBox(const Matrix& world, int32 lodIndex) const
{
    if (LODs.Count() == 0)
        return BoundingBox::Zero;
    return LODs[lodIndex].GetBox(world);
}

BoundingBox SkinnedModel::GetBox(int32 lodIndex) const
{
    if (LODs.Count() == 0)
        return BoundingBox::Zero;
    return LODs[lodIndex].GetBox();
}

template<typename ContextType>
FORCE_INLINE void SkinnedModelDraw(SkinnedModel* model, const RenderContext& renderContext, const ContextType& context, const SkinnedMesh::DrawInfo& info)
{
    ASSERT(info.Buffer);
    if (!model->CanBeRendered())
        return;
    if (!info.Buffer->IsValidFor(model))
        info.Buffer->Setup(model);
    const auto frame = Engine::FrameCount;
    const auto modelFrame = info.DrawState->PrevFrame + 1;

    // Select a proper LOD index (model may be culled)
    int32 lodIndex;
    if (info.ForcedLOD != -1)
    {
        lodIndex = info.ForcedLOD;
    }
    else
    {
        lodIndex = RenderTools::ComputeSkinnedModelLOD(model, info.Bounds.Center, (float)info.Bounds.Radius, renderContext);
        if (lodIndex == -1)
        {
            // Handling model fade-out transition
            if (modelFrame == frame && info.DrawState->PrevLOD != -1 && !renderContext.View.IsSingleFrame)
            {
                // Check if start transition
                if (info.DrawState->LODTransition == 255)
                {
                    info.DrawState->LODTransition = 0;
                }

                RenderTools::UpdateModelLODTransition(info.DrawState->LODTransition);

                // Check if end transition
                if (info.DrawState->LODTransition == 255)
                {
                    info.DrawState->PrevLOD = lodIndex;
                }
                else
                {
                    const auto prevLOD = model->ClampLODIndex(info.DrawState->PrevLOD);
                    const float normalizedProgress = static_cast<float>(info.DrawState->LODTransition) * (1.0f / 255.0f);
                    model->LODs.Get()[prevLOD].Draw(renderContext, info, normalizedProgress);
                }
            }

            return;
        }
    }
    lodIndex += info.LODBias + renderContext.View.ModelLODBias;
    lodIndex = model->ClampLODIndex(lodIndex);

    if (renderContext.View.IsSingleFrame)
    {
    }
    // Check if it's the new frame and could update the drawing state (note: model instance could be rendered many times per frame to different viewports)
    else if (modelFrame == frame)
    {
        // Check if start transition
        if (info.DrawState->PrevLOD != lodIndex && info.DrawState->LODTransition == 255)
        {
            info.DrawState->LODTransition = 0;
        }

        RenderTools::UpdateModelLODTransition(info.DrawState->LODTransition);

        // Check if end transition
        if (info.DrawState->LODTransition == 255)
        {
            info.DrawState->PrevLOD = lodIndex;
        }
    }
    // Check if there was a gap between frames in drawing this model instance
    else if (modelFrame < frame || info.DrawState->PrevLOD == -1)
    {
        // Reset state
        info.DrawState->PrevLOD = lodIndex;
        info.DrawState->LODTransition = 255;
    }

    // Draw
    if (info.DrawState->PrevLOD == lodIndex || renderContext.View.IsSingleFrame)
    {
        model->LODs.Get()[lodIndex].Draw(context, info, 0.0f);
    }
    else if (info.DrawState->PrevLOD == -1)
    {
        const float normalizedProgress = static_cast<float>(info.DrawState->LODTransition) * (1.0f / 255.0f);
        model->LODs.Get()[lodIndex].Draw(context, info, 1.0f - normalizedProgress);
    }
    else
    {
        const auto prevLOD = model->ClampLODIndex(info.DrawState->PrevLOD);
        const float normalizedProgress = static_cast<float>(info.DrawState->LODTransition) * (1.0f / 255.0f);
        model->LODs.Get()[prevLOD].Draw(context, info, normalizedProgress);
        model->LODs.Get()[lodIndex].Draw(context, info, normalizedProgress - 1.0f);
    }
}

void SkinnedModel::Draw(const RenderContext& renderContext, const SkinnedMesh::DrawInfo& info)
{
    SkinnedModelDraw(this, renderContext, renderContext, info);
}

void SkinnedModel::Draw(const RenderContextBatch& renderContextBatch, const SkinnedMesh::DrawInfo& info)
{
    SkinnedModelDraw(this, renderContextBatch.GetMainContext(), renderContextBatch, info);
}

bool SkinnedModel::SetupLODs(const Span<int32>& meshesCountPerLod)
{
    ScopeLock lock(Locker);

    // Validate input and state
    if (!IsVirtual())
    {
        LOG(Error, "Only virtual models can be updated at runtime.");
        return true;
    }

    return Init(meshesCountPerLod);;
}

bool SkinnedModel::SetupSkeleton(const Array<SkeletonNode>& nodes)
{
    // Validate input
    if (nodes.Count() <= 0 || nodes.Count() > MAX_uint16)
        return true;
    auto model = this;
    if (!model->IsVirtual())
        return true;

    ScopeLock lock(model->Locker);

    // Setup
    model->Skeleton.Nodes = nodes;
    model->Skeleton.Bones.Resize(nodes.Count());
    for (int32 i = 0; i < nodes.Count(); i++)
    {
        auto& node = model->Skeleton.Nodes[i];
        model->Skeleton.Bones[i].ParentIndex = node.ParentIndex;
        model->Skeleton.Bones[i].LocalTransform = node.LocalTransform;
        model->Skeleton.Bones[i].NodeIndex = i;
    }
    ClearSkeletonMapping();

    // Calculate offset matrix (inverse bind pose transform) for every bone manually
    for (int32 i = 0; i < model->Skeleton.Bones.Count(); i++)
    {
        Matrix t = Matrix::Identity;
        int32 idx = model->Skeleton.Bones[i].NodeIndex;

        do
        {
            t *= model->Skeleton.Nodes[idx].LocalTransform.GetWorld();
            idx = model->Skeleton.Nodes[idx].ParentIndex;
        } while (idx != -1);

        t.Invert();

        model->Skeleton.Bones[i].OffsetMatrix = t;
    }

    return false;
}

bool SkinnedModel::SetupSkeleton(const Array<SkeletonNode>& nodes, const Array<SkeletonBone>& bones, bool autoCalculateOffsetMatrix)
{
    // Validate input
    if (nodes.Count() <= 0 || nodes.Count() > MAX_uint16)
        return true;
    if (bones.Count() <= 0)
    {
        if (bones.Count() > 255)
        {
            for (auto& lod : LODs)
            {
                for (auto& mesh : lod.Meshes)
                {
                    auto* vertexLayout = mesh.GetVertexLayout();
                    VertexElement element = vertexLayout ? vertexLayout->FindElement(VertexElement::Types::BlendIndices) : VertexElement();
                    if (element.Format == PixelFormat::R8G8B8A8_UInt)
                    {
                        LOG(Warning, "Cannot use more than 255 bones if skinned model uses 8-bit storage for blend indices in vertices.");
                        return true;
                    }
                }
            }
        }
        if (bones.Count() > MODEL_MAX_BONES_PER_MODEL)
            return true;
    }
    auto model = this;
    if (!model->IsVirtual())
        return true;

    ScopeLock lock(model->Locker);

    // Setup
    model->Skeleton.Nodes = nodes;
    model->Skeleton.Bones = bones;
    ClearSkeletonMapping();

    // Calculate offset matrix (inverse bind pose transform) for every bone manually
    if (autoCalculateOffsetMatrix)
    {
        for (int32 i = 0; i < model->Skeleton.Bones.Count(); i++)
        {
            Matrix t = Matrix::Identity;
            int32 idx = model->Skeleton.Bones[i].NodeIndex;

            do
            {
                t *= model->Skeleton.Nodes[idx].LocalTransform.GetWorld();
                idx = model->Skeleton.Nodes[idx].ParentIndex;
            } while (idx != -1);

            t.Invert();

            model->Skeleton.Bones[i].OffsetMatrix = t;
        }
    }

    return false;
}

bool SkinnedModel::Init(const Span<int32>& meshesCountPerLod)
{
    if (meshesCountPerLod.IsInvalid() || meshesCountPerLod.Length() > MODEL_MAX_LODS)
    {
        Log::ArgumentOutOfRangeException();
        return true;
    }

    // Dispose previous data and disable streaming (will start data uploading tasks manually)
    StopStreaming();

    // Setup
    MaterialSlots.Resize(1);
    MinScreenSize = 0.0f;
    LODs.Resize(meshesCountPerLod.Length());
    _initialized = true;

    // Setup meshes
    for (int32 lodIndex = 0; lodIndex < meshesCountPerLod.Length(); lodIndex++)
    {
        auto& lod = LODs[lodIndex];
        lod._model = this;
        lod._lodIndex = lodIndex;
        lod.ScreenSize = 1.0f;
        const int32 meshesCount = meshesCountPerLod[lodIndex];
        if (meshesCount < 0 || meshesCount > MODEL_MAX_MESHES)
            return true;

        lod.Meshes.Resize(meshesCount);
        for (int32 meshIndex = 0; meshIndex < meshesCount; meshIndex++)
        {
            lod.Meshes[meshIndex].Link(this, lodIndex, meshIndex);
        }
    }

    // Update resource residency
    _loadedLODs = meshesCountPerLod.Length();
    ResidencyChanged();

    return false;
}

void BlendShape::LoadHeader(ReadStream& stream, byte headerVersion)
{
    stream.Read(Name, 13);
    stream.Read(Weight);
}

void BlendShape::Load(ReadStream& stream, byte meshVersion)
{
    UseNormals = stream.ReadBool();
    stream.ReadUint32(&MinVertexIndex);
    stream.ReadUint32(&MaxVertexIndex);
    uint32 blendShapeVertices;
    stream.ReadUint32(&blendShapeVertices);
    Vertices.Resize(blendShapeVertices);
    stream.ReadBytes(Vertices.Get(), Vertices.Count() * sizeof(BlendShapeVertex));
}

#if USE_EDITOR

void BlendShape::SaveHeader(WriteStream& stream) const
{
    stream.Write(Name, 13);
    stream.Write(Weight);
}

void BlendShape::Save(WriteStream& stream) const
{
    stream.WriteBool(UseNormals);
    stream.WriteUint32(MinVertexIndex);
    stream.WriteUint32(MaxVertexIndex);
    stream.WriteUint32(Vertices.Count());
    stream.WriteBytes(Vertices.Get(), Vertices.Count() * sizeof(BlendShapeVertex));
}

#endif

bool SkinnedModel::LoadMesh(MemoryReadStream& stream, byte meshVersion, MeshBase* mesh, MeshData* dataIfReadOnly)
{
    if (ModelBase::LoadMesh(stream, meshVersion, mesh, dataIfReadOnly))
        return true;
    static_assert(MODEL_MESH_VERSION == 2, "Update code");
    auto skinnedMesh = (SkinnedMesh*)mesh;

    // Blend Shapes
    uint16 blendShapesCount;
    stream.Read(blendShapesCount);
    if (dataIfReadOnly)
    {
        // Skip blend shapes
        BlendShape tmp;
        while (blendShapesCount-- != 0)
            tmp.Load(stream, meshVersion);
        return false;
    }
    if (blendShapesCount != skinnedMesh->BlendShapes.Count())
    {
        LOG(Warning, "Incorrect blend shapes amount: {} (expected: {})", blendShapesCount, skinnedMesh->BlendShapes.Count());
        return true;
    }
    for (auto& blendShape : skinnedMesh->BlendShapes)
        blendShape.Load(stream, meshVersion);

    return false;
}

bool SkinnedModel::LoadHeader(ReadStream& stream, byte& headerVersion)
{
    if (ModelBase::LoadHeader(stream, headerVersion))
        return true;
    static_assert(MODEL_HEADER_VERSION == 2, "Update code");

    // LODs
    byte lods = stream.ReadByte();
    if (lods > MODEL_MAX_LODS)
        return true;
    LODs.Resize(lods);
    _initialized = true;
    for (int32 lodIndex = 0; lodIndex < lods; lodIndex++)
    {
        auto& lod = LODs[lodIndex];
        lod._model = this;
        lod._lodIndex = lodIndex;
        stream.Read(lod.ScreenSize);

        // Meshes
        uint16 meshesCount;
        stream.Read(meshesCount);
        if (meshesCount > MODEL_MAX_MESHES)
            return true;
        ASSERT(lodIndex == 0 || LODs[0].Meshes.Count() >= meshesCount);
        lod.Meshes.Resize(meshesCount, false);
        for (uint16 meshIndex = 0; meshIndex < meshesCount; meshIndex++)
        {
            SkinnedMesh& mesh = lod.Meshes[meshIndex];
            mesh.Link(this, lodIndex, meshIndex);

            // Material Slot index
            int32 materialSlotIndex;
            stream.Read(materialSlotIndex);
            if (materialSlotIndex < 0 || materialSlotIndex >= MaterialSlots.Count())
            {
                LOG(Warning, "Invalid material slot index {0} for mesh {1}. Slots count: {2}.", materialSlotIndex, meshIndex, MaterialSlots.Count());
                return true;
            }
            mesh.SetMaterialSlotIndex(materialSlotIndex);

            // Bounds
            BoundingBox box;
            stream.Read(box);
            BoundingSphere sphere;
            stream.Read(sphere);
            mesh.SetBounds(box, sphere);

            // Blend Shapes
            uint16 blendShapes;
            stream.Read(blendShapes);
            mesh.BlendShapes.Resize(blendShapes);
            for (auto& blendShape : mesh.BlendShapes)
                blendShape.LoadHeader(stream, headerVersion);
        }
    }

    // Skeleton
    {
        int32 nodesCount;
        stream.Read(nodesCount);
        if (nodesCount < 0)
            return true;
        Skeleton.Nodes.Resize(nodesCount, false);
        for (auto& node : Skeleton.Nodes)
        {
            stream.Read(node.ParentIndex);
            stream.Read(node.LocalTransform);
            stream.Read(node.Name, 71);
        }

        int32 bonesCount;
        stream.Read(bonesCount);
        if (bonesCount < 0)
            return true;
        Skeleton.Bones.Resize(bonesCount, false);
        for (auto& bone : Skeleton.Bones)
        {
            stream.Read(bone.ParentIndex);
            stream.Read(bone.NodeIndex);
            stream.Read(bone.LocalTransform);
            stream.Read(bone.OffsetMatrix);
        }
    }

    // Retargeting
    {
        int32 entriesCount;
        stream.Read(entriesCount);
        _skeletonRetargets.Resize(entriesCount);
        for (auto& retarget : _skeletonRetargets)
        {
            stream.Read(retarget.SourceAsset);
            stream.Read(retarget.SkeletonAsset);
            stream.Read(retarget.NodesMapping);
        }
    }

    return false;
}

#if USE_EDITOR

bool SkinnedModel::SaveHeader(WriteStream& stream) const
{
    if (ModelBase::SaveHeader(stream))
        return true;
    static_assert(MODEL_HEADER_VERSION == 2, "Update code");

    // LODs
    stream.Write((byte)LODs.Count());
    for (int32 lodIndex = 0; lodIndex < LODs.Count(); lodIndex++)
    {
        auto& lod = LODs[lodIndex];
        stream.Write(lod.ScreenSize);

        // Meshes
        stream.Write((uint16)lod.Meshes.Count());
        for (const auto& mesh : lod.Meshes)
        {
            stream.Write(mesh.GetMaterialSlotIndex());
            stream.Write(mesh.GetBox());
            stream.Write(mesh.GetSphere());

            // Blend Shapes
            const int32 blendShapes = mesh.BlendShapes.Count();
            stream.Write((uint16)blendShapes);
            for (const auto& blendShape : mesh.BlendShapes)
                blendShape.Save(stream);
        }
    }

    // Skeleton nodes
    const auto& skeletonNodes = Skeleton.Nodes;
    stream.Write(skeletonNodes.Count());
    for (const auto& node : skeletonNodes)
    {
        stream.Write(node.ParentIndex);
        stream.Write(node.LocalTransform);
        stream.Write(node.Name, 71);
    }

    // Skeleton bones
    const auto& skeletonBones = Skeleton.Bones;
    stream.WriteInt32(skeletonBones.Count());
    for (const auto& bone : skeletonBones)
    {
        stream.Write(bone.ParentIndex);
        stream.Write(bone.NodeIndex);
        stream.Write(bone.LocalTransform);
        stream.Write(bone.OffsetMatrix);
    }

    // Retargeting
    stream.WriteInt32(_skeletonRetargets.Count());
    for (const auto& retarget : _skeletonRetargets)
    {
        stream.Write(retarget.SourceAsset);
        stream.Write(retarget.SkeletonAsset);
        stream.Write(retarget.NodesMapping);
    }

    return false;
}

bool SkinnedModel::SaveHeader(WriteStream& stream, const ModelData& modelData)
{
    if (ModelBase::SaveHeader(stream, modelData))
        return true;
    static_assert(MODEL_HEADER_VERSION == 2, "Update code");

    // LODs
    stream.Write((byte)modelData.LODs.Count());
    for (int32 lodIndex = 0; lodIndex < modelData.LODs.Count(); lodIndex++)
    {
        auto& lod = modelData.LODs[lodIndex];

        // Screen Size
        stream.Write(lod.ScreenSize);

        // Meshes
        stream.Write((uint16)lod.Meshes.Count());
        for (const auto& mesh : lod.Meshes)
        {
            BoundingBox box;
            BoundingSphere sphere;
            mesh->CalculateBounds(box, sphere);
            stream.Write(mesh->MaterialSlotIndex);
            stream.Write(box);
            stream.Write(sphere);

            // Blend Shapes
            const int32 blendShapes = mesh->BlendShapes.Count();
            stream.WriteUint16((uint16)blendShapes);
            for (const auto& blendShape : mesh->BlendShapes)
                blendShape.SaveHeader(stream);
        }
    }

    // Skeleton nodes
    const auto& skeletonNodes = modelData.Skeleton.Nodes;
    stream.WriteInt32(skeletonNodes.Count());
    for (const auto& node : skeletonNodes)
    {
        stream.Write(node.ParentIndex);
        stream.Write(node.LocalTransform);
        stream.Write(node.Name, 71);
    }

    // Skeleton bones
    const auto& skeletonBones = modelData.Skeleton.Bones;
    stream.WriteInt32(skeletonBones.Count());
    for (const auto& bone : skeletonBones)
    {
        stream.Write(bone.ParentIndex);
        stream.Write(bone.NodeIndex);
        stream.Write(bone.LocalTransform);
        stream.Write(bone.OffsetMatrix);
    }

    // Retargeting
    stream.Write(0); // Empty list

    return false;
}

bool SkinnedModel::SaveMesh(WriteStream& stream, const MeshBase* mesh) const
{
    if (ModelBase::SaveMesh(stream, mesh))
        return true;
    static_assert(MODEL_MESH_VERSION == 2, "Update code");
    auto skinnedMesh = (const SkinnedMesh*)mesh;

    // Blend Shapes
    uint16 blendShapesCount = skinnedMesh->BlendShapes.Count();
    stream.Write(blendShapesCount);
    for (auto& blendShape : skinnedMesh->BlendShapes)
        blendShape.Save(stream);

    return false;
}

bool SkinnedModel::SaveMesh(WriteStream& stream, const ModelData& modelData, int32 lodIndex, int32 meshIndex)
{
    static_assert(MODEL_MESH_VERSION == 2, "Update code");
    auto& mesh = modelData.LODs[lodIndex].Meshes[meshIndex];

    // Blend Shapes
    uint16 blendShapesCount = (uint16)mesh->BlendShapes.Count();
    stream.Write(blendShapesCount);
    for (auto& blendShape : mesh->BlendShapes)
        blendShape.Save(stream);

    return false;
}

#endif

void SkinnedModel::ClearSkeletonMapping()
{
    for (auto& e : _skeletonMappingCache)
    {
        e.Key->OnUnloaded.Unbind<SkinnedModel, &SkinnedModel::OnSkeletonMappingSourceAssetUnloaded>(this);
#if USE_EDITOR
        e.Key->OnReloading.Unbind<SkinnedModel, &SkinnedModel::OnSkeletonMappingSourceAssetUnloaded>(this);
#endif
        Allocator::Free(e.Value.NodesMapping.Get());
    }
    _skeletonMappingCache.Clear();
}

void SkinnedModel::OnSkeletonMappingSourceAssetUnloaded(Asset* obj)
{
    ScopeLock lock(Locker);
    auto i = _skeletonMappingCache.Find(obj);
    ASSERT(i != _skeletonMappingCache.End());

    // Unlink event
    obj->OnUnloaded.Unbind<SkinnedModel, &SkinnedModel::OnSkeletonMappingSourceAssetUnloaded>(this);
#if USE_EDITOR
    obj->OnReloading.Unbind<SkinnedModel, &SkinnedModel::OnSkeletonMappingSourceAssetUnloaded>(this);
#endif

    // Clear cache
    Allocator::Free(i->Value.NodesMapping.Get());
    _skeletonMappingCache.Remove(i);
}

uint64 SkinnedModel::GetMemoryUsage() const
{
    Locker.Lock();
    uint64 result = BinaryAsset::GetMemoryUsage();
    result += sizeof(SkinnedModel) - sizeof(BinaryAsset);
    result += Skeleton.GetMemoryUsage();
    result += _skeletonMappingCache.Capacity() * sizeof(Dictionary<Asset*, Span<int32>>::Bucket);
    for (const auto& e : _skeletonMappingCache)
        result += e.Value.NodesMapping.Length();
    Locker.Unlock();
    return result;
}

void SkinnedModel::SetupMaterialSlots(int32 slotsCount)
{
    ModelBase::SetupMaterialSlots(slotsCount);

    // Adjust meshes indices for slots
    for (int32 lodIndex = 0; lodIndex < LODs.Count(); lodIndex++)
    {
        for (int32 meshIndex = 0; meshIndex < LODs[lodIndex].Meshes.Count(); meshIndex++)
        {
            auto& mesh = LODs[lodIndex].Meshes[meshIndex];
            if (mesh.GetMaterialSlotIndex() >= slotsCount)
                mesh.SetMaterialSlotIndex(slotsCount - 1);
        }
    }
}

int32 SkinnedModel::GetLODsCount() const
{
    return LODs.Count();
}

const ModelLODBase* SkinnedModel::GetLOD(int32 lodIndex) const
{
    CHECK_RETURN(LODs.IsValidIndex(lodIndex), nullptr);
    return &LODs.Get()[lodIndex];
}

ModelLODBase* SkinnedModel::GetLOD(int32 lodIndex)
{
    CHECK_RETURN(LODs.IsValidIndex(lodIndex), nullptr);
    return &LODs.Get()[lodIndex];
}

const MeshBase* SkinnedModel::GetMesh(int32 meshIndex, int32 lodIndex) const
{
    auto& lod = LODs[lodIndex];
    return &lod.Meshes[meshIndex];
}

MeshBase* SkinnedModel::GetMesh(int32 meshIndex, int32 lodIndex)
{
    auto& lod = LODs[lodIndex];
    return &lod.Meshes[meshIndex];
}

void SkinnedModel::GetMeshes(Array<const MeshBase*>& meshes, int32 lodIndex) const
{
    LODs[lodIndex].GetMeshes(meshes);
}

void SkinnedModel::GetMeshes(Array<MeshBase*>& meshes, int32 lodIndex)
{
    LODs[lodIndex].GetMeshes(meshes);
}

void SkinnedModel::InitAsVirtual()
{
    // Init with one mesh and single bone
    int32 meshesCount = 1;
    Init(ToSpan(&meshesCount, 1));
    ClearSkeletonMapping();
    Skeleton.Dispose();
    //
    Skeleton.Nodes.Resize(1);
    Skeleton.Nodes[0].Name = TEXT("Root");
    Skeleton.Nodes[0].LocalTransform = Transform::Identity;
    Skeleton.Nodes[0].ParentIndex = -1;
    //
    Skeleton.Bones.Resize(1);
    Skeleton.Bones[0].NodeIndex = 0;
    Skeleton.Bones[0].OffsetMatrix = Matrix::Identity;
    Skeleton.Bones[0].LocalTransform = Transform::Identity;
    Skeleton.Bones[0].ParentIndex = -1;

    // Base
    BinaryAsset::InitAsVirtual();
}

int32 SkinnedModel::GetMaxResidency() const
{
    return LODs.Count();
}

int32 SkinnedModel::GetAllocatedResidency() const
{
    return LODs.Count();
}

Asset::LoadResult SkinnedModel::load()
{
    // Get header chunk
    auto chunk0 = GetChunk(0);
    if (chunk0 == nullptr || chunk0->IsMissing())
        return LoadResult::MissingDataChunk;
    MemoryReadStream headerStream(chunk0->Get(), chunk0->Size());

    // Load asset data (anything but mesh contents that use streaming)
    byte headerVersion;
    if (LoadHeader(headerStream, headerVersion))
        return LoadResult::InvalidData;

    // Request resource streaming
    StartStreaming(true);

    return LoadResult::Ok;
}

void SkinnedModel::unload(bool isReloading)
{
    ModelBase::unload(isReloading);

    // Cleanup
    LODs.Clear();
    Skeleton.Dispose();
    _skeletonRetargets.Clear();
    ClearSkeletonMapping();
}

AssetChunksFlag SkinnedModel::getChunksToPreload() const
{
    // Note: we don't preload any meshes here because it's done by the Streaming Manager
    return GET_CHUNK_FLAG(0);
}

bool SkinnedModelLOD::Intersects(const Ray& ray, const Matrix& world, Real& distance, Vector3& normal, SkinnedMesh** mesh)
{
    // Check all meshes
    bool result = false;
    Real closest = MAX_float;
    Vector3 closestNormal = Vector3::Up;
    for (int32 i = 0; i < Meshes.Count(); i++)
    {
        // Test intersection with mesh and check if is closer than previous
        Real dst;
        Vector3 nrm;
        if (Meshes[i].Intersects(ray, world, dst, nrm) && dst < closest)
        {
            result = true;
            *mesh = &Meshes[i];
            closest = dst;
            closestNormal = nrm;
        }
    }

    distance = closest;
    normal = closestNormal;
    return result;
}

bool SkinnedModelLOD::Intersects(const Ray& ray, const Transform& transform, Real& distance, Vector3& normal, SkinnedMesh** mesh)
{
    // Check all meshes
    bool result = false;
    Real closest = MAX_float;
    Vector3 closestNormal = Vector3::Up;
    for (int32 i = 0; i < Meshes.Count(); i++)
    {
        // Test intersection with mesh and check if is closer than previous
        Real dst;
        Vector3 nrm;
        if (Meshes[i].Intersects(ray, transform, dst, nrm) && dst < closest)
        {
            result = true;
            *mesh = &Meshes[i];
            closest = dst;
            closestNormal = nrm;
        }
    }

    distance = closest;
    normal = closestNormal;
    return result;
}

int32 SkinnedModelLOD::GetMeshesCount() const
{
    return Meshes.Count();
}

const MeshBase* SkinnedModelLOD::GetMesh(int32 index) const
{
    return Meshes.Get() + index;
}

MeshBase* SkinnedModelLOD::GetMesh(int32 index)
{
    return Meshes.Get() + index;
}

void SkinnedModelLOD::GetMeshes(Array<MeshBase*>& meshes)
{
    meshes.Resize(Meshes.Count());
    for (int32 meshIndex = 0; meshIndex < Meshes.Count(); meshIndex++)
        meshes[meshIndex] = &Meshes.Get()[meshIndex];
}

void SkinnedModelLOD::GetMeshes(Array<const MeshBase*>& meshes) const
{
    meshes.Resize(Meshes.Count());
    for (int32 meshIndex = 0; meshIndex < Meshes.Count(); meshIndex++)
        meshes[meshIndex] = &Meshes.Get()[meshIndex];
}
