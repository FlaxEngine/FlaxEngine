// Writen by Nori_SC
#include "../UIComponent.h"
#include "../UIPanelComponent.h"
#include "Engine/Core/Memory/Memory.h"
#include "Engine/Debug/DebugLog.h"
#include "Engine/Engine/Screen.h"
#include "Engine/Render2D/Render2D.h"

#include "Engine/Serialization/ISerializeModifier.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Serialization/JsonWriters.h"
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
    Visibility = (UIComponentVisibility)(Visible | HitSelf);
    IsVariable = false;
    IsVolatile = false;
    OverrideCursor = false;
    Cursor = CursorType::Default;
    //compute defaultflagsvalue for seralization discard
    defaultflagsvalue = GetCompactedFlags();
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
        stream.JKEY("Transform"); stream.Blob(&floats[0], sizeof(floats));
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
        Encryption::Base64Decode(transform->value.GetString(), sizeof(floats), (byte*)&floats[0]);
        Transform.Rect.Location.X = floats[0];
        Transform.Rect.Location.Y = floats[1];
        Transform.Rect.Size.X = floats[2];
        Transform.Rect.Size.Y = floats[3];
        Transform.Shear.X = floats[4];
        Transform.Shear.Y = floats[5];
        Transform.Angle = floats[6];
        Transform.Pivot.X = floats[7];
        Transform.Pivot.Y = floats[8];
    }
    DESERIALIZE(RenderOpacity);

    UpdateTransform();
    CreatedByUIBlueprint = true;
}


#pragma region GettersSetters

void UIComponent::SetClipping(const UIComponentClipping& InClipping)
{
    Clipping = InClipping;
}

const UIComponentClipping& UIComponent::GetClipping() const
{
    return Clipping;
}

void UIComponent::SetTransform(const UIComponentTransform& InTransform)
{
    Transform = InTransform; if (!IsVolatile) {
        UpdateTransform();
    }
}

const UIComponentTransform& UIComponent::GetTransform() const
{
    return Transform;
}

void UIComponent::SetTranslation(const Vector2& InTranslation)
{
    Transform.Rect.Location = InTranslation;
    if (!IsVolatile)
        UpdateTransform();
}

const Vector2& UIComponent::GetTranslation() const
{
    return Transform.Rect.Location;
}

void UIComponent::SetSize(const Vector2& InSize)
{
    Transform.Rect.Size = InSize;
    if (!IsVolatile)
        UpdateTransform();
}

const Vector2& UIComponent::GetSize() const
{
    return Transform.Rect.Size;
}

void UIComponent::SetShear(const Vector2& InShear)
{
    Transform.Shear = InShear;
    if (!IsVolatile)
        UpdateTransform();
}

const Vector2& UIComponent::GetShear() const
{
    return Transform.Shear;
}

void UIComponent::SetAngle(float InAngle) {
    Transform.Angle = InAngle; 
    if (!IsVolatile) 
        UpdateTransform();
}

float UIComponent::GetAngle() const
{
    return Transform.Angle;
}

void UIComponent::SetPivot(const Vector2& InPivot)
{
    Transform.Pivot = InPivot; 
    if (!IsVolatile) 
        UpdateTransform();
}

const Vector2& UIComponent::GetPivot() const
{
    return Transform.Pivot;
}

void UIComponent::SetIsEnabled(bool InIsEnabled)
{
    IsEnabled = InIsEnabled;
} 

bool UIComponent::GetIsEnabled() const
{
    return IsEnabled;
}

void UIComponent::SetToolTipText(const String& InToolTipText)
{
    throw "Not implemented yet";
}

const String& UIComponent::GetToolTipText() const
{
    throw "Not implemented yet";
    return String();
}

UIComponent* UIComponent::GetToolTip() const
{
    throw "Not implemented yet";
    return nullptr;
}

void UIComponent::SetToolTip(UIComponent* UIComponent)
{
    throw "Not implemented yet";
}

void UIComponent::SetCursor(const CursorType& InCursor)
{
    Cursor = InCursor;
}

const CursorType& UIComponent::GetCursor() const
{
    throw "Not implemented yet";
}

