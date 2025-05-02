// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "MTypes.h"

/// <summary>
/// Main handler for CLR Engine.
/// </summary>
class FLAXENGINE_API MCore
{
public:
    /// <summary>
    /// Gets the root domain.
    /// </summary>
    static MDomain* GetRootDomain();

    /// <summary>
    /// Gets the currently active domain.
    /// </summary>
    static MDomain* GetActiveDomain();

    /// <summary>
    /// Creates an new empty domain.
    /// </summary>
    /// <param name="domainName">The domain name to create.</param>
    /// <returns>The domain object.</returns>
    static MDomain* CreateDomain(const StringAnsi& domainName);

    /// <summary>
    /// Unloads the domain.
    /// </summary>
    /// <param name="domainName">The domain name to remove.</param>
    static void UnloadDomain(const StringAnsi& domainName);

public:
    /// <summary>
    /// Initialize CLR Engine
    /// </summary>
    /// <returns>True if failed, otherwise false.</returns>
    static bool LoadEngine();

    /// <summary>
    /// Unload CLR Engine
    /// </summary>
    static void UnloadEngine();

    /// <summary>
    /// Creates the assembly load context for assemblies used by Scripting.
    /// </summary>
    static void CreateScriptingAssemblyLoadContext();

#if USE_EDITOR
    /// <summary>
    /// Called by Scripting in a middle of hot-reload (after unloading modules but before loading them again).
    /// </summary>
    static void UnloadScriptingAssemblyLoadContext();
#endif

public:
    /// <summary>
    /// Utilities for C# object management.
    /// </summary>
    struct FLAXENGINE_API Object
    {
        static MObject* Box(void* value, const MClass* klass);
        static void* Unbox(MObject* obj);
        static MObject* New(const MClass* klass);
        static void Init(MObject* obj);
        static MClass* GetClass(MObject* obj);
        static MString* ToString(MObject* obj);
        static int32 GetHashCode(MObject* obj);
    };

    /// <summary>
    /// Utilities for C# string management.
    /// </summary>
    struct FLAXENGINE_API String
    {
        static MString* GetEmpty(MDomain* domain = nullptr);
        static MString* New(const char* str, int32 length, MDomain* domain = nullptr);
        static MString* New(const Char* str, int32 length, MDomain* domain = nullptr);
        static StringView GetChars(MString* obj);
    };

    /// <summary>
    /// Utilities for C# array management.
    /// </summary>
    struct FLAXENGINE_API Array
    {
        static MArray* New(const MClass* elementKlass, int32 length);
        static MClass* GetClass(MClass* elementKlass);
        static MClass* GetArrayClass(const MArray* obj);
        static int32 GetLength(const MArray* obj);
        static void* GetAddress(const MArray* obj);
        static MArray* Unbox(MObject* obj);

        template<typename T>
        FORCE_INLINE static T* GetAddress(const MArray* obj)
        {
            return (T*)GetAddress(obj);
        }
    };

    /// <summary>
    /// Utilities for GC Handle management.
    /// </summary>
    struct FLAXENGINE_API GCHandle
    {
        static MGCHandle New(MObject* obj, bool pinned = false);
        static MGCHandle NewWeak(MObject* obj, bool trackResurrection = false);
        static MObject* GetTarget(const MGCHandle& handle);
        static void Free(const MGCHandle& handle);
    };

    /// <summary>
    /// Helper utilities for C# garbage collector.
    /// </summary>
    struct FLAXENGINE_API GC
    {
        static void Collect();
        static void Collect(int32 generation);
        static void Collect(int32 generation, MGCCollectionMode collectionMode, bool blocking, bool compacting);
        static int32 MaxGeneration();
        static void WaitForPendingFinalizers();
        static void WriteRef(void* ptr, MObject* ref);
        static void WriteValue(void* dst, void* src, int32 count, const MClass* klass);
        static void WriteArrayRef(MArray* dst, MObject* ref, int32 index);
        static void WriteArrayRef(MArray* dst, Span<MObject*> span);
#if USE_NETCORE
        static void* AllocateMemory(int32 size, bool coTaskMem = false);
        static void FreeMemory(void* ptr, bool coTaskMem = false);
#endif
    };

    /// <summary>
    /// Utilities for C# threads management.
    /// </summary>
    struct FLAXENGINE_API Thread
    {
        static void Attach();
        static void Exit();
        static bool IsAttached();
    };

    /// <summary>
    /// Helper utilities for C# exceptions throwing.
    /// </summary>
    struct FLAXENGINE_API Exception
    {
        static void Throw(MObject* exception);
        static MObject* GetNullReference();
        static MObject* Get(const char* msg);
        static MObject* GetArgument(const char* arg, const char* msg);
        static MObject* GetArgumentNull(const char* arg);
        static MObject* GetArgumentOutOfRange(const char* arg);
        static MObject* GetNotSupported(const char* msg);
    };

    /// <summary>
    /// Helper utilities for C# types information.
    /// </summary>
    struct FLAXENGINE_API Type
    {
        static ::String ToString(MType* type);
        static MClass* GetClass(MType* type);
        static MType* GetElementType(MType* type);
        static int32 GetSize(MType* type);
        static MTypes GetType(MType* type);
        static bool IsPointer(MType* type);
        static bool IsReference(MType* type);
#if USE_MONO
        static MTypeObject* GetObject(MType* type);
        static MType* Get(MTypeObject* type);
#endif
    };

    /// <summary>
    /// Helper types cache from C# corlib and engine.
    /// </summary>
    struct FLAXENGINE_API TypeCache
    {
        static MClass* Void;
        static MClass* Object;
        static MClass* Byte;
        static MClass* Boolean;
        static MClass* SByte;
        static MClass* Char;
        static MClass* Int16;
        static MClass* UInt16;
        static MClass* Int32;
        static MClass* UInt32;
        static MClass* Int64;
        static MClass* UInt64;
        static MClass* IntPtr;
        static MClass* UIntPtr;
        static MClass* Single;
        static MClass* Double;
        static MClass* String;
    };

    /// <summary>
    /// Utilities for ScriptingObject management.
    /// </summary>
    struct FLAXENGINE_API ScriptingObject
    {
        static void SetInternalValues(MClass* klass, MObject* object, void* unmanagedPtr, const Guid* id);
        static MObject* CreateScriptingObject(MClass* klass, void* unmanagedPtr, const Guid* id);
    };
};
