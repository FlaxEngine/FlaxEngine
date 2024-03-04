//                                                        UI System
//                                                    writen by Nori_SC
//                                                https://github.com/NoriteSC
#include "../UIComponent.h"
#include "../UIPanelComponent.h"
#include "Engine/Core/Memory/Memory.h"
#include "Engine/Debug/DebugLog.h"
#include "Engine/Engine/Screen.h"
#include "Engine/Render2D/Render2D.h"

#include "Engine/Serialization/ISerializeModifier.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Serialization/JsonWriters.h"

#include "../../System/UISerializationConfig.h"
int defaultflagsvalue;

UIComponent::UIComponent(const SpawnParams& params) : ScriptingObject(params)
{
    Clipping = Inherit;
    Label = "";
    Transform = UIComponentTransform();
    RenderOpacity = 1.0f;
    Slot = nullptr;

    //flags
    IsEnabled = true;
    Visibility = Visible;
    IsVariable = false;
    IsVolatile = false;
    OverrideCursor = false;
    Cursor = CursorType::Default;
    //compute defaultflagsvalue for seralization discard
    defaultflagsvalue = GetCompactedFlags();

    DesignerFlags = UIComponentDesignFlags::None;
}
FORCE_INLINE int UIComponent::GetCompactedFlags()
{
    // 10000000 00000000 00000000 00000000 = value | IsVolatile     << 0        bit 0
    // 01000000 00000000 00000000 00000000 = value | IsVariable     << 1        bit 1
    // 00100000 00000000 00000000 00000000 = value | OverrideCursor << 2        bit 2
    // 00010000 00000000 00000000 00000000 = value | IsEnabled      << 3        bit 3
    // 00001100 00000000 00000000 00000000 = value | Clipping       << 4        bits 4-5 (value range 0 to 3)
    // 00000011 11000000 00000000 00000000 = value | Visibility     << 6        bits 6-9 (value range 0 to 15)
    // 00000000 00111100 00000000 00000000 = value | Cursor         << 10       bits 10-14 (value range 0 to 15)
    
    //free bits                   00000000 00000011 11111111 11111111           bits 15-32 (value range 0 to 262143)

    return ((int)IsVolatile) | IsVariable << 0b1 | OverrideCursor << 0b10 |
        IsEnabled << 0b11 | Clipping << 0b100 | Visibility << 0b110 | ((int)Cursor) << 0b1010;
}
void UIComponent::Serialize(SerializeStream& stream, const void* otherObj)
{
    SERIALIZE_GET_OTHER_OBJ(UIComponent);
    if (!IsGeneratedName()) 
    {
        SERIALIZE(Label);
    }
    int32 flags = GetCompactedFlags();
    if(defaultflagsvalue != flags)
    {
        // pack flags and bools in to 1 value (lowers amaut of charaters in text file)
        stream.JKEY("Flags");
        stream.Int(flags);
    }
    if (!Transform.IsIdentity())
    {
        //custom packing for transform
        float floats[9] =
        {
            Transform.Rect.Location.X,
            Transform.Rect.Location.Y,
            Transform.Rect.Size.X,
            Transform.Rect.Size.Y,
            Transform.Shear.X,
            Transform.Shear.Y,
            Transform.Angle,
            Transform.Pivot.X,
            Transform.Pivot.Y
        };
#if UI_USE_COMPACT_SERIALIZATION
        stream.Blob(&floats[0], sizeof(floats));
#else
        stream.JKEY("Transform");
        stream.StartArray();
        for (size_t i = 0; i < 9; i++)
        {
            stream.Float(floats[i]);
        }
        stream.EndArray();
#endif
    }
    if (RenderOpacity != 1.0f) 
    {
        SERIALIZE(RenderOpacity);
    }
}

