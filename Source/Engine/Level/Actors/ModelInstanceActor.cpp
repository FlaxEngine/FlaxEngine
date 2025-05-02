// Copyright (c) Wojciech Figat. All rights reserved.

#include "ModelInstanceActor.h"
#include "Engine/Content/Assets/MaterialInstance.h"
#include "Engine/Level/Scene/SceneRendering.h"

ModelInstanceActor::ModelInstanceActor(const SpawnParams& params)
    : Actor(params)
{
}

String ModelInstanceActor::MeshReference::ToString() const
{
    return String::Format(TEXT("Actor={},LOD={},Mesh={}"), Actor ? Actor->GetNamePath() : String::Empty, LODIndex, MeshIndex);
}

void ModelInstanceActor::SetEntries(const Array<ModelInstanceEntry>& value)
{
    WaitForModelLoad();
    bool anyChanged = false;
    Entries.Resize(value.Count());
    for (int32 i = 0; i < value.Count(); i++)
    {
        anyChanged |= Entries[i] != value[i];
        Entries[i] = value[i];
    }
    if (anyChanged && _sceneRenderingKey != -1)
        GetSceneRendering()->UpdateActor(this, _sceneRenderingKey, ISceneRenderingListener::Visual);
}

void ModelInstanceActor::SetMaterial(int32 entryIndex, MaterialBase* material)
{
    WaitForModelLoad();
    if (Entries.Count() == 0 && !material)
        return;
    CHECK(entryIndex >= 0 && entryIndex < Entries.Count());
    if (Entries[entryIndex].Material == material)
        return;
    Entries[entryIndex].Material = material;
    if (_sceneRenderingKey != -1)
        GetSceneRendering()->UpdateActor(this, _sceneRenderingKey, ISceneRenderingListener::Visual);
}

MaterialInstance* ModelInstanceActor::CreateAndSetVirtualMaterialInstance(int32 entryIndex)
{
    WaitForModelLoad();
    MaterialBase* material = GetMaterial(entryIndex);
    CHECK_RETURN(material && !material->WaitForLoaded(), nullptr);
    MaterialInstance* result = material->CreateVirtualInstance();
    Entries[entryIndex].Material = result;
    if (_sceneRenderingKey != -1)
        GetSceneRendering()->UpdateActor(this, _sceneRenderingKey, ISceneRenderingListener::Visual);
    return result;
}

void ModelInstanceActor::WaitForModelLoad()
{
}

void ModelInstanceActor::OnLayerChanged()
{
    if (_sceneRenderingKey != -1)
        GetSceneRendering()->UpdateActor(this, _sceneRenderingKey, ISceneRenderingListener::Layer);
}

void ModelInstanceActor::OnStaticFlagsChanged()
{
    if (_sceneRenderingKey != -1)
        GetSceneRendering()->UpdateActor(this, _sceneRenderingKey, ISceneRenderingListener::StaticFlags);
}

void ModelInstanceActor::OnTransformChanged()
{
    // Base
    Actor::OnTransformChanged();

    UpdateBounds();
}

void ModelInstanceActor::OnEnable()
{
    GetSceneRendering()->AddActor(this, _sceneRenderingKey);

    // Base
    Actor::OnEnable();
}

void ModelInstanceActor::OnDisable()
{
    // Base
    Actor::OnDisable();

    GetSceneRendering()->RemoveActor(this, _sceneRenderingKey);
}
