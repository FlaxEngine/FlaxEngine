// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "ModelInstanceActor.h"
#include "Engine/Content/Assets/MaterialInstance.h"
#include "Engine/Level/Scene/SceneRendering.h"
#include "Engine/Serialization/Serialization.h"

ModelInstanceActor::ModelInstanceActor(const SpawnParams& params)
    : Actor(params)
{
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
        GetSceneRendering()->UpdateActor(this, _sceneRenderingKey);
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
        GetSceneRendering()->UpdateActor(this, _sceneRenderingKey);
}

MaterialInstance* ModelInstanceActor::CreateAndSetVirtualMaterialInstance(int32 entryIndex)
{
    WaitForModelLoad();
    MaterialBase* material = GetMaterial(entryIndex);
    CHECK_RETURN(material && !material->WaitForLoaded(), nullptr);
    MaterialInstance* result = material->CreateVirtualInstance();
    Entries[entryIndex].Material = result;
    if (_sceneRenderingKey != -1)
        GetSceneRendering()->UpdateActor(this, _sceneRenderingKey);
    return result;
}

void ModelInstanceActor::SetDrawCustomDepth(bool value)
{
    _drawCustomDepth = value;
}

void ModelInstanceActor::Serialize(SerializeStream& stream, const void* otherObj)
{
    Actor::Serialize(stream, otherObj);
    SERIALIZE_GET_OTHER_OBJ(ModelInstanceActor);

    SERIALIZE_MEMBER(DrawCustomDepth, _drawCustomDepth);
}

void ModelInstanceActor::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    Actor::Deserialize(stream, modifier);

    DESERIALIZE_MEMBER(DrawCustomDepth, _drawCustomDepth);

    // Update state
    SetDrawCustomDepth(_drawCustomDepth);
}

void ModelInstanceActor::WaitForModelLoad()
{
}

void ModelInstanceActor::OnLayerChanged()
{
    if (_sceneRenderingKey != -1)
        GetSceneRendering()->UpdateActor(this, _sceneRenderingKey);
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
