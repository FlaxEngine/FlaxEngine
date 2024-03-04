//                                                        UI System
//                                                    writen by Nori_SC
//                                                https://github.com/NoriteSC
#include "../Image.h"
#include "Engine/Serialization/ISerializeModifier.h"
#include "Engine/Serialization/Serialization.h"
#include <Engine/UI/Experimental/Brushes/ImageBoxBrush.h>

UIImage::UIImage(const SpawnParams& params) : UIComponent(params)
{
    Brush = New<UIImageBoxBrush>();
}

void UIImage::OnDraw()
{
    if (Brush)
        Brush->Draw(GetRect());
}

void UIImage::Serialize(SerializeStream& stream, const void* otherObj)
{
    UIComponent::Serialize(stream, otherObj);
    SERIALIZE_GET_OTHER_OBJ(UIImage);
    SERIALIZE(Brush);
}

void UIImage::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    DESERIALIZE(Brush)
}
