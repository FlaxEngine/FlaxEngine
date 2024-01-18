#pragma once

#include "Engine/Content/JsonAsset.h"
#include "Engine/Core/ISerializable.h"
#include "Engine/Core/Math/Vector2.h"
#include "Engine/Scripting/ScriptingType.h"
#include "Engine/Scripting/ScriptingType.h"
#include "../Types/UIPanelComponent.h"

API_CLASS(NoSpawn) class FLAXENGINE_API UIBlueprintAsset : public JsonAssetBase
{
    DECLARE_ASSET_HEADER(UIBlueprintAsset);

    API_FIELD() UIComponent* Component;

protected:
    virtual void OnGetData(rapidjson_flax::StringBuffer& buffer) const override;

    // [Asset]
    LoadResult loadAsset() override;
    void unload(bool isReloading) override;

private:
    static UIComponent* DeserializeComponent(ISerializable::DeserializeStream& stream, ISerializeModifier* modifier, Array<StringAnsiView>& Types);
    static void SerializeComponent(ISerializable::SerializeStream& stream, UIComponent* component, Array<StringAnsiView>& Types);
};