void UIComponent::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    DESERIALIZE(Label);
    int32 value = 0;
    const auto flags = stream.FindMember(rapidjson_flax::Value("Flags", (sizeof("Flags") / sizeof("Flags"[0])) - 1));
    if (flags != stream.MemberEnd())
    {
        Serialization::Deserialize(flags->value, value, modifier);

        //                    packed          bit mask                               remove offset
        //IsVolatile        = value &         10000000 00000000 00000000 00000000 >> 0                  bit 0
        //IsVariable 		= value &         01000000 00000000 00000000 00000000 >> 1                  bit 1
        //OverrideCursor    = value &         00100000 00000000 00000000 00000000 >> 2                  bit 2
        //IsEnabled         = value &         00010000 00000000 00000000 00000000 >> 3                  bit 3
        //Clipping          = value &         00001100 00000000 00000000 00000000 >> 4                  bits 4-5 (value range 0 to 3)
        //Visibility        = value &         00000011 11000000 00000000 00000000 >> 6                  bits 6-9 (value range 0 to 15)
        //Cursor            = value &         00000000 00111100 00000000 00000000 >> 10                 bits 10-14 (value range 0 to 15)
        //free bits                           00000000 00000011 11111111 11111111                       bits 15-32 (value range 0 to 262143)

        IsVolatile = (value & 0b1);
        IsVariable = (value & 0b10) >> 0b1;
        OverrideCursor = (value & 0b100) >> 0b10;
        IsEnabled = (value & 0b1000) >> 0b11;
        Clipping = (UIComponentClipping)((value & 0b110000) >> 0b100);
        Visibility = (UIComponentVisibility)((value & 0b1111000000) >> 0b110);
        Cursor = (CursorType)((value & 0b11110000000000) >> 0b1010);
    }
    const auto transform = stream.FindMember(rapidjson_flax::Value("Transform", (sizeof("Transform") / sizeof("Transform"[0])) - 1));
    if (transform != stream.MemberEnd())
    {
        //custom unpacking for transform
        float floats[9] = { 0,0,0,0,0,0,0,0,0 };
        if (transform->value.IsString()) 
        {

            Encryption::Base64Decode(transform->value.GetString(), sizeof(floats), (byte*)&floats[0]);

        }
        else
        {
            auto ar = transform->value.GetArray();
            for (uint32 i = 0; i < ar.Size(); i++)
            {
                floats[i] = ar[i].GetFloat();
            }
        }
        Transform.Rect.Location.X = (floats[0]);
        Transform.Rect.Location.Y = (floats[1]);
        Transform.Rect.Size.X = (floats[2]);
        Transform.Rect.Size.Y = (floats[3]);
        Transform.Shear.X = floats[4];
        Transform.Shear.Y = floats[5];
        Transform.Angle = floats[6];
        Transform.Pivot.X = floats[7];
        Transform.Pivot.Y = floats[8];
    }
    DESERIALIZE(RenderOpacity);

    Transform.UpdateTransform();
    CreatedByUIBlueprint = true;
}


#pragma region GettersSetters
inline bool UIComponent::GetIsEnabled() const {return IsEnabled;}
inline const UIComponentClipping& UIComponent::GetClipping() const{return Clipping;}
inline const UIComponentTransform& UIComponent::GetTransform() const {return Transform;}
inline float UIComponent::GetTop() const { return Transform.Rect.GetTop(); }
inline float UIComponent::GetBottom() const { return Transform.Rect.GetBottom(); }
inline float UIComponent::GetLeft() const { return Transform.Rect.GetLeft(); }
inline float UIComponent::GetRight() const { return Transform.Rect.GetRight(); }
inline Float2 UIComponent::GetCenter() const { return Transform.Rect.GetCenter(); }
inline const Rectangle& UIComponent::GetRect() const { return Transform.Rect; }
inline const Vector2& UIComponent::GetTranslation() const { return Transform.Rect.Location; }
inline const Vector2& UIComponent::GetSize() const { return Transform.Rect.Size; }
inline const Vector2& UIComponent::GetShear() const { return Transform.Shear; }
inline float UIComponent::GetAngle() const { return Transform.Angle; }
inline const Vector2& UIComponent::GetPivot() const { return Transform.Pivot; }
inline const CursorType& UIComponent::GetCursor() const { return Cursor; }
inline const UIComponentVisibility& UIComponent::GetVisibility() const { return Visibility; }

