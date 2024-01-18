//                                                        UI System
//                                                    writen by Nori_SC
//                                                https://github.com/NoriteSC
#include "Engine/Scripting/ScriptingObject.h"
#include "Engine/UI/Experimental/Types/UIPanelSlot.h"
#include "Engine/UI/Experimental/Types/UIComponent.h"

UIPanelSlot::UIPanelSlot(const SpawnParams& params) : ScriptingObject(params) {}

void UIPanelSlot::Layout()
{

}

const Rectangle& UIPanelSlot::GetBounds()
{
    return Content->Transform.Rect;
}
