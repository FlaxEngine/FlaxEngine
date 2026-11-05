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
#define USE_NETCORE 0

// Dummy types declarations
typedef struct CSharpObject MObject;
typedef struct CSharpArray MArray;
typedef struct CSharpString MString;
typedef struct CSharpType MType;
typedef unsigned int MGCHandle;

#else

#ifndef USE_MONO_AOT
// No AOT by default
#define USE_MONO_AOT 0
#define USE_MONO_AOT_MODE MONO_AOT_MODE_NONE
#endif

// .NET scripting
#define USE_CSHARP 1
#define USE_NETCORE 1

// Dotnet types declarations
typedef struct DotNetObject MObject;
typedef struct DotNetArray MArray;
typedef struct DotNetString MString;
typedef struct DotNetType MType;
typedef unsigned long long MGCHandle;

// Enables using single (root) app domain for the user scripts
#define USE_SCRIPTING_SINGLE_DOMAIN 1

#endif
