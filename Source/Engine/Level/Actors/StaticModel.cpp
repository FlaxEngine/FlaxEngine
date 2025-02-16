// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "StaticModel.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Graphics/GPUBuffer.h"
#include "Engine/Graphics/GPUBufferDescription.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/Models/MeshDeformation.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Level/Prefabs/PrefabManager.h"
#include "Engine/Level/Scene/Scene.h"
#include "Engine/Renderer/GlobalSignDistanceFieldPass.h"
#include "Engine/Renderer/GI/GlobalSurfaceAtlasPass.h"
#include "Engine/Utilities/Encryption.h"
#if USE_EDITOR
#include "Editor/Editor.h"
#endif

StaticModel::StaticModel(const SpawnParams& params)
    : ModelInstanceActor(params)
    , _scaleInLightmap(1.0f)
    , _boundsScale(1.0f)
    , _lodBias(0)
    , _forcedLod(-1)
    , _vertexColorsDirty(false)
    , _vertexColorsCount(0)
    , _sortOrder(0)
{
    _drawCategory = SceneRendering::SceneDrawAsync;
    Model.Changed.Bind<StaticModel, &StaticModel::OnModelChanged>(this);
    Model.Loaded.Bind<StaticModel, &StaticModel::OnModelLoaded>(this);
}

StaticModel::~StaticModel()
{
    for (int32 lodIndex = 0; lodIndex < _vertexColorsCount; lodIndex++)
        SAFE_DELETE_GPU_RESOURCE(_vertexColorsBuffer[lodIndex]);
    if (_deformation)
        Delete(_deformation);
}

float StaticModel::GetScaleInLightmap() const
{
    return _scaleInLightmap;
}

void StaticModel::SetScaleInLightmap(float value)
{
    _scaleInLightmap = value;
}

float StaticModel::GetBoundsScale() const
{
    return _boundsScale;
}

void StaticModel::SetBoundsScale(float value)
{
    if (Math::NearEqual(_boundsScale, value))
        return;

    _boundsScale = value;
    UpdateBounds();
}

int32 StaticModel::GetLODBias() const
{
    return _lodBias;
}

void StaticModel::SetLODBias(int32 value)
{
    _lodBias = static_cast<char>(Math::Clamp(value, -100, 100));
}

int32 StaticModel::GetForcedLOD() const
{
    return _forcedLod;
}

void StaticModel::SetForcedLOD(int32 value)
{
    _forcedLod = static_cast<char>(Math::Clamp(value, -1, 100));
}

int32 StaticModel::GetSortOrder() const
{
    return _sortOrder;
}

void StaticModel::SetSortOrder(int32 value)
{
    _sortOrder = (int8)Math::Clamp<int32>(value, MIN_int8, MAX_int8);
}

bool StaticModel::HasLightmap() const
{
    return Lightmap.TextureIndex != INVALID_INDEX;
}

void StaticModel::RemoveLightmap()
{
    Lightmap.TextureIndex = INVALID_INDEX;
}

MaterialBase* StaticModel::GetMaterial(int32 meshIndex, int32 lodIndex) const
{
    auto model = Model.Get();
    ASSERT(model &&
        Math::IsInRange(lodIndex, 0, model->GetLODsCount()) &&
        Math::IsInRange(meshIndex, 0, model->LODs[lodIndex].Meshes.Count()));
    const auto& mesh = model->LODs[lodIndex].Meshes[meshIndex];
    const auto materialSlotIndex = mesh.GetMaterialSlotIndex();
    MaterialBase* material = Entries[materialSlotIndex].Material.Get();
    return material ? material : model->MaterialSlots[materialSlotIndex].Material.Get();
}

