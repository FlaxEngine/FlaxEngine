// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Scripting/ManagedCLR/MTypes.h"

typedef void (*Thunk_Void_0)(MonoObject** exception);
typedef void (*Thunk_Void_1)(void* param_1, MonoObject** exception);
typedef void (*Thunk_Void_2)(void* param_1, void* param_2, MonoObject** exception);
typedef void (*Thunk_Void_3)(void* param_1, void* param_2, void* param_3, MonoObject** exception);
typedef void (*Thunk_Void_4)(void* param_1, void* param_2, void* param_3, void* param_4, MonoObject** exception);

typedef MonoObject* (*Thunk_Object_0)(MonoObject** exception);
typedef MonoObject* (*Thunk_Object_1)(void* param_1, MonoObject** exception);
typedef MonoObject* (*Thunk_Object_2)(void* param_1, void* param_2, MonoObject** exception);
typedef MonoObject* (*Thunk_Object_3)(void* param_1, void* param_2, void* param_3, MonoObject** exception);
typedef MonoObject* (*Thunk_Object_4)(void* param_1, void* param_2, void* param_3, void* param_4, MonoObject** exception);