void UIComponent::SetRect(const Rectangle& InRectangle)
{
    Layout(InRectangle, Transform.Pivot);
    if (!IsVolatile)
    {
        Transform.UpdateTransform();
    }
}

void UIComponent::SetRect_Internal(const Rectangle& InRectangle)
{
    Transform.Rect = InRectangle;
}
void UIComponent::SetPivot_Internal(const Vector2& InNewPoivt)
{
    Transform.Pivot = InNewPoivt;
}

void UIComponent::SetClipping(const UIComponentClipping& InClipping)
{
    Clipping = InClipping;
}

void UIComponent::SetTransform(const UIComponentTransform& InTransform)
{
    Layout(InTransform.Rect, Transform.Pivot);
    Transform.Shear = InTransform.Shear;
    Transform.Angle = InTransform.Angle;
    Transform.Pivot = InTransform.Pivot;
    if (!IsVolatile)
    {
        Transform.UpdateTransform();
    }
}

void UIComponent::SetCenter(const Float2& value)
{
    auto& r = Rectangle(Transform.Rect);
    r.SetCenter(value);
    Layout(r, Transform.Pivot);
    if (!IsVolatile)
        Transform.UpdateTransform();
}

void UIComponent::SetTop(float value)
{
    auto& r = Rectangle(Transform.Rect);
    r.SetTop(value);
    Layout(r, Transform.Pivot);
    if (!IsVolatile)
        Transform.UpdateTransform();
}

void UIComponent::SetLeft(float value)
{
    auto& r = Rectangle(Transform.Rect);
    r.SetLeft(value);
    Layout(r, Transform.Pivot);
    if (!IsVolatile)
        Transform.UpdateTransform();
}

void UIComponent::SetBottom(float value)
{
    auto& r = Rectangle(Transform.Rect);
    r.SetBottom(value);
    Layout(r, Transform.Pivot);
    if (!IsVolatile)
        Transform.UpdateTransform();
}

void UIComponent::SetRight(float value)
{
    auto& r = Rectangle(Transform.Rect);
    r.SetRight(value);
    Layout(r, Transform.Pivot);
    if (!IsVolatile)
        Transform.UpdateTransform();
}

void UIComponent::SetTranslation(const Vector2& InTranslation)
{
    Layout(Rectangle(InTranslation, Transform.Rect.Size), Transform.Pivot);
    if (!IsVolatile)
        Transform.UpdateTransform();
}

void UIComponent::SetSize(const Vector2& InSize)
{
    Layout(Rectangle(Transform.Rect.Location, InSize),Transform.Pivot);
    if (!IsVolatile)
        Transform.UpdateTransform();
}

void UIComponent::SetShear(const Vector2& InShear)
{
    Transform.Shear = InShear;
    if (!IsVolatile)
        Transform.UpdateTransform();
}

void UIComponent::SetAngle(float InAngle) {
    Transform.Angle = InAngle; 
    if (!IsVolatile) 
        Transform.UpdateTransform();
}

void UIComponent::SetPivot(const Vector2& InPivot)
{
    Layout(Transform.Rect, InPivot);
    if (!IsVolatile) 
        Transform.UpdateTransform();
}

void UIComponent::SetIsEnabled(bool InIsEnabled)
{
    IsEnabled = InIsEnabled;
}

void UIComponent::SetToolTipText(const String& InToolTipText)
{
    LOG(Warning, "Not implemented yet");
}

const String& UIComponent::GetToolTipText() const
{
    LOG(Warning,"Not implemented yet");
    return TEXT("");
}

UIComponent* UIComponent::GetToolTip() const
{
    LOG(Warning, "Not implemented yet");
    return nullptr;
}

void UIComponent::SetToolTip(UIComponent* UIComponent)
{
    LOG(Warning, "Not implemented yet");
}

void UIComponent::SetCursor(const CursorType& InCursor)
{
    Cursor = InCursor;
}

void UIComponent::SetVisibility(const UIComponentVisibility& InVisibility)
{
    Visibility = InVisibility;
}
#pragma endregion

void UIComponent::ResetCursor()
{
    LOG(Warning, "Not implemented yet");
}

