//                                                        UI System
//                                                    writen by Nori_SC
//                                                https://github.com/NoriteSC
#pragma once

#include "Engine/Scripting/ScriptingObject.h"
#include "UIComponent.h"
#include "Engine/Core/Math/Rectangle.h"

class UIPanelComponent;

/// <summary>
/// this class is light weight link betwine parent and child
/// perpes of it is to be a Layout controler and hold data needed to preform the Layout on the child
/// </summary>
API_CLASS(Namespace = "FlaxEngine.Experimental.UI")
class FLAXENGINE_API UIPanelSlot : public ScriptingObject
{
    DECLARE_SCRIPTING_TYPE(UIPanelSlot);
public:

    //API_FIELD()
    class UIPanelComponent* Parent;

    //API_FIELD()
    class UIComponent* Content;

    virtual void Layout(const Rectangle& InSlotNewBounds);
#if USE_EDITOR
    FORCE_INLINE bool IsDesignTime() const;
#else
    FORCE_INLINE bool IsDesignTime() const { return false; }
#endif
};

