//                                                        UI System
//                                                    writen by Nori_SC
//                                                https://github.com/NoriteSC
#pragma once
#include "Engine/Platform/Base/WindowBase.h"
#include "Engine/Core/Math/Rectangle.h"
#include "Engine/Core/Math/Vector2.h"
#include "Engine/Scripting/ScriptingObject.h"
#include "Engine/Scripting/ScriptingType.h"
#include "Engine/Core/ISerializable.h"

#include "UIComponentDesignFlags.h"
#include "UIComponentVisibility.h"
#include "UIComponentTransform.h"
#include "UIComponentClipping.h"
#include "UIPointerEvent.h"
#include "UIActionEvent.h"

/// <summary>
/// Base class for any UI element
/// </summary>
API_CLASS(Namespace = "FlaxEngine.Experimental.UI")
class FLAXENGINE_API UIComponent : public ScriptingObject, public ISerializable
{
    DECLARE_SCRIPTING_TYPE(UIComponent);
private:
    class UIPanelSlot* Slot;
    friend class UIPanelComponent;
protected:

    UIComponentClipping Clipping;

    /// <summary>
    /// name of UIComponent can be empty
    /// </summary>
    String Label;

    /// <summary>
    /// Sets whether this UI Component can be modified interactively by the user
    /// </summary>
    bool IsEnabled = true;

    /// <summary>
    /// The transform of the UI Component allows for arbitrary 2D transforms to be applied to the UI Component.
    /// </summary>
    UIComponentTransform Transform;

    /// <summary>
    /// The visibility of the UI Component
    /// </summary>
    UIComponentVisibility Visibility;

    /// <summary>
    /// Cursor Type
    /// </summary>
    CursorType Cursor;

    /// <summary>
    /// If true prevents the UI Component or its child's geometry or layout information from being cached. If this UI Component
    /// changes every frame, but you want it to still be in an invalidation panel you should make it as volatile
    /// instead of invalidating it every frame, which would prevent the invalidation panel from actually
    /// ever caching anything.
    /// </summary>
    API_FIELD() bool IsVolatile = false;

    /// <summary>
    /// Allows controls to be exposed as variables in a UIBlueprint.  Not all controls need to be exposed
    /// as variables, so this allows only the most useful ones to end up being exposed.
    /// </summary>
    API_FIELD() bool IsVariable = true;

    /// <summary>
    /// Flag if the UI Component was created from a UIBlueprint
    /// </summary>
    API_FIELD() bool CreatedByUIBlueprint = false;


    /// <summary>
    /// Flag for if the UI Component need to change cursor when entering the UI Component
    /// </summary>
    API_FIELD() bool OverrideCursor = true;

    /// <summary>
    /// The opacity of the UI Component
    /// </summary>
    API_FIELD() float RenderOpacity;
public:
   
    /// <summary>
    /// Controls how the clipping behavior of this UI Component.                           
    /// Normally content that overflows the bounds of the UI Component continues rendering.
    /// Enabling clipping prevents that overflowing content from being seen.           
    /// <para><b>Note:</b></para>
    /// UI Components in different clipping spaces can not be batched together, and so there is a performance cost to clipping.
    /// Do not enable clipping unless a panel actually needs to prevent content from showing up outside its bounds.
    /// </summary>
    API_PROPERTY() void SetClipping(const UIComponentClipping& InClipping);

    /// <summary>
    /// Controls how the clipping behavior of this UI Component.                           
    /// Normally content that overflows the bounds of the UI Component continues rendering.
    /// Enabling clipping prevents that overflowing content from being seen.           
    /// <para><b>Note:</b></para>
    /// UI Components in different clipping spaces can not be batched together, and so there is a performance cost to clipping.
    /// Do not enable clipping unless a panel actually needs to prevent content from showing up outside its bounds.
    /// </summary>
    API_PROPERTY() const UIComponentClipping& GetClipping() const;

    /// <summary>
    /// Sets the transform.
    /// </summary>
    /// <param name="InTransform">The in transform.</param>
    /// <returns></returns>
    API_PROPERTY() void SetTransform(const UIComponentTransform& InTransform);

    /// <summary>
    /// Gets the transform.
    /// </summary>
    /// <returns></returns>
    API_PROPERTY() const UIComponentTransform& GetTransform() const;

    /// <summary>
    /// Sets the translation.
    /// </summary>
    /// <param name="InTranslation">The in translation.</param>
    /// <returns></returns>
    API_PROPERTY() void SetTranslation(const Vector2& InTranslation);

    /// <summary>
    /// Gets the translation.
    /// </summary>
    /// <returns></returns>
    API_PROPERTY() const Vector2& GetTranslation() const;

    /// <summary>
    /// Sets the size.
    /// </summary>
    /// <param name="InSize">Size of the in.</param>
    /// <returns></returns>
    API_PROPERTY() void SetSize(const Vector2& InSize);

    /// <summary>
    /// Gets the size.
    /// </summary>
    /// <returns></returns>
    API_PROPERTY() const Vector2& GetSize() const;

