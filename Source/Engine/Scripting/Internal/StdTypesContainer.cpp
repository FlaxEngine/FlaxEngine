// Copyright (c) Wojciech Figat. All rights reserved.

#include "StdTypesContainer.h"
#include "Engine/Scripting/Scripting.h"
#include "Engine/Scripting/ScriptingType.h"
#include "Engine/Scripting/BinaryModule.h"
#include "Engine/Scripting/ManagedCLR/MAssembly.h"
#include "Engine/Scripting/ManagedCLR/MClass.h"
#include "Engine/Core/Log.h"
#include "FlaxEngine.Gen.h"

StdTypesContainer::StdTypesContainer()
{
    Clear();
}

void StdTypesContainer::Clear()
{
    GuidClass = nullptr;
    DictionaryClass = nullptr;
    ActivatorClass = nullptr;
    TypeClass = nullptr;

    Vector2Class = nullptr;
    Vector3Class = nullptr;
    Vector4Class = nullptr;
    ColorClass = nullptr;
    TransformClass = nullptr;
    QuaternionClass = nullptr;
    MatrixClass = nullptr;
    BoundingBoxClass = nullptr;
    BoundingSphereClass = nullptr;
    RectangleClass = nullptr;
    RayClass = nullptr;

    CollisionClass = nullptr;

    JSON = nullptr;
    Json_Serialize = nullptr;
    Json_SerializeDiff = nullptr;
    Json_Deserialize = nullptr;

    ManagedArrayClass = nullptr;

#if USE_EDITOR
    ExecuteInEditModeAttribute = nullptr;
#endif
}

bool StdTypesContainer::Gather()
{
#if USE_CSHARP
#define GET_CLASS(assembly, type, typeName) \
    type = ((ManagedBinaryModule*)CONCAT_MACROS(GetBinaryModule, assembly)())->Assembly->GetClass(typeName); \
    if (type == nullptr) \
    { \
        LOG(Error, "Missing managed type: \'{0}\'", String(typeName)); \
        return true; \
    }
#define GET_METHOD(type, klass, typeName, paramsCount) \
    type = klass->GetMethod(typeName, paramsCount); \
    if (type == nullptr) \
    { \
        LOG(Error, "Missing managed type: \'{0}\'", String(typeName)); \
        return true; \
    }

    GET_CLASS(Corlib, GuidClass, "System.Guid");
    GET_CLASS(Corlib, DictionaryClass, "System.Collections.Generic.Dictionary`2");
    GET_CLASS(Corlib, ActivatorClass, "System.Activator");
    GET_CLASS(Corlib, TypeClass, "System.Type");

    GET_CLASS(FlaxEngine, Vector2Class, "FlaxEngine.Vector2");
    GET_CLASS(FlaxEngine, Vector3Class, "FlaxEngine.Vector3");
    GET_CLASS(FlaxEngine, Vector4Class, "FlaxEngine.Vector4");
    GET_CLASS(FlaxEngine, ColorClass, "FlaxEngine.Color");
    GET_CLASS(FlaxEngine, TransformClass, "FlaxEngine.Transform");
    GET_CLASS(FlaxEngine, QuaternionClass, "FlaxEngine.Quaternion");
    GET_CLASS(FlaxEngine, MatrixClass, "FlaxEngine.Matrix");
    GET_CLASS(FlaxEngine, BoundingBoxClass, "FlaxEngine.BoundingBox");
    GET_CLASS(FlaxEngine, BoundingSphereClass, "FlaxEngine.BoundingSphere");
    GET_CLASS(FlaxEngine, RectangleClass, "FlaxEngine.Rectangle");
    GET_CLASS(FlaxEngine, RayClass, "FlaxEngine.Ray");

    GET_CLASS(FlaxEngine, CollisionClass, "FlaxEngine.Collision");

    GET_CLASS(FlaxEngine, JSON, "FlaxEngine.Json.JsonSerializer");

    GET_METHOD(Json_Serialize, JSON, "Serialize", 2);
    GET_METHOD(Json_SerializeDiff, JSON, "SerializeDiff", 3);
    GET_METHOD(Json_Deserialize, JSON, "Deserialize", 3);

    GET_CLASS(FlaxEngine, ManagedArrayClass, "FlaxEngine.Interop.ManagedArray");

#if USE_EDITOR
    GET_CLASS(FlaxEngine, ExecuteInEditModeAttribute, "FlaxEngine.ExecuteInEditModeAttribute");
#endif

#undef GET_CLASS
#undef GET_METHOD
#endif
    return false;
}
