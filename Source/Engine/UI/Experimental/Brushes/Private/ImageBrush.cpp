
//                                                        UI System
//                                                    writen by Nori_SC
//                                                https://github.com/NoriteSC
#pragma once
#include "../ImageBrush.h"
#include "Engine/Content/AssetReference.h"
#include "Engine/Render2D/Render2D.h"
#include "Engine/Serialization/ISerializeModifier.h"
#include "Engine/Serialization/Serialization.h"
#include <Engine/Core/Math/Vector2.h>

UIImageBrush::UIImageBrush(const SpawnParams& params) : UIBrush(params)
{
}

void UIImageBrush::Draw(const Rectangle& rect)
{
    Render2D::DrawTexture(Image.Get(), rect);
}

void UIImageBrush::Serialize(SerializeStream& stream, const void* otherObj)
{
    UIBrush::Serialize(stream, otherObj);
    SERIALIZE_GET_OTHER_OBJ(UIImageBrush);
    SERIALIZE(Image);
}

void UIImageBrush::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    UIBrush::Deserialize(stream, modifier);
    DESERIALIZE(Image);
}