Color32 StaticModel::GetVertexColor(int32 lodIndex, int32 meshIndex, int32 vertexIndex) const
{
    if (Model && !Model->WaitForLoaded() && _vertexColorsCount == Model->GetLODsCount())
    {
        if (lodIndex < 0 || lodIndex >= Model->GetLODsCount())
        {
            LOG(Warning, "Specified model LOD index {0} was out of range.", lodIndex);
            return Color32::Black;
        }

        int32 index = 0;
        const ModelLOD& lod = Model->LODs[lodIndex];
        auto& vertexColorsData = _vertexColorsData[lodIndex];
        if (vertexColorsData.Count() != lod.GetVertexCount())
            return Color32::Black;
        for (int32 i = 0; i < lod.Meshes.Count(); i++)
        {
            const Mesh& mesh = lod.Meshes[i];
            if (i == meshIndex)
            {
                if (vertexIndex < 0 || vertexIndex >= mesh.GetVertexCount())
                {
                    LOG(Warning, "Specified vertex index {3} was out of range. LOD{0} mesh {1} has {2}.", lodIndex, meshIndex, mesh.GetVertexCount(), vertexIndex);
                    return Color32::Black;
                }
                index += vertexIndex;
                return _vertexColorsData[lodIndex][index];
            }
            index += mesh.GetVertexCount();
        }

        LOG(Warning, "Specified model mesh index was out of range. LOD{0} mesh {1}.", lodIndex, meshIndex);
    }

    return Color32::Black;
}

void StaticModel::SetVertexColor(int32 lodIndex, int32 meshIndex, int32 vertexIndex, const Color32& color)
{
    if (!Model || Model->WaitForLoaded())
    {
        LOG(Warning, "Cannot set vertex color if model is missing or failed to load.");
        return;
    }

    if (lodIndex < 0 || lodIndex >= Model->GetLODsCount())
    {
        LOG(Warning, "Specified model LOD index {0} was out of range.", lodIndex);
        return;
    }

    if (_vertexColorsCount != Model->GetLODsCount())
    {
        // Initialize vertex colors data for all LODs
        RemoveVertexColors();
        _vertexColorsCount = Model->GetLODsCount();
        for (int32 i = 0; i < _vertexColorsCount; i++)
            _vertexColorsBuffer[i] = nullptr;
        _vertexColorsDirty = false;
    }

    int32 index = 0;
    const ModelLOD& lod = Model->LODs[lodIndex];
    auto& vertexColorsData = _vertexColorsData[lodIndex];
    if (vertexColorsData.Count() != lod.GetVertexCount())
    {
        vertexColorsData.Resize(lod.GetVertexCount());
        vertexColorsData.SetAll(Color32::Black);
    }
    for (int32 i = 0; i < lod.Meshes.Count(); i++)
    {
        const Mesh& mesh = lod.Meshes[i];
        if (i == meshIndex)
        {
            if (vertexIndex < 0 || vertexIndex >= mesh.GetVertexCount())
            {
                LOG(Warning, "Specified vertex index {3} was out of range. LOD{0} mesh {1} has {2}.", lodIndex, meshIndex, mesh.GetVertexCount(), vertexIndex);
                return;
            }
            index += vertexIndex;
            vertexColorsData[index] = color;
            _vertexColorsDirty = true;
            return;
        }
        index += mesh.GetVertexCount();
    }

    LOG(Warning, "Specified model mesh index was out of range. LOD{0} mesh {1}.", lodIndex, meshIndex);
}

bool StaticModel::HasVertexColors() const
{
    return _vertexColorsCount != 0;
}

void StaticModel::RemoveVertexColors()
{
    for (int32 lodIndex = 0; lodIndex < _vertexColorsCount; lodIndex++)
        _vertexColorsData[lodIndex].Resize(0);
    for (int32 lodIndex = 0; lodIndex < _vertexColorsCount; lodIndex++)
        SAFE_DELETE_GPU_RESOURCE(_vertexColorsBuffer[lodIndex]);
    _vertexColorsCount = 0;
    _vertexColorsDirty = false;
}

void StaticModel::OnModelChanged()
{
    if (_residencyChangedModel)
    {
        _residencyChangedModel = nullptr;
        Model->ResidencyChanged.Unbind<StaticModel, &StaticModel::OnModelResidencyChanged>(this);
    }
    RemoveVertexColors();
    Entries.Release();
    if (Model && !Model->IsLoaded())
        UpdateBounds();
    if (_deformation)
        _deformation->Clear();
    else if (!Model && _sceneRenderingKey != -1)
        GetSceneRendering()->RemoveActor(this, _sceneRenderingKey);
}

