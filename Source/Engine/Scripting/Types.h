// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Platform/Defines.h"

// Forward declarations
class Scripting;
struct ScriptingType;
class BinaryModule;
class ScriptingObject;
class MCore;
class MDomain;
class MException;
class MAssembly;
class MClass;
class MField;
class MMethod;
class MProperty;
class MEvent;
class MDomain;
class MType;

#if COMPILE_WITHOUT_CSHARP

// No Scripting
#define USE_MONO 0
#define USE_NETCORE 0
typedef void MObject;

#else

#define USE_MONO 1
#define USE_NETCORE 0

// Enables using single (root) app domain for the user scripts
#define USE_SCRIPTING_SINGLE_DOMAIN 1

#if USE_MONO

// Enables/disables profiling managed world via Mono
#define USE_MONO_PROFILER (COMPILE_WITH_PROFILER)

// Enable/disable mono debugging
#define MONO_DEBUG_ENABLE (!BUILD_RELEASE)

#ifndef USE_MONO_AOT
#define USE_MONO_AOT 0
#define USE_MONO_AOT_MODE MONO_AOT_MODE_NONE
#endif

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

#endif
