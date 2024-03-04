//                                                        UI System
//                                                    writen by Nori_SC
//                                                https://github.com/NoriteSC
#include "Engine/UI/Experimental/Types/UIComponent.h"
#include "Engine/UI/Experimental/Types/UIPanelSlot.h"
#include "Engine/UI/Experimental/Types/UIPanelOrderedSlot.h"
#include "Engine/UI/Experimental/Types/UIPanelComponent.h"
#include "Engine/Scripting/ScriptingObject.h"
#include "Engine/Core/Collections/Sorting.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Render2D/Render2D.h"
#include <Engine/UI/Experimental/Types/Anchor.h>
#include <Engine/Core/Log.h>

UIPanelComponent::UIPanelComponent(const SpawnParams& params) : UIComponent(params) 
{
    CanHaveMultipleChildren = true;
}

int32 UIPanelComponent::GetChildrenCount() const
{
    return Slots.Count();
}

UIComponent* UIPanelComponent::GetChildAt(int32 Index) const {

    if (Slots.IsValidIndex(Index))
    {
        if (UIPanelSlot* ChildSlot = Slots[Index])
        {
            return ChildSlot->Content;
        }
    }
    return nullptr;
}

Array<UIComponent*> UIPanelComponent::GetAllChildren() const
{
    Array<UIComponent*> Result;

    for (UIPanelSlot* ChildSlot : Slots)
    {
        Result.Add(ChildSlot->Content);
    }
    return Result;
}

int32 UIPanelComponent::GetChildIndex(const UIComponent* Content) const
{
    for (auto i = 0; i < Slots.Count(); i++)
    {
        UIPanelSlot* ChildSlot = Slots[i];
        if (ChildSlot->Content == Content)
        {
            return i;
        }
    }
    return -1;
}

/// <summary>
/// Returns true if panel contains this UIComponent
/// </summary>
/// <param name="Content"></param>
/// <returns></returns>

bool UIPanelComponent::HasChild(UIComponent* Content) const
{
    if (!Content)
    {
        return false;
    }
    return (Content->GetParent() == this);
}

/// <summary>
/// Removes a child by it's index.
/// </summary>
/// <param name="Index"></param>
/// <returns></returns>

bool UIPanelComponent::RemoveChildAt(int32 Index)
{
    if (Slots.IsValidIndex(Index))
    {
        UIPanelSlot* PanelSlot = Slots[Index];
        if (PanelSlot->Content)
        {
            PanelSlot->Content->Slot = nullptr;
        }

        Slots.RemoveAt(Index);

        OnSlotRemoved(PanelSlot);

        return true;
    }
    return false;
}

/// <summary>
/// Adds a new child UIComponent to the container.  Returns the base slot type,
/// requires casting to turn it into the type specific to the container.
/// </summary>
/// <param name="Content"></param>
/// <returns></returns>

UIPanelSlot* UIPanelComponent::AddChild(UIComponent* Content)
{
    if (Content == nullptr)
        return nullptr;
    if (!CanAddMoreChildren())
        return nullptr;

    Content->RemoveFromParent();
    UIPanelSlot* PanelSlot = (UIPanelSlot*)ScriptingObject::NewObject(GetSlotClass());
    PanelSlot->Content = Content;
    PanelSlot->Parent = this;

    Content->Slot = PanelSlot;

    Slots.Add(PanelSlot);

    OnSlotAdded(PanelSlot);

    return PanelSlot;
}

ScriptingTypeInitializer& UIPanelComponent::GetSlotClass() const
{
    return UIPanelSlot::TypeInitializer;
}

bool UIPanelComponent::ReplaceChildAt(int32 Index, UIComponent* Content)
{
    throw "Not implemented yet";
    return false;
}

bool UIPanelComponent::ReplaceChild(UIComponent* CurrentChild, UIComponent* NewChild)
{
    throw "Not implemented yet";
    return false;
}

UIPanelSlot* UIPanelComponent::InsertChildAt(int32 Index, UIComponent* Content)
{
    if (Content == nullptr)
        return nullptr;
    if (!CanAddMoreChildren())
        return nullptr;

    Content->RemoveFromParent();
    UIPanelSlot* PanelSlot = (UIPanelSlot*)ScriptingObject::NewObject(GetSlotClass());
    PanelSlot->Content = Content;
    PanelSlot->Parent = this;

    Content->Slot = PanelSlot;

    Slots.Insert(Index,PanelSlot);

    OnSlotAdded(PanelSlot);
    return PanelSlot;
}

bool UIPanelComponent::ShiftChild(int32 Index, UIComponent* Child)
{
    if (Child == nullptr) 
    {
        LOG(Warning, "Child {0} is null");
        return false;
    }

    if (HasChild(Child))
    {
        if (Slots.IsValidIndex(Index))
        {
            if (Slots[Index]->Content == nullptr) 
            {
                return true;
            }
            else
            {
                LOG(Warning, "Slot {0} is occupied on parent {1}", Index, Label);
            }
        }
        else
        {
            LOG(Warning, "Can't shift child index is {0} out of range", Index);
        }
    }
    else
    {
        LOG(Warning, "Can't shift child the child {0} with has different parent", Child->Label);
    }
    return false;
}