const UIComponentVisibility& UIComponent::GetVisibility() const
{
    return Visibility;
}

void UIComponent::SetVisibility(const UIComponentVisibility& InVisibility)
{
    Visibility = InVisibility;
}
#pragma endregion

void UIComponent::ResetCursor()
{
    throw "Not implemented yet";
}

bool UIComponent::IsRendered() const
{
    return IsVisible() && RenderOpacity > 0;
}

bool UIComponent::IsVisible() const
{
    return EnumHasAnyFlags(Visibility,(UIComponentVisibility)(Visible | HitChildren | HitSelf));
}

Object* UIComponent::GetSourceAssetOrClass() const
{
    return nullptr;
}
UIComponent* UIComponent::RebuildUIComponent()
{
    DebugLog::LogError(String(L"The Rebuild UI Component is on implemented by Parent class\n").Append(DebugLog::GetStackTrace()));
    return nullptr;
}

void UIComponent::OnUIComponentRebuilt()
{
}

void UIComponent::UpdateTransform()
{
    // Actual pivot and negative pivot
    auto v1 = Transform.Pivot;
    auto v2 = -v1;
    v1 += Transform.Rect.Location;

    // Size and Shear
    Matrix3x3 m1 = Matrix3x3
    (
        Transform.Rect.Size.X,
        Transform.Rect.Size.X * (Transform.Shear.Y == 0 ? 0 : (1.0f / Math::Tan(DegreesToRadians * (90 - Math::Clamp(Transform.Shear.Y, -89.0f, 89.0f))))),
        0,
        Transform.Rect.Size.Y * (Transform.Shear.X == 0 ? 0 : (1.0f / Math::Tan(DegreesToRadians * (90 - Math::Clamp(Transform.Shear.X, -89.0f, 89.0f))))),
        Transform.Rect.Size.Y,
        0, 0, 0, 1
    );
    //rotate and mix
    float sin = Math::Sin(DegreesToRadians * Transform.Angle);
    float cos = Math::Cos(DegreesToRadians * Transform.Angle);
    m1.M11 = (Transform.Rect.Size.X * cos) + (m1.M12 * -sin);
    m1.M12 = (Transform.Rect.Size.X * sin) + (m1.M12 * cos);
    float m21 = (m1.M21 * cos) + (Transform.Rect.Size.Y * -sin);
    m1.M22 = (m1.M21 * sin) + (Transform.Rect.Size.Y * cos);
    m1.M21 = m21;

    // Mix all the stuff
    m1.M31 = (v2.X * m1.M11) + (v2.Y * m1.M21) + v1.X;
    m1.M32 = (v2.X * m1.M12) + (v2.Y * m1.M22) + v1.Y;
    CachedTransform = m1;

    // Cache inverted transform
    Matrix3x3::Invert(CachedTransform, CachedTransformInv);
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

bool UIComponent::IsEditorUIComponent() const
{
    return false;
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
const String& UIComponent::GetDisplayLabel() const
{
    return DisplayLabel;
}

void UIComponent::SetDisplayLabel(const String& InDisplayLabel)
{
    DisplayLabel = InDisplayLabel;
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

void UIComponent::SelectByDesigner()
{
    //[ToDo] Enable Outlide
    OnSelectedByDesigner();
}

void UIComponent::DeselectByDesigner()
{
    //[ToDo] Disable Outlide
    OnDeselectedByDesigner();
}
#endif

//---------------------------------------------------------------------------------------------------
//--------------------------------------------Internal-----------------------------------------------
//---------------------------------------------------------------------------------------------------


void UIComponent::DrawInternal()
{
    if (IsVolatile)
    {
        //as stated in IsVolatile comment the Matrix3x3 are not cached
        //there are computed every frame
        UpdateTransform();
    }
    Render2D::PushTransform(CachedTransform);
    OnDraw();
    Render2D::PopTransform();
}
void UIComponent::InvalidateLayout(const UIPanelSlot* InFor)
{
    if (IsVolatile) //as stated in IsVolatile comment the Layout is not cached layout is refreshed evry frame
        return;
    //Call to parent to update layout
    GetParent()->InvalidateLayout(Slot);
}