bool UIComponent::IsRendered() const
{
    return IsVisible() && RenderOpacity > 0;
}

bool UIComponent::IsVisible() const
{
    return !EnumHasAnyFlags(Visibility,(UIComponentVisibility)(Hiden));
}

UIPanelComponent* UIComponent::GetParent() const
{
    if (Slot)
    {
        return Slot->Parent;
    }

    return nullptr;
}

void UIComponent::RemoveFromParent()
{
    if (Slot)
    {
        if (Slot->Parent) 
        {
            Slot->Parent->RemoveChild(this);
            
        }
    }
}

#if USE_EDITOR

bool UIComponent::IsLockedInDesigner() const
{
    return LockedInDesigner;
}

void UIComponent::SetLockedInDesigner(bool NewLockedInDesigner)
{
    LockedInDesigner = NewLockedInDesigner;
}

UIComponentVisibility UIComponent::GetVisibilityInDesigner() const
{
    return HiddenInDesigner ? UIComponentVisibility::Collapsed : UIComponentVisibility::Visible;
}

bool UIComponent::IsDesignTime() const
{
    return HasAnyDesignerFlags(UIComponentDesignFlags::Designing);
}

bool UIComponent::HasAnyDesignerFlags(UIComponentDesignFlags FlagsToCheck) const
{
    return EnumHasAnyFlags(DesignerFlags, FlagsToCheck);
}

bool UIComponent::IsPreviewTime() const
{
    return HasAnyDesignerFlags(UIComponentDesignFlags::Previewing);
}

bool UIComponent::IsGeneratedName() const
{
    return Label.IsEmpty();
}

String UIComponent::GetLabel() const
{
    return Label;
}

bool UIComponent::IsVisibleInDesigner() const
{
    if (HiddenInDesigner)
    {
        return false;
    }
    
    auto Parent = GetParent();
    while (Parent != nullptr)
    {
        if (Parent->HiddenInDesigner)
        {
            return false;
        }

        Parent = Parent->GetParent();
    }

    return true;
}

void UIComponent::Select()
{
    //SetFlag
    DesignerFlags = (UIComponentDesignFlags)((int)DesignerFlags | (int)UIComponentDesignFlags::ShowOutline);
    //[ToDo] Enable Outlide
    OnSelectedByDesigner();
}

void UIComponent::Deselect()
{
    //UnsetFlag
    DesignerFlags = (UIComponentDesignFlags)((int)DesignerFlags & ~(int)UIComponentDesignFlags::ShowOutline);
    //[ToDo] Disable Outlide
    OnDeselectedByDesigner();
}
#endif
void UIComponent::OnDraw(){}

//---------------------------------------------------------------------------------------------------
//--------------------------------------------Internal-----------------------------------------------
//---------------------------------------------------------------------------------------------------



void UIComponent::Layout(const Rectangle& InNewBounds, const Vector2& InNewPoivt)
{
    if (GetParent())
    {
        //Call to parent to update the layout for this element
        //in some ceses it might just set the InNewSize value
        GetParent()->Layout(InNewBounds, InNewPoivt, Slot);
    }
}

void UIComponent::DrawInternal()
{
    if (IsVolatile)
    {
        //as stated in IsVolatile comment the Matrix3x3 are not 'cached'
        //there are computed every frame
        Transform.UpdateTransform();
    }
    Render2D::PushTransform(Transform.CachedTransform);
#if USE_EDITOR
    if (IsDesignTime())
    {
        if (HasAnyDesignerFlags(UIComponentDesignFlags::ShowOutline))
        {
            Render2D::DrawRectangle(Transform.Rect, Color::Green, 2);
        }
        else
        {
            Render2D::DrawRectangle(Transform.Rect, Color::Gray, 2);
        }
    }
#endif
    if (IsVisible())
        OnDraw();
    Render2D::PopTransform();
}

void UIComponent::InvalidateLayout()
{
    if (GetParent())
    {
        //Call to parent to update the layout for this element
        //in some ceses it might just set the InNewSize value
        GetParent()->Layout(GetRect(), GetPivot(), Slot);
    }
}
