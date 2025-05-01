// Copyright (c) Wojciech Figat. All rights reserved.

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

#if COMPILE_WITHOUT_CSHARP

// No Scripting
#define USE_CSHARP 0
#define USE_MONO 0
#define USE_NETCORE 0

// Dummy types declarations
typedef struct CSharpObject MObject;
typedef struct CSharpArray MArray;
typedef struct CSharpString MString;
typedef struct CSharpType MType;
typedef MType MTypeObject;
typedef unsigned int MGCHandle;
#define INTERNAL_TYPE_GET_OBJECT(type) (type)
#define INTERNAL_TYPE_OBJECT_GET(type) (type)

#else

#ifndef USE_MONO_AOT
#define USE_MONO_AOT 0
#define USE_MONO_AOT_MODE MONO_AOT_MODE_NONE
#endif

#if COMPILE_WITH_MONO

// Mono scripting
#define USE_CSHARP 1
#define USE_MONO 1
#define USE_NETCORE 0

// Enables/disables profiling managed world via Mono
#define USE_MONO_PROFILER (COMPILE_WITH_PROFILER)

// Enable/disable mono debugging
#define MONO_DEBUG_ENABLE (!BUILD_RELEASE)

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
typedef MonoArray MArray;
typedef MonoString MString;
typedef MonoType MType;
typedef MonoReflectionType MTypeObject;
typedef unsigned int MGCHandle;
#define INTERNAL_TYPE_GET_OBJECT(type) MCore::Type::GetObject(type)
#define INTERNAL_TYPE_OBJECT_GET(type) MCore::Type::Get(type)

#else

// .NET scripting
#define USE_CSHARP 1
#define USE_MONO 0
#define USE_NETCORE 1

// Dotnet types declarations
typedef struct DotNetObject MObject;
typedef struct DotNetArray MArray;
typedef struct DotNetString MString;
typedef struct DotNetType MType;
typedef MType MTypeObject;
typedef unsigned long long MGCHandle;
#define INTERNAL_TYPE_GET_OBJECT(type) (type)
#define INTERNAL_TYPE_OBJECT_GET(type) (type)

#endif

// Enables using single (root) app domain for the user scripts
#define USE_SCRIPTING_SINGLE_DOMAIN 1

#endif
