%copyright%
#pragma once

#include "Engine/Scripting/Script.h"

API_CLASS() class %module%%class% : public Script
{
DECLARE_SCRIPTING_TYPE(%class%);

    // [Script]
    void OnEnable() override;
    void OnDisable() override;
    void OnUpdate() override;
};