    /// <summary>
    /// Sets the shear.
    /// </summary>
    /// <param name="InShear">The in shear.</param>
    /// <returns></returns>
    API_PROPERTY() void SetShear(const Vector2& InShear);

    /// <summary>
    /// Gets the shear.
    /// </summary>
    /// <returns></returns>
    API_PROPERTY() const Vector2& GetShear() const;

    /// <summary>
    /// Sets the angle.
    /// </summary>
    /// <param name="InAngle">The in angle.</param>
    /// <returns></returns>
    API_PROPERTY() void SetAngle(float InAngle);

    /// <summary>
    /// Gets the angle.
    /// </summary>
    /// <returns></returns>
    API_PROPERTY() float GetAngle() const;;

    /// <summary>
    /// Sets the pivot.
    /// </summary>
    /// <param name="InPivot">The in pivot.</param>
    /// <returns></returns>
    API_PROPERTY() void SetPivot(const Vector2& InPivot);

    /// <summary>
    /// Gets the pivot.
    /// </summary>
    /// <returns></returns>
    API_PROPERTY() const Vector2& GetPivot() const;

    /// <summary>
    /// Sets the is enabled.
    /// </summary>
    /// <param name="InIsEnabled">if set to <c>true</c> [in is enabled].</param>
    /// <returns></returns>
    API_PROPERTY() void SetIsEnabled(bool InIsEnabled);

    /// <summary>
    /// Gets the is enabled.
    /// </summary>
    /// <returns></returns>
    API_PROPERTY() bool GetIsEnabled() const;

    /// <summary>
    /// Sets the tool tip text.
    /// </summary>
    /// <param name="InToolTipText">The in tool tip text.</param>
    /// <returns></returns>
    API_PROPERTY() void SetToolTipText(const String& InToolTipText);

    /// <summary>
    /// Gets the tool tip text.
    /// </summary>
    /// <returns></returns>
    API_PROPERTY() const String& GetToolTipText() const;

    /// <summary>
    /// Sets the tool tip.
    /// </summary>
    /// <param name="InToolTip">The in tool tip.</param>
    /// <returns></returns>
    API_PROPERTY() void SetToolTip(UIComponent* InToolTip);

    /// <summary>
    /// Gets the tool tip.
    /// </summary>
    /// <returns></returns>
    API_PROPERTY() UIComponent* GetToolTip() const;

    /// <summary>
    /// Sets the cursor.
    /// </summary>
    /// <param name="InCursor">The in cursor.</param>
    /// <returns></returns>
    API_PROPERTY() void SetCursor(const CursorType& InCursor);

    /// <summary>
    /// Gets the cursor.
    /// </summary>
    /// <returns></returns>
    API_PROPERTY() const CursorType& GetCursor() const;

    /// <summary>
    /// Sets the visibility.
    /// </summary>
    /// <param name="InVisibility">The in visibility.</param>
    /// <returns></returns>
    API_PROPERTY() void SetVisibility(const UIComponentVisibility& InVisibility);

    /// <summary>
    /// Gets the visibility.
    /// </summary>
    /// <returns></returns>
    API_PROPERTY() const UIComponentVisibility& GetVisibility() const;

    /// <summary>
    /// Resets the cursor to use on the UIComponent, removing any customization for it.
    /// </summary>
    API_FUNCTION() void ResetCursor();

    /// <summary>
    /// Returns true if the UIComponent is Visible, HitTestInvisible or SelfHitTestInvisible and the  Opacity is greater than 0.
    /// </summary>
    /// <returns></returns>
    API_FUNCTION() bool IsRendered() const;

    /// <summary>
    /// Returns true if the UIComponent is Visible, HitChildren or HitSelf.
    /// </summary>
    /// <returns></returns>
    API_FUNCTION() bool IsVisible() const;

    /// <summary>
    /// Gets the source asset or class.
    /// </summary>
    /// <returns></returns>
    Object* GetSourceAssetOrClass() const;

    /// <summary>
    /// Function implemented by all subclasses of UIComponent is called when the underlying UIComponent needs to be constructed.
    /// </summary>
    /// <returns></returns>
    virtual UIComponent* RebuildUIComponent();

    /// <summary>
    /// Function called after the underlying UIComponent is constructed.
    /// </summary>
    API_FUNCTION() void OnUIComponentRebuilt();

    /// <summary>
    /// Updates the transform.
    /// </summary>
    API_FUNCTION() void UpdateTransform();

    /// <summary>
    /// Gets the parent.
    /// </summary>
    /// <returns></returns>
    API_FUNCTION() class UIPanelComponent* GetParent() const;

    /// <summary>
    /// Removes from parent.
    /// </summary>
    API_FUNCTION() void RemoveFromParent();

#if USE_EDITOR
    /// <summary>
    /// Any flags used by the designer at edit time.
    /// </summary>
    UIComponentDesignFlags DesignerFlags;

    /// <summary>
    /// The friendly name for this UI Component displayed in the designer.
    /// </summary>
    String DisplayLabel;

