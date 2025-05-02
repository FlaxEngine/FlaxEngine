// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Scripting/ManagedCLR/MTypes.h"

typedef void (*Thunk_Void_0)(MObject** exception);
typedef void (*Thunk_Void_1)(void* param_1, MObject** exception);
typedef void (*Thunk_Void_2)(void* param_1, void* param_2, MObject** exception);
typedef void (*Thunk_Void_3)(void* param_1, void* param_2, void* param_3, MObject** exception);
typedef void (*Thunk_Void_4)(void* param_1, void* param_2, void* param_3, void* param_4, MObject** exception);

typedef MObject* (*Thunk_Object_0)(MObject** exception);
typedef MObject* (*Thunk_Object_1)(void* param_1, MObject** exception);
typedef MObject* (*Thunk_Object_2)(void* param_1, void* param_2, MObject** exception);
typedef MObject* (*Thunk_Object_3)(void* param_1, void* param_2, void* param_3, MObject** exception);
typedef MObject* (*Thunk_Object_4)(void* param_1, void* param_2, void* param_3, void* param_4, MObject** exception);
