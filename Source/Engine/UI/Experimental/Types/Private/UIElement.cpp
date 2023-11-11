#include "Engine/Core/Memory/Memory.h"
#include "Engine/UI/Experimental/Types/UIElement.h"

void UIElement::Detach()
{
    if (Slot) 
    {
        Slot->RemoveChild(this);
    }
    else
    {
        DebugLog::LogWarning(StringView(TEXT("Failed to Detach from ISlot, the UIElement do not have parent")));
    }
}
void UIElement::Attach(ISlot* To)
{
    if (To)
    {
        To->AddChild(this);
    }
    else
    {
        DebugLog::LogWarning(StringView(TEXT("Failed to Attach ISlot, ISlot* To == nullptr")));
    }
}
UIElement::UIElement(const SpawnParams& params) : ScriptingObject(params)
{
    RenderTransform = New<UIRenderTransform>();
    OnPreCunstruct(false);
}

UIElement::UIElement(const SpawnParams& params, bool isInDesigner) : ScriptingObject(params)
{
    RenderTransform = New<UIRenderTransform>();
    OnPreCunstruct(true);
}

void UIElement::OnPreCunstruct(bool isInDesigner){}
void UIElement::OnCunstruct(){}
void UIElement::OnDestruct(){}
void UIElement::OnDraw(){}

/// <summary>
/// Attach this to <see cref="ISlot"/>
/// </summary>

ISlot* UIElement::GetSlot()
{
    return Slot;
}
void UIElement::OnScriptingDispose()
{
    Detach();
    OnDestruct();
}
void UIElement::OnDeleteObject()
{
    SAFE_DELETE(RenderTransform);
}
