// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Singleton.h"

class MClass;
class MMethod;

/// <summary>
/// The helper class for STD types from managed runtime.
/// </summary>
class FLAXENGINE_API StdTypesContainer : public Singleton<StdTypesContainer>
{
public:

    StdTypesContainer();

    void Clear();
    bool Gather();

public:

    MClass* GuidClass;
    MClass* DictionaryClass;
    MClass* ActivatorClass;
    MClass* TypeClass;

    MClass* Vector2Class;
    MClass* Vector3Class;
    MClass* Vector4Class;
    MClass* ColorClass;
    MClass* TransformClass;
    MClass* QuaternionClass;
    MClass* MatrixClass;
    MClass* BoundingBoxClass;
    MClass* BoundingSphereClass;
    MClass* RectangleClass;
    MClass* RayClass;

    MClass* CollisionClass;

    MClass* JSON;
    MMethod* Json_Serialize;
    MMethod* Json_SerializeDiff;
    MMethod* Json_Deserialize;

    MClass* ManagedArrayClass;

#if USE_EDITOR
    MClass* ExecuteInEditModeAttribute;
#endif
};
