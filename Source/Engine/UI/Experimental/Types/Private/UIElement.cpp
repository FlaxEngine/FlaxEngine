#include "Engine/Core/Memory/Memory.h"
#include "Engine/UI/Experimental/Types/UIElement.h"

void UIElement::Detach()
{
    if (Parent) 
    {
        Parent->RemoveChild(this);
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
void UIElement::OnScriptingDispose()
{
    Detach();
    OnDestruct();
}
void UIElement::OnDeleteObject()
{
    SAFE_DELETE(RenderTransform);
}