void StaticModel::OnModelLoaded()
{
    Entries.SetupIfInvalid(Model);
    UpdateBounds();
    if (_sceneRenderingKey == -1 && _scene && _isActiveInHierarchy && _isEnabled && !_residencyChangedModel)
    {
        // Register for rendering but once the model has any LOD loaded
        if (Model->GetLoadedLODs() == 0)
        {
            _residencyChangedModel = Model;
            _residencyChangedModel->ResidencyChanged.Bind<StaticModel, &StaticModel::OnModelResidencyChanged>(this);
        }
        else
        {
            GetSceneRendering()->AddActor(this, _sceneRenderingKey);
        }
    }
}

void StaticModel::OnModelResidencyChanged()
{
    if (_sceneRenderingKey == -1 && _scene && Model && Model->GetLoadedLODs() > 0 && _residencyChangedModel)
    {
        GetSceneRendering()->AddActor(this, _sceneRenderingKey);
        _residencyChangedModel->ResidencyChanged.Unbind<StaticModel, &StaticModel::OnModelResidencyChanged>(this);
        _residencyChangedModel = nullptr;
    }
}

void StaticModel::UpdateBounds()
{
    const auto model = Model.Get();
    if (model && model->IsLoaded() && model->LODs.Count() != 0)
    {
        Transform transform = _transform;
        transform.Scale *= _boundsScale;
        _box = model->LODs[0].GetBox(transform, _deformation);
    }
    else
    {
        _box = BoundingBox(_transform.Translation);
    }
    BoundingSphere::FromBox(_box, _sphere);
    if (_sceneRenderingKey != -1)
        GetSceneRendering()->UpdateActor(this, _sceneRenderingKey, ISceneRenderingListener::Bounds);
}

void StaticModel::FlushVertexColors()
{
    RenderContext::GPULocker.Lock();
    for (int32 lodIndex = 0; lodIndex < _vertexColorsCount; lodIndex++)
    {
        auto& vertexColorsData = _vertexColorsData[lodIndex];
        auto& vertexColorsBuffer = _vertexColorsBuffer[lodIndex];
        if (vertexColorsData.HasItems())
        {
            const uint32 size = vertexColorsData.Count() * sizeof(Color32);
            if (!vertexColorsBuffer)
                vertexColorsBuffer = GPUDevice::Instance->CreateBuffer(TEXT("VertexColors"));
            if (vertexColorsBuffer->GetSize() != size)
            {
                if (vertexColorsBuffer->Init(GPUBufferDescription::Vertex(sizeof(Color32), vertexColorsData.Count())))
                    break;
            }
            GPUDevice::Instance->GetMainContext()->UpdateBuffer(vertexColorsBuffer, vertexColorsData.Get(), size);
        }
        else
        {
            SAFE_DELETE_GPU_RESOURCE(vertexColorsBuffer);
        }
    }
    RenderContext::GPULocker.Unlock();
}

bool StaticModel::HasContentLoaded() const
{
    return (Model == nullptr || Model->IsLoaded()) && Entries.HasContentLoaded();
}

void StaticModel::Draw(RenderContext& renderContext)
{
    if (!Model || !Model->IsLoaded() || !Model->CanBeRendered())
        return;
    if (renderContext.View.Pass == DrawPass::GlobalSDF)
    {
        if (EnumHasAnyFlags(DrawModes, DrawPass::GlobalSDF) && Model->SDF.Texture)
            GlobalSignDistanceFieldPass::Instance()->RasterizeModelSDF(this, Model->SDF, _transform, _box);
        return;
    }
    if (renderContext.View.Pass == DrawPass::GlobalSurfaceAtlas)
    {
        if (EnumHasAnyFlags(DrawModes, DrawPass::GlobalSurfaceAtlas) && Model->SDF.Texture)
            GlobalSurfaceAtlasPass::Instance()->RasterizeActor(this, this, _sphere, _transform, Model->LODs.Last().GetBox());
        return;
    }
    ACTOR_GET_WORLD_MATRIX(this, view, world);
    GEOMETRY_DRAW_STATE_EVENT_BEGIN(_drawState, world);
    if (_vertexColorsDirty)
        FlushVertexColors();

    Mesh::DrawInfo draw;
    draw.Buffer = &Entries;
    draw.World = &world;
    draw.DrawState = &_drawState;
    draw.Deformation = _deformation;
    draw.Lightmap = _scene ? _scene->LightmapsData.GetReadyLightmap(Lightmap.TextureIndex) : nullptr;
    draw.LightmapUVs = &Lightmap.UVsArea;
    draw.Flags = _staticFlags;
    draw.DrawModes = DrawModes;
    draw.Bounds = _sphere;
    draw.Bounds.Center -= renderContext.View.Origin;
    draw.PerInstanceRandom = GetPerInstanceRandom();
    draw.LODBias = _lodBias;
    draw.ForcedLOD = _forcedLod;
    draw.SortOrder = _sortOrder;
    draw.VertexColors = _vertexColorsCount ? _vertexColorsBuffer : nullptr;
#if USE_EDITOR
    if (HasStaticFlag(StaticFlags::Lightmap))
        draw.LightmapScale = _scaleInLightmap;
#endif

    Model->Draw(renderContext, draw);

    GEOMETRY_DRAW_STATE_EVENT_END(_drawState, world);
}

