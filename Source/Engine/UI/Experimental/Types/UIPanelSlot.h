//                                                        UI System
//                                                    writen by Nori_SC
//                                                https://github.com/NoriteSC
#pragma once

#include "Engine/Scripting/ScriptingObject.h"
#include "UIComponent.h"
#include "Engine/Core/Math/Rectangle.h"

class UIPanelComponent;

API_CLASS(Namespace = "FlaxEngine.Experimental.UI") 
class FLAXENGINE_API UIPanelSlot : public ScriptingObject
{
    DECLARE_SCRIPTING_TYPE(UIPanelSlot);
public:

    //API_FIELD()
        class UIPanelComponent* Parent;

    //API_FIELD()
        class UIComponent* Content;

#if WITH_EDITOR
    bool IsDesignTime() const
    {
        return Parent->IsDesignTime();
    }
#else
    FORCE_INLINE bool IsDesignTime() const { return false; }
#endif

    virtual void Layout();
    virtual const Rectangle& GetBounds();
};

