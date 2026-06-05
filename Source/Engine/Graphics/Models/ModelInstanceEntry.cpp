// Copyright (c) Wojciech Figat. All rights reserved.

#include "ModelInstanceEntry.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Content/Assets/Model.h"
#include "Engine/Content/Assets/SkinnedModel.h"
#include "Engine/Profiler/ProfilerMemory.h"

bool ModelInstanceEntries::HasContentLoaded() const
{
    bool result = true;
    for (auto& e : *this)
    {
        const auto material = e.Material.Get();
        if (material && !material->IsLoaded())
        {
            result = false;
            break;
        }
    }
    return result;
}

bool ModelInstanceEntries::ShouldSerialize(const void* otherObj) const
{
    if (!otherObj)
        return true;
    return !(*this == *(const ModelInstanceEntries*)otherObj);
}

void ModelInstanceEntries::Serialize(SerializeStream& stream, const void* otherObj)
{
    SERIALIZE_GET_OTHER_OBJ(ModelInstanceEntries);

    stream.JKEY("Entries");
    stream.StartArray();
    if (other && other->Count() == Count())
    {
        for (int32 i = 0; i < Count(); i++)
            stream.Object(&At(i), &other->At(i));
    }
    else
    {
        for (auto& e : *this)
            stream.Object(&e, nullptr);
    }
    stream.EndArray();
}

void ModelInstanceEntries::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    PROFILE_MEM(Graphics);
    const DeserializeStream& entriesData = stream[rapidjson_flax::Value(rapidjson::StringRef("Entries", 7))];
    CHECK(entriesData.IsArray());
    Resize(entriesData.Size());
    ModelInstanceEntry* entries = Get();
    for (int32 i = 0; i < Count(); i++)
    {
        entries[i].Deserialize((DeserializeStream&)entriesData[i], modifier);
    }
}

bool ModelInstanceEntry::operator==(const ModelInstanceEntry& other) const
{
    return Material.Get() == other.Material.Get() && ShadowsMode == other.ShadowsMode && Visible == other.Visible && ReceiveDecals == other.ReceiveDecals;
}

bool ModelInstanceEntries::IsValidFor(const Model* model) const
{
    // Just check amount of material slots
    ASSERT(model && model->IsInitialized());
    return model->MaterialSlots.Count() == Count();
}

bool ModelInstanceEntries::IsValidFor(const SkinnedModel* model) const
{
    // Just check amount of material slots
    ASSERT(model && model->IsInitialized());
    return model->MaterialSlots.Count() == Count();
}

void ModelInstanceEntries::Setup(const Model* model)
{
    ASSERT(model && model->IsInitialized());
    const int32 slotsCount = model->MaterialSlots.Count();
    Setup(slotsCount);
}

void ModelInstanceEntries::Setup(const SkinnedModel* model)
{
    ASSERT(model && model->IsInitialized());
    const int32 slotsCount = model->MaterialSlots.Count();
    Setup(slotsCount);
}

void ModelInstanceEntries::Setup(int32 slotsCount)
{
    PROFILE_MEM(Graphics);
    Clear();
    Resize(slotsCount);
}

void ModelInstanceEntries::SetupIfInvalid(const Model* model)
{
    if (!IsValidFor(model))
    {
        Setup(model);
    }
}

void ModelInstanceEntries::SetupIfInvalid(const SkinnedModel* model)
{
    if (!IsValidFor(model))
    {
        Setup(model);
    }
}
