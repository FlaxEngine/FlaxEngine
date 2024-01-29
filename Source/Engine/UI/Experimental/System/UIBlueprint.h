#pragma once
#include "Engine/Scripting/ScriptingObject.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Content/AssetReference.h"
#include "Engine/Core/Log.h"
#include "../Types/UIComponent.h"
#include "../Types/UIPanelComponent.h"
#include "../Asset/UIBlueprintAsset.h"
#include "Engine\Scripting\ManagedCLR\MClass.h"

/// <summary>
/// Game user interface blueprint class
/// </summary>
API_CLASS(Namespace = "FlaxEngine.Experimental.UI")
class UIBlueprint : public ScriptingObject
{
    DECLARE_SCRIPTING_TYPE(UIBlueprint);
protected:

    /// <summary>
    /// This is called only once for the UIBlueprint.
    /// After deserialization when components of UIBlueprint are ready for use 
    /// If you have one-time things to establish up-front (like binding callbacks or events), do so here.
    /// u can override this in derived classes, but always call the parent implementation.
    /// </summary>
    API_FUNCTION() virtual void OnInitialized();

    /// <summary>
    /// Called by both the game and the editor.
    /// Allows users to run initial setup for their UI to better preview
    /// the setup in the designer and since generally that same setup code is required at runtime, it's called there as well.
    /// </summary>
    /// <param name="IsDesignTime">if set to <c>true</c> [is design time].</param>
    API_FUNCTION() virtual void PreConstruct(bool IsDesignTime);

    /// <summary>
    /// Called after the underlying UI Components are constructed.
    /// Depending on how the object is used this may be called multiple times due to adding and removing from the hierarchy.
    /// If you need a true called once, use OnInitialized.
    /// </summary>
    API_FUNCTION() virtual void Construct();

    /// <summary>
    /// Ticks this UI Component. 
    /// Override in derived classes, but always call the parent implementation.
    /// </summary>
    /// <param name="DeltaTime">The delta time.</param>
    API_FUNCTION() virtual void Tick(float DeltaTime);

    /// <summary>
    /// Called when a UI Component is no longer referenced causing the resource to be destroyed.
    /// Just like construct this event can be called multiple times.
    /// </summary>
    API_FUNCTION() virtual void Destruct();

public:
    struct Variable
    {
    private:
        String Name;
        UIComponent* Component;
    public:
        Variable(const String& InName, UIComponent* InComponent) : Name(InName), Component(InComponent)
        {
            InComponent->Deleted.Bind
            (
                [this](ScriptingObject* comp)
                {
                    Name = "";
                    Component = nullptr;
                }
            );
        }

        FORCE_INLINE UIComponent* GetComponent() { return Component; }
        FORCE_INLINE bool operator ==(const String& name) { return Name == name; }
    };

    /// <summary>
    /// Gets the variable.
    /// <para></para>
    /// <note type="note">
    /// Remember to cache the return value the call is relatively expensive
    /// </note>
    /// </summary>
    /// <typeparam name="T">
    ///   <span class="keyword">typeof</span>(<see cref="UIComponent" />)</typeparam>
    /// <param name="VarName">Name of the variable.</param>
    /// <returns>A <span class="typeparameter">T</span> where <span class="typeparameter">T</span> is <span class="keyword">typeof</span>(<see cref="UIComponent" />) it can return null</returns>
    template<typename T>
    T* GetVariable(const String& VarName)
    {
        for (auto& var : Variables)
        {
            if(var == VarName)
            {
                if (!ScriptingObject::CanCast(UIComponent::GetStaticClass(), T::GetStaticClass()))
                {
                    return (T*)var.GetComponent();
                }
                else
                {
                    LOG(Error,"Cant cast {0} to UIComponent",String(T::GetStaticClass()->GetName().GetText()));
                }
                return nullptr;
            }
        }
        return nullptr;
    }

private:
    bool IsReady = true;
    API_FIELD(internal) UIComponent* Component;
    Array<Variable> Variables;
    API_FUNCTION() UIComponent* GetVariable_Internal(const String& VarName)
    {
        for (auto& var : Variables)
        {
            if (var == VarName)
            {
                return var.GetComponent();
            }
        }
        return nullptr;
    }

protected:
    friend class UISystem;
    AssetReference<UIBlueprintAsset> Asset;
    API_FUNCTION(internal) UIEventResponse SendEvent(const UIPointerEvent& InEvent,API_PARAM(out) const UIComponent*& OutHit);
#if USE_EDITOR
    static void AddDesinerFlags(UIComponent* comp, UIComponentDesignFlags flags)
    {
        if (!comp)
            return;
        comp->DesignerFlags |= flags;
        if (auto c = comp->Cast<UIPanelComponent>())
        {
            for (auto s : c->GetSlots())
            {
                AddDesinerFlags(s->Content, flags);
            }
        }
    }
    static void RemoveDesinerFlags(UIComponent* comp, UIComponentDesignFlags flags)
    {
        if (!comp)
            return;
        comp->DesignerFlags &= ~flags;
        if (auto c = comp->Cast<UIPanelComponent>())
        {
            for (auto s : c->GetSlots())
            {
                AddDesinerFlags(s->Content, flags);
            }
        }
    }
    static void SetDesinerFlags(UIComponent* comp, UIComponentDesignFlags flags)
    {
        if (!comp)
            return;
        comp->DesignerFlags = flags;
        if (auto c = comp->Cast<UIPanelComponent>())
        {
            for (auto s : c->GetSlots())
            {
                AddDesinerFlags(s->Content, flags);
            }
        }
    }
    API_FUNCTION(internal) void AddDesinerFlags(UIComponentDesignFlags flags) { UIBlueprint::AddDesinerFlags(Component, flags); }
    API_FUNCTION(internal) void RemoveDesinerFlags(UIComponentDesignFlags flags) { UIBlueprint::RemoveDesinerFlags(Component, flags); }
    API_FUNCTION(internal) void SetDesinerFlags(UIComponentDesignFlags flags) { UIBlueprint::SetDesinerFlags(Component, flags); }
#endif
};