void StaticModel::Draw(RenderContextBatch& renderContextBatch)
{
    if (!Model || !Model->IsLoaded())
        return;
    const RenderContext& renderContext = renderContextBatch.GetMainContext();
    ACTOR_GET_WORLD_MATRIX(this, view, world);
    GEOMETRY_DRAW_STATE_EVENT_BEGIN(_drawState, world);
    if (_vertexColorsDirty)
        FlushVertexColors();

    Mesh::DrawInfo draw;
    draw.Buffer = &Entries;
    draw.World = &world;
    draw.DrawState = &_drawState;
    draw.Deformation = _deformation;
    draw.Lightmap = _scene ? _scene->LightmapsData.GetReadyLightmap(Lightmap.TextureIndex) : nullptr;
    draw.LightmapUVs = &Lightmap.UVsArea;
    draw.Flags = _staticFlags;
    draw.DrawModes = DrawModes;
    draw.Bounds = _sphere;
    draw.Bounds.Center -= renderContext.View.Origin;
    draw.PerInstanceRandom = GetPerInstanceRandom();
    draw.LODBias = _lodBias;
    draw.ForcedLOD = _forcedLod;
    draw.SortOrder = _sortOrder;
    draw.VertexColors = _vertexColorsCount ? _vertexColorsBuffer : nullptr;
#if USE_EDITOR
    if (HasStaticFlag(StaticFlags::Lightmap))
        draw.LightmapScale = _scaleInLightmap;
#endif

    Model->Draw(renderContextBatch, draw);

    GEOMETRY_DRAW_STATE_EVENT_END(_drawState, world);
}

bool StaticModel::IntersectsItself(const Ray& ray, Real& distance, Vector3& normal)
{
    bool result = false;
    if (Model != nullptr && Model->IsLoaded())
    {
        Mesh* mesh;
        Matrix world;
        GetLocalToWorldMatrix(world);
        result = Model->Intersects(ray, world, distance, normal, &mesh);
    }
    return result;
}

void StaticModel::Serialize(SerializeStream& stream, const void* otherObj)
{
    // Base
    ModelInstanceActor::Serialize(stream, otherObj);

    SERIALIZE_GET_OTHER_OBJ(StaticModel);

    SERIALIZE_MEMBER(ScaleInLightmap, _scaleInLightmap);
    SERIALIZE_MEMBER(BoundsScale, _boundsScale);
    SERIALIZE(Model);
    SERIALIZE_MEMBER(LODBias, _lodBias);
    SERIALIZE_MEMBER(ForcedLOD, _forcedLod);
    SERIALIZE_MEMBER(SortOrder, _sortOrder);
    SERIALIZE(DrawModes);

    if (HasLightmap()
#if USE_EDITOR
        && !PrefabManager::IsCreatingPrefab
#endif
    )
    {
        stream.JKEY("LightmapIndex");
        stream.Int(Lightmap.TextureIndex);

        stream.JKEY("LightmapArea");
        stream.Rectangle(Lightmap.UVsArea);
    }

    stream.JKEY("Buffer");
    stream.Object(&Entries, other ? &other->Entries : nullptr);

    if (_vertexColorsCount)
    {
        stream.JKEY("VertexColors");
        stream.StartArray();
        Array<char> encodedData;
        for (int32 lodIndex = 0; lodIndex < _vertexColorsCount; lodIndex++)
        {
            auto& vertexColorsData = _vertexColorsData[lodIndex];
            if (vertexColorsData.HasItems())
            {
                const int32 size = vertexColorsData.Count() * sizeof(Color32);
                Encryption::Base64Encode((byte*)vertexColorsData.Get(), size, encodedData);
                stream.String(encodedData.Get(), encodedData.Count());
            }
            else
            {
                stream.String("", 0);
            }
        }
        stream.EndArray();
    }
}

