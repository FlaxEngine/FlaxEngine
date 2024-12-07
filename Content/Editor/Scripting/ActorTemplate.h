%copyright%#pragma once

#include "Engine/Level/Actor.h"

API_CLASS() class %module%%class% : public Actor
{
API_AUTO_SERIALIZATION();
DECLARE_SCENE_OBJECT(%class%);

    // [Actor]
    void OnEnable() override;
    void OnDisable() override;
};
