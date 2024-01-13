// Writen by Nori_SC
#include "../UIComponent.h"
#include "../UIPanelComponent.h"
#include "Engine/Core/Memory/Memory.h"
#include "Engine/Debug/DebugLog.h"
#include "Engine/Engine/Screen.h"
#include "Engine/Render2D/Render2D.h"

UIComponent::UIComponent(const SpawnParams& params) : ScriptingObject(params) {}

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
    Transform.Translation = InTranslation;
    if (!IsVolatile)
        UpdateTransform();
}

const Vector2& UIComponent::GetTranslation() const
{
    return Transform.Translation;
}

void UIComponent::SetSize(const Vector2& InSize)
{
    Transform.Size = InSize;
    if (!IsVolatile)
        UpdateTransform();
}

const Vector2& UIComponent::GetSize() const
{
    return Transform.Size;
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
    throw "Not implemented yet";
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
    v1 += Transform.Translation;

    // Size and Shear
    Matrix3x3 m1 = Matrix3x3
    (
        Transform.Size.X,
        Transform.Size.X * (Transform.Shear.Y == 0 ? 0 : (1.0f / Math::Tan(DegreesToRadians * (90 - Math::Clamp(Transform.Shear.Y, -89.0f, 89.0f))))),
        0,
        Transform.Size.Y * (Transform.Shear.X == 0 ? 0 : (1.0f / Math::Tan(DegreesToRadians * (90 - Math::Clamp(Transform.Shear.X, -89.0f, 89.0f))))),
        Transform.Size.Y,
        0, 0, 0, 1
    );
    //rotate and mix
    float sin = Math::Sin(DegreesToRadians * Transform.Angle);
    float cos = Math::Cos(DegreesToRadians * Transform.Angle);
    m1.M11 = (Transform.Size.X * cos) + (m1.M12 * -sin);
    m1.M12 = (Transform.Size.X * sin) + (m1.M12 * cos);
    float m21 = (m1.M21 * cos) + (Transform.Size.Y * -sin);
    m1.M22 = (m1.M21 * sin) + (Transform.Size.Y * cos);
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
        Slot->Parent->RemoveChild(this);
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