void StaticModel::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    // Base
    ModelInstanceActor::Deserialize(stream, modifier);

    DESERIALIZE_MEMBER(ScaleInLightmap, _scaleInLightmap);
    DESERIALIZE_MEMBER(BoundsScale, _boundsScale);
    DESERIALIZE(Model);
    DESERIALIZE_MEMBER(LODBias, _lodBias);
    DESERIALIZE_MEMBER(ForcedLOD, _forcedLod);
    DESERIALIZE_MEMBER(SortOrder, _sortOrder);
    DESERIALIZE(DrawModes);
    DESERIALIZE_MEMBER(LightmapIndex, Lightmap.TextureIndex);
    DESERIALIZE_MEMBER(LightmapArea, Lightmap.UVsArea);

    Entries.DeserializeIfExists(stream, "Buffer", modifier);

    {
        const auto member = stream.FindMember("VertexColors");
        if (member != stream.MemberEnd() && member->value.IsArray())
        {
            // TODO: don't stall but just check the length of the loaded vertex colors arrays size later when asset gets loaded
            if (Model && !Model->WaitForLoaded())
            {
                RemoveVertexColors();
                auto& array = member->value;
                _vertexColorsCount = array.Size();
                Array<byte> decodedData;
                if (_vertexColorsCount == Model->GetLODsCount())
                {
                    for (int32 lodIndex = 0; lodIndex < _vertexColorsCount; lodIndex++)
                    {
                        _vertexColorsBuffer[lodIndex] = nullptr;
                        auto& vertexColorsData = _vertexColorsData[lodIndex];
                        vertexColorsData.Clear();
                        auto& v = array[lodIndex];
                        if (v.IsString())
                        {
                            Encryption::Base64Decode(v.GetString(), v.GetStringLength(), decodedData);
                            const int32 length = decodedData.Count() / sizeof(Color32);
                            vertexColorsData.Resize(length);
                            Platform::MemoryCopy(vertexColorsData.Get(), decodedData.Get(), decodedData.Count());
                        }
                    }
                }
                else
                {
                    LOG(Error, "Loaded vertex colors data for {0} has different size than the model {1} LODs count.", ToString(), Model->ToString());
                }
                _vertexColorsDirty = true;
            }
        }
    }

    // [Deprecated on 11.10.2019, expires on 11.10.2020]
    if (modifier->EngineBuild <= 6187)
    {
        const auto member = stream.FindMember("HiddenShadow");
        if (member != stream.MemberEnd() && member->value.IsBool() && member->value.GetBool())
        {
            DrawModes = DrawPass::Depth;
        }
    }
    // [Deprecated on 07.02.2022, expires on 07.02.2024]
    if (modifier->EngineBuild <= 6330)
        DrawModes |= DrawPass::GlobalSDF;
    // [Deprecated on 27.04.2022, expires on 27.04.2024]
    if (modifier->EngineBuild <= 6331)
        DrawModes |= DrawPass::GlobalSurfaceAtlas;

    {
        const auto member = stream.FindMember("RenderPasses");
        if (member != stream.MemberEnd() && member->value.IsInt())
        {
            DrawModes = (DrawPass)member->value.GetInt();
        }
    }
}

const Span<MaterialSlot> StaticModel::GetMaterialSlots() const
{
    const auto model = Model.Get();
    if (model && !model->WaitForLoaded())
        return ToSpan(model->MaterialSlots);
    return Span<MaterialSlot>();
}

MaterialBase* StaticModel::GetMaterial(int32 entryIndex)
{
    if (!Model || Model->WaitForLoaded())
        return nullptr;
    CHECK_RETURN(entryIndex >= 0 && entryIndex < Entries.Count(), nullptr);
    MaterialBase* material = Entries[entryIndex].Material.Get();
    if (!material && entryIndex < Model->MaterialSlots.Count())
    {
        material = Model->MaterialSlots[entryIndex].Material.Get();
        if (!material)
            material = GPUDevice::Instance->GetDefaultMaterial();
    }
    return material;
}

