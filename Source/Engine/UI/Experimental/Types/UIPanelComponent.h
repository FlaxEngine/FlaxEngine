//                                                        UI System
//                                                    writen by Nori_SC
//                                                https://github.com/NoriteSC
#pragma once
#include "Engine/Scripting/ScriptingObject.h"
#include "Engine/Core/Collections/Array.h"
#include "UIPanelSlot.h"

class UIComponent;

API_CLASS(Namespace = "FlaxEngine.Experimental.UI")
class FLAXENGINE_API UIPanelComponent : public UIComponent
{
    DECLARE_SCRIPTING_TYPE(UIPanelComponent);

protected:

    /// <summary>
    /// The slots in the UIComponent holding the child UIComponents of this panel.
    /// </summary>
    Array<UIPanelSlot*> Slots;

public:

    /// <summary>
    /// Gets number of child UIComponents in the container.
    /// </summary>
    /// <returns></returns>
    API_FUNCTION() int32 GetChildrenCount() const;

    /// <summary>
    /// Gets the UIComponent at an index.
    /// </summary>
    /// <param name="Index">Index The index of the UIComponent.</param>
    /// <returns>The UIComponent at the given index, or nothing if there is no UIComponent there.</returns>
    API_FUNCTION() UIComponent* GetChildAt(int32 Index) const;

    /// <summary>
    /// Gets all UIComponents in the container
    /// </summary>
    /// <returns></returns>
    API_FUNCTION() Array<UIComponent*> GetAllChildren() const;

    /// <summary>
    ///  Gets the index of a specific child UIComponent
    /// </summary>
    /// <param name="Content"></param>
    /// <returns>index of a specific child UIComponent can return -1 if the UIComponent is not found</returns>
    API_FUNCTION() int32 GetChildIndex(const UIComponent* Content) const;;

    /// <summary>
    /// Returns true if panel contains this UIComponent
    /// </summary>
    /// <param name="Content"></param>
    /// <returns></returns>
    API_FUNCTION() bool HasChild(UIComponent* Content) const;;

    /// <summary>
    /// Removes a child by it's index.
    /// </summary>
    /// <param name="Index"></param>
    /// <returns></returns>
    API_FUNCTION() bool RemoveChildAt(int32 Index);;




    /// <summary>
    /// Adds a new child UIComponent to the container.  Returns the base slot type,
    /// requires casting to turn it into the type specific to the container.
    /// </summary>
    /// <param name="Content"></param>
    /// <returns></returns>
    API_FUNCTION() UIPanelSlot* AddChild(UIComponent* Content);;

    /// <summary>
    /// Swaps the UIComponent out of the slot at the given index, replacing it with a different UIComponent.
    /// </summary>
    /// <param name="Index"> Index the index at which to replace the child content with this new Content.</param>
    /// <param name="Content">Content The content to replace the child with.</param>
    /// <returns> true if the index existed and the child could be replaced.</returns>
    API_FUNCTION() bool ReplaceChildAt(int32 Index, UIComponent* Content);

    /// <summary>
    /// Swaps the child UIComponent out of the slot, and replaces it with the new child UIComponent.
    /// </summary>
    /// <param name="CurrentChild">CurrentChild The existing child UIComponent being removed.</param>
    /// <param name="NewChild">NewChild The new child UIComponent being inserted.</param>
    /// <returns>true if the CurrentChild was found and the swap occurred, otherwise false.</returns>
    API_FUNCTION() virtual bool ReplaceChild(UIComponent* CurrentChild, UIComponent* NewChild);

    /// <summary>
    /// Inserts a UIComponent at a specific index. This does not update the live slate version, it requires
    /// a rebuild of the whole UI to see a change.
    /// </summary>
    /// <param name="Index"></param>
    /// <param name="Content"></param>
    /// <returns></returns>
    API_FUNCTION() UIPanelSlot* InsertChildAt(int32 Index, UIComponent* Content);

    /// <summary>
    /// Moves the child UIComponent from its current index to the new index provided.
    /// </summary>
    /// <param name="Index"></param>
    /// <param name="Child"></param>
    API_FUNCTION() void ShiftChild(int32 Index, UIComponent* Child);

    /// <summary>
    ///  Removes a specific UIComponent from the container.
    /// </summary>
    /// <param name="Content"></param>
    /// <returns>true if the UIComponent was found and removed.</returns>
    API_FUNCTION() bool RemoveChild(UIComponent* Content);

    /// <summary>
    /// </summary>
    /// <returns>true if there are any child UIComponents in the panel</returns>
    API_FUNCTION() bool HasAnyChildren() const;

    /// <summary>
    /// Remove all child UIComponents from the panel.
    /// </summary>
    API_FUNCTION() virtual void ClearChildren();

    /// <summary>
    /// </summary>
    /// <returns>The slots in the UIComponent holding the child UIComponents of this panel.</returns>
    API_FUNCTION() const Array<UIPanelSlot*> GetSlots() const;

    /// <summary>
    /// </summary>
    /// <returns>true if the panel supports more than one child.</returns>
    API_FUNCTION()  bool GetCanHaveMultipleChildren() const;

    /// <summary>
    /// </summary>
    /// <returns>true if the panel can accept another child UIComponent.</returns>
    API_FUNCTION() bool CanAddMoreChildren() const;

#if USE_EDITOR
    /// <summary>
    /// Locks to panel on drag.
    /// </summary>
    /// <returns></returns>
    virtual bool LockToPanelOnDrag() const
    {
        return false;
    }
#endif

protected:

    /// <summary>
    /// Gets the slot class.
    /// </summary>
    /// <returns></returns>
    virtual ScriptingTypeInitializer& GetSlotClass() const;

    /// <summary>
    /// Called when slot is added.
    /// </summary>
    /// <param name="InSlot">The slot.</param>
    virtual void OnSlotAdded(UIPanelSlot* InSlot)
    {

    }

    /// <summary>
    /// Called when slot is removed.
    /// </summary>
    /// <param name="InSlot">The slot.</param>
    virtual void OnSlotRemoved(UIPanelSlot* InSlot)
    {
    }

protected:
    friend class UIComponent;
    friend class UIPanelSlot;
    friend class UIBlueprint;

    /// <summary>
    /// Can this panel allow for multiple children?
    /// </summary>
    bool CanHaveMultipleChildren;
protected:
    virtual void Layout(const Rectangle& InNewBounds) override;
    virtual void Layout(const Rectangle& InSlotNewBounds, UIPanelSlot* InFor);
public: // exposed for Native UI host on c# side
    API_FUNCTION(internal) void Render();
};
