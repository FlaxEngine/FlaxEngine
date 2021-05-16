// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/String.h"
#include "Engine/Core/Types/StringView.h"

// Enable/disable mono debugging
#define MONO_DEBUG_ENABLE (!BUILD_RELEASE)
#define USE_MONO (1)
#define USE_NETCORE (0)

#ifndef USE_MONO_AOT
#define USE_MONO_AOT 0
#define USE_MONO_AOT_MODE MONO_AOT_MODE_NONE
#endif

// Enables/disables profiling managed world via Mono
#define USE_MONO_PROFILER (COMPILE_WITH_PROFILER)

// Enables using single (root) app domain for the user scripts
#define USE_SINGLE_DOMAIN 1

#if USE_MONO

/// <summary>
/// String container for names and typenames used by the managed runtime backend.
/// </summary>
typedef StringAnsi MString;

// Mono types declarations
typedef struct _MonoClass MonoClass;
typedef struct _MonoDomain MonoDomain;
typedef struct _MonoImage MonoImage;
typedef struct _MonoAssembly MonoAssembly;
typedef struct _MonoMethod MonoMethod;
typedef struct _MonoProperty MonoProperty;
typedef struct _MonoObject MonoObject;
typedef struct _MonoEvent MonoEvent;
typedef struct _MonoType MonoType;
typedef struct _MonoString MonoString;
typedef struct _MonoArray MonoArray;
typedef struct _MonoReflectionType MonoReflectionType;
typedef struct _MonoReflectionAssembly MonoReflectionAssembly;
typedef struct _MonoException MonoException;
typedef struct _MonoClassField MonoClassField;
typedef MonoObject MObject;

#endif

// Forward declarations
class MCore;
class MAssembly;
class MClass;
class MField;
class MMethod;
class MProperty;
class MEvent;
class MDomain;
class MType;

enum class MVisibility
{
    Private,
    PrivateProtected,
    Internal,
    Protected,
    ProtectedInternal,
    Public,
};
