
//                                                        UI System
//                                                    writen by Nori_SC
//                                                https://github.com/NoriteSC
#pragma once
#include "../ImageBoxBrush.h"
#include "Engine/Render2D/Render2D.h"
#include "Engine/Serialization/ISerializeModifier.h"
#include "Engine/Serialization/Serialization.h"

UIImageBoxBrush::UIImageBoxBrush(const SpawnParams& params) : UIImageBrush(params)
{
    Margin = UIMargin();
}

void UIImageBoxBrush::SetMargin(const UIMargin& InUIMargin)
{
    Margin = InUIMargin;
}

const UIMargin& UIImageBoxBrush::GetMargin()
{
    return Margin;
}

void UIImageBoxBrush::Draw(const Rectangle& rect)
{
    if (Image.Get()) 
    {
        Render2D::DrawTexturedTriangles(Image.Get()->GetTexture(), Verts, Uvs);
    }
}

void UIImageBoxBrush::Serialize(SerializeStream& stream, const void* otherObj)
{
    UIImageBrush::Serialize(stream, otherObj);
    SERIALIZE_GET_OTHER_OBJ(UIImageBrush);
}

void UIImageBoxBrush::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    UIImageBrush::Deserialize(stream, modifier);
}