bool StaticModel::IntersectsEntry(int32 entryIndex, const Ray& ray, Real& distance, Vector3& normal)
{
    auto model = Model.Get();
    if (!model || !model->IsInitialized() || model->GetLoadedLODs() == 0)
        return false;

    // Find mesh in the highest loaded LOD that is using the given material slot index and ray hits it
    auto& meshes = model->LODs[model->HighestResidentLODIndex()].Meshes;
    for (int32 i = 0; i < meshes.Count(); i++)
    {
        const auto& mesh = meshes[i];
        if (mesh.GetMaterialSlotIndex() == entryIndex && mesh.Intersects(ray, _transform, distance, normal))
            return true;
    }

    distance = 0;
    normal = Vector3::Up;
    return false;
}

bool StaticModel::IntersectsEntry(const Ray& ray, Real& distance, Vector3& normal, int32& entryIndex)
{
    auto model = Model.Get();
    if (!model || !model->IsInitialized() || model->GetLoadedLODs() == 0)
        return false;

    // Find mesh in the highest loaded LOD that is using the given material slot index and ray hits it
    bool result = false;
    Real closest = MAX_Real;
    Vector3 closestNormal = Vector3::Up;
    int32 closestEntry = -1;
    auto& meshes = model->LODs[model->HighestResidentLODIndex()].Meshes;
    for (int32 i = 0; i < meshes.Count(); i++)
    {
        // Test intersection with mesh and check if is closer than previous
        const auto& mesh = meshes[i];
        Real dst;
        Vector3 nrm;
        if (mesh.Intersects(ray, _transform, dst, nrm) && dst < closest)
        {
            result = true;
            closest = dst;
            closestNormal = nrm;
            closestEntry = mesh.GetMaterialSlotIndex();
        }
    }

    distance = closest;
    normal = closestNormal;
    entryIndex = closestEntry;
    return result;
}

bool StaticModel::GetMeshData(const MeshReference& mesh, MeshBufferType type, BytesContainer& result, int32& count) const
{
    count = 0;
    if (mesh.LODIndex < 0 || mesh.MeshIndex < 0)
        return true;
    const auto model = Model.Get();
    if (!model || model->WaitForLoaded())
        return true;
    auto& lod = model->LODs[Math::Min(mesh.LODIndex, model->LODs.Count() - 1)];
    return lod.Meshes[Math::Min(mesh.MeshIndex, lod.Meshes.Count() - 1)].DownloadDataCPU(type, result, count);
}

MeshDeformation* StaticModel::GetMeshDeformation() const
{
    if (!_deformation)
        _deformation = New<MeshDeformation>();
    return _deformation;
}

void StaticModel::OnEnable()
{
    // If model is set and loaded but we still don't have residency registered do it here (eg. model is streaming LODs right now)
    if (_scene && _sceneRenderingKey == -1 && !_residencyChangedModel && Model && Model->IsLoaded())
    {
        // Register for rendering but once the model has any LOD loaded
        if (Model->GetLoadedLODs() == 0)
        {
            _residencyChangedModel = Model;
            _residencyChangedModel->ResidencyChanged.Bind<StaticModel, &StaticModel::OnModelResidencyChanged>(this);
        }
        else
        {
            GetSceneRendering()->AddActor(this, _sceneRenderingKey);
        }
    }

    // Skip ModelInstanceActor (add to SceneRendering manually)
    Actor::OnEnable();
}

void StaticModel::OnDisable()
{
    // Skip ModelInstanceActor (add to SceneRendering manually)
    Actor::OnDisable();

    if (_sceneRenderingKey != -1)
    {
        GetSceneRendering()->RemoveActor(this, _sceneRenderingKey);
    }
    if (_residencyChangedModel)
    {
        _residencyChangedModel->ResidencyChanged.Unbind<StaticModel, &StaticModel::OnModelResidencyChanged>(this);
        _residencyChangedModel = nullptr;
    }
}

void StaticModel::WaitForModelLoad()
{
    if (Model)
        Model->WaitForLoaded();
}