    /// <summary>
    /// Category name used in the UI Component designer for sorting purpose
    /// </summary>
    String CategoryName = L"Unknown";

    /// <summary>
    /// Stores the design time flag setting if the UI Component is hidden inside the designer
    /// </summary>
    bool HiddenInDesigner = true;

    /// <summary>
    /// Stores the design time flag setting if the UI Component is expanded inside the designer
    /// </summary>
    bool ExpandedInDesigner = true;

    /// <summary>
    /// Stores the design time flag setting if the UI Component is locked inside the designer
    /// </summary>
    bool LockedInDesigner = true;

    /// <summary>
    /// Is this UI Component locked in the designer
    /// </summary>
    /// <returns>Lock value</returns>
    bool IsLockedInDesigner() const;

    /// <summary>
    /// LockedInDesigner should this UI Component be locked
    /// </summary>
    /// <param name="NewLockedInDesigner">New lock value</param>
    virtual void SetLockedInDesigner(bool NewLockedInDesigner);

    /// <summary>
    /// Gets the visibility in designer.
    /// This is an implementation detail that allows us to show and hide the UI Component in the designer
    /// regardless of the actual visibility state set by the user.
    /// </summary>
    /// <returns></returns>
    UIComponentVisibility GetVisibilityInDesigner() const;

    /// <summary>
    /// Determines whether this is Editor UI component.
    /// </summary>
    /// <returns>
    ///   <c>true</c> if is editor UI component otherwise, <c>false</c>.
    /// </returns>
    bool IsEditorUIComponent() const;

    /// <summary>
    /// </summary>
    /// <returns>if the UI Component is currently being displayed in the designer, it may want to display different data.</returns>
    FORCE_INLINE bool IsDesignTime() const;

    /// <summary>
    /// Determines whether [has any designer flags] [the specified flags to check].
    /// </summary>
    /// <param name="FlagsToCheck">The flags to check.</param>
    /// <returns></returns>
    FORCE_INLINE bool HasAnyDesignerFlags(UIComponentDesignFlags FlagsToCheck) const;

    /// <summary>
    /// Determines whether [is preview time].
    /// </summary>
    /// <returns></returns>
    FORCE_INLINE bool IsPreviewTime() const;

    /// <summary>
    /// Gets the display label.
    /// </summary>
    /// <returns></returns>
    const String& GetDisplayLabel() const;

    /// <summary>
    /// Sets the display label.
    /// </summary>
    /// <param name="DisplayLabel">The display label.</param>
    void SetDisplayLabel(const String& InDisplayLabel);

    /// <summary>
    /// Is the label generated or provided by the user?
    /// </summary>
    /// <returns></returns>
    bool IsGeneratedName() const;

    /// <summary>
    /// Gets the label to display to the user for this UI Component.
    /// </summary>
    /// <returns></returns>
    String GetLabel() const;

    /// <summary>
    /// Determines whether is visible in designer.
    /// </summary>
    /// <returns>
    ///   <c>true</c> if is visible in designer, otherwise <c>false</c>.
    /// </returns>
    bool IsVisibleInDesigner() const;

    /// <summary>
    /// Selects the by designer.
    /// </summary>
    void SelectByDesigner();

    /// <summary>
    /// Deselects the by designer.
    /// </summary>
    void DeselectByDesigner();

    /// <summary>
    /// Called when [selected by designer].
    /// </summary>
    virtual void OnSelectedByDesigner() { }

    /// <summary>
    /// Called when [deselected by designer].
    /// </summary>
    virtual void OnDeselectedByDesigner() { }

    /// <summary>
    /// Called when [begin edit by designer].
    /// </summary>
    virtual void OnBeginEditByDesigner() { }

    /// <summary>
    /// Called when [end edit by designer].
    /// </summary>
    virtual void OnEndEditByDesigner() { }

#else
    FORCE_INLINE bool IsDesignTime() const { return false; }
    FORCE_INLINE bool IsPreviewTime() const { return false; }
#endif
public:
    virtual Vector2 ComputeDesiredSize(float scale = 1)
    {
        return Transform.Rect.Size * scale;
    }


    virtual void OnDraw(){}

public:

    /// <summary>
    /// Called when input has event's: mouse, touch, stylus, gamepad emulated mouse, etc. and value has changed
    /// </summary>
    API_FUNCTION() virtual void OnPointerInput(const UIPointerEvent& InEvent) {}

    /// <summary>
    /// Called when input has event's: keyboard, gamepay buttons, etc. and value has changed
    /// Note: keyboard can have action keys where value is from 0 to 1
    /// </summary>
    API_FUNCTION() virtual void OnActionInputChaneged(const UIActionEvent& InEvent) {} //

protected:
    friend class UIPanelOrderedSlot;
    friend class UIPanelSlot;

    void DrawInternal();
public: // exposed for Native UI host on c# side
    virtual void InvalidateLayout(const UIPanelSlot* InFor);
    FORCE_INLINE int GetCompactedFlags();
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;
};