bool UIPanelComponent::RemoveChild(UIComponent* Child)
{
    if (Child == nullptr)
    {
        LOG(Warning, "Child {0} is null");
        return false;
    }

    if (HasChild(Child))
    {
        return RemoveChildAt(Slots.Find(Child->Slot));
    }
    else
    {
        LOG(Warning, "Can't remove child {0} with has different parent", Child->Label);
    }
    return false;
}

bool UIPanelComponent::HasAnyChildren() const
{
    return GetChildrenCount() != 0;;
}

void UIPanelComponent::ClearChildren()
{
    while (!Slots.IsEmpty())
    {
        Slots.Peek()->Content->RemoveFromParent();
    }
}

const Array<UIPanelSlot*> UIPanelComponent::GetSlots() const
{
    Array<UIPanelSlot*> out;
    out.Resize(Slots.Count(), false);
    for (auto i = 0; i < Slots.Count(); i++)
    {
        out[i] = Slots[i];
    }
    return out;
}

/// <summary>
/// </summary>
/// <returns>true if the panel supports more than one child.</returns>

bool UIPanelComponent::GetCanHaveMultipleChildren() const
{
    return CanHaveMultipleChildren;
}

/// <summary>
/// </summary>
/// <returns>true if the panel can accept another child UIComponent.</returns>

bool UIPanelComponent::CanAddMoreChildren() const
{
    return CanHaveMultipleChildren || GetChildrenCount() == 0;
}

void UIPanelComponent::Layout(const Rectangle& InNewBounds, const Float2& InNewPoivt)
{
    Vector2 locationDiff = InNewBounds.Location - GetTranslation();

    for (auto i = 0; i < Slots.Count(); i++)
    {
        Rectangle& newr = Rectangle(Slots[i]->Content->GetTranslation() + locationDiff, Slots[i]->Content->GetSize());
        UIPanelComponent::Layout(newr, Slots[i]->Content->GetPivot(), InNewBounds, Slots[i]);
    }

    SetRect_Internal(InNewBounds);
    SetPivot_Internal(InNewPoivt);
}
void UIPanelComponent::Layout(const Rectangle& InNewBounds, const Float2& InNewPoivt, const Rectangle& InNewParentBounds, UIPanelSlot* InFor)
{
    InFor->Layout(InNewBounds, InNewPoivt, InNewParentBounds);
}

void UIPanelComponent::Layout(const Rectangle& InNewBounds, const Float2& InNewPoivt, UIPanelSlot* InFor)
{
    InFor->Layout(InNewBounds, InNewPoivt,GetRect());
}

void UIPanelComponent::Render()
{
    DrawInternal();

    //get all slots
    Array<UIPanelSlot*> slots = GetSlots();
    {
        bool needsToOrderSlots = GetSlotClass().IsSubclassOf(UIPanelOrderedSlot::TypeInitializer);
        if (needsToOrderSlots)
        {
            //cost N * (N / 2)

            for (int i = 0; i < slots.Count(); i++)
            {
                bool done = true;
                for (int j = i; j < slots.Count(); j++)
                {
                    if (((UIPanelOrderedSlot*)slots[i])->ZOrder < ((UIPanelOrderedSlot*)slots[j])->ZOrder)
                    {
                        auto temp = slots[i];
                        slots[i] = slots[j];
                        slots[j] = temp;
                        done = false;
                    }
                }
                if (done) break;
            }
        }
    }
    if (Clipping == ClipToBounds)
    {
        Render2D::PushClip(GetRect());
        for (auto i = 0; i < slots.Count(); i++)
        {
#if !USE_EDITOR
            if (!slots[i]->Content->IsVisible()) // faster skip 
                continue;
#endif
            if (CanCast(slots[i]->Content->GetClass(), UIPanelComponent::GetStaticClass()))
            {
                UIPanelComponent* panel = ((UIPanelComponent*)slots[i]->Content);
                //panels are not Drawable by design so there is no call to DrawInternal
                //but panels need to sort order the children for drawing with teaks time
                panel->Render();
            }
            else
            {
                //by design the UI components cannot have children so this is the end of recursive call
                slots[i]->Content->DrawInternal();
            }
        }
        Render2D::PopClip();
        return;
    }
    else
    {
        for (auto i = 0; i < slots.Count(); i++)
        {
            if (CanCast(slots[i]->Content->GetClass(), UIPanelComponent::GetStaticClass()))
            {
                UIPanelComponent* panel = ((UIPanelComponent*)slots[i]->Content);
                //panelds are not Drawable so there is no DrawInternalCall
                panel->Render();
            }
            else
            {
                Slots[i]->Content->DrawInternal();
            }
        }
    }
}
