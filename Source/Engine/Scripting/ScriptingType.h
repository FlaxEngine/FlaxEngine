// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Types.h"
#include "Engine/Core/Types/StringView.h"
#include "Engine/Core/Types/Guid.h"

class MMethod;
class BinaryModule;
class ManagedBinaryModule;
class NativeBinaryModule;
struct ScriptingTypeHandle;
struct ScriptingTypeInitializer;
struct ScriptingObjectSpawnParams;
typedef struct _MonoClass MonoClass;
typedef struct _MonoObject MonoObject;
typedef struct _MonoType MonoType;

/// <summary>
/// The safe handle to the scripting type contained in the scripting assembly.
/// </summary>
struct FLAXENGINE_API ScriptingTypeHandle
{
    BinaryModule* Module;
    const int32 TypeIndex;

    FORCE_INLINE ScriptingTypeHandle()
        : Module(nullptr)
        , TypeIndex(-1)
    {
    }

    FORCE_INLINE ScriptingTypeHandle(BinaryModule* module, int32 typeIndex)
        : Module(module)
        , TypeIndex(typeIndex)
    {
    }

    FORCE_INLINE ScriptingTypeHandle(const ScriptingTypeHandle& other)
        : Module(other.Module)
        , TypeIndex(other.TypeIndex)
    {
    }

    ScriptingTypeHandle(const ScriptingTypeInitializer& initializer);

    FORCE_INLINE operator bool() const
    {
        return Module != nullptr;
    }

    ScriptingTypeHandle& operator=(const ScriptingTypeHandle& other)
    {
        Platform::MemoryCopy(this, &other, sizeof(ScriptingTypeHandle));
        return *this;
    }

    ScriptingTypeHandle& operator=(const ScriptingTypeHandle&& other)
    {
        if (this != &other)
        {
            Platform::MemoryCopy(this, &other, sizeof(ScriptingTypeHandle));
        }
        return *this;
    }

    String ToString(bool withAssembly = false) const;

    const ScriptingType& GetType() const;

    bool operator==(const ScriptingTypeHandle& other) const
    {
        return Module == other.Module && TypeIndex == other.TypeIndex;
    }

    bool operator!=(const ScriptingTypeHandle& other) const
    {
        return Module != other.Module || TypeIndex != other.TypeIndex;
    }

    bool operator==(const ScriptingTypeInitializer& other) const;
    bool operator!=(const ScriptingTypeInitializer& other) const;
};

inline uint32 GetHash(const ScriptingTypeHandle& key)
{
    return (uint32)(uintptr)key.Module ^ key.TypeIndex;
}

/// <summary>
/// The scripting types.
/// </summary>
enum class ScriptingTypes
{
    Script = 0,
    Structure = 1,
    Enum = 2,
    Class = 3,
    Interface = 4,
};

/// <summary>
/// The scripting type metadata for the native scripting. Used to cache the script class information and instantiate a object of its type.
/// </summary>
struct FLAXENGINE_API ScriptingType
{
    typedef void (*InitRuntimeHandler)();
    typedef ScriptingObject* (*SpawnHandler)(const ScriptingObjectSpawnParams& params);
    typedef void (*SetupScriptVTableHandler)(MClass* mclass, void**& scriptVTable, void**& scriptVTableBase);
    typedef void (*SetupScriptObjectVTableHandler)(void** scriptVTable, void** scriptVTableBase, void** vtable, int32 entriesCount, int32 wrapperIndex);
    typedef void (*Ctor)(void* ptr);
    typedef void (*Dtor)(void* ptr);
    typedef void (*Copy)(void* dst, void* src);
    typedef MonoObject* (*Box)(void* ptr);
    typedef void (*Unbox)(void* ptr, MonoObject* managed);
    typedef void (*GetField)(void* ptr, const String& name, Variant& value);
    typedef void (*SetField)(void* ptr, const String& name, const Variant& value);

    struct InterfaceImplementation
    {
        // Pointer to the type of the implemented interface.
        const ScriptingTypeInitializer* InterfaceType;

        // The offset (in bytes) from the object pointer to the interface implementation. Used for casting object to the interface.
        int16 VTableOffset;
    };

    /// <summary>
    /// The managed class (cached, can be null if missing).
    /// </summary>
    MClass* ManagedClass;

    /// <summary>
    /// The binary module that contains this type (cannot be null).
    /// </summary>
    BinaryModule* Module;

    /// <summary>
    /// The runtime data initialization handler (cannot be null).
    /// </summary>
    const InitRuntimeHandler InitRuntime;

    /// <summary>
    /// The full typename of this class (namespace and class name itself including nested classes prefix).
    /// </summary>
    const StringAnsiView Fullname;

    /// <summary>
    /// The type.
    /// </summary>
    const ScriptingTypes Type;

    /// <summary>
    /// The type base class (can be invalid).
    /// </summary>
    const ScriptingTypeHandle BaseTypeHandle;

    /// <summary>
    /// The type base class (pointer to initializer).
    /// </summary>
    const ScriptingTypeInitializer* BaseTypePtr;

    /// <summary>
    /// The list of interfaces implemented by this type (null if unused, list ends with null entry).
    /// </summary>
    const InterfaceImplementation* Interfaces;

    /// <summary>
    /// The native size of the type value (in bytes).
    /// </summary>
    int32 Size;

    union
    {
        struct
        {
            /// <summary>
            /// The object instance spawning handler (cannot be null).
            /// </summary>
            SpawnHandler Spawn;

            /// <summary>
            /// The native methods VTable. Used only by types that override the default vtable (eg. C# scripts).
            /// </summary>
            void** VTable;

            /// <summary>
            /// The script methods VTable used by the wrapper functions attached to native object vtable. Cached to improve C#/VisualScript invocation performance.
            /// </summary>
            void** ScriptVTable;

            /// <summary>
            /// The native methods VTable that matches ScriptVTable but contains pointers to the base class methods C++ instead (native methods or wrappers). Has the same size as ScriptVTable.
            /// </summary>
            void** ScriptVTableBase;

            /// <summary>
            /// The script vtable initialization handler (can be null).
            /// </summary>
            SetupScriptVTableHandler SetupScriptVTable;

            /// <summary>
            /// The native vtable initialization handler (can be null).
            /// </summary>
            SetupScriptObjectVTableHandler SetupScriptObjectVTable;

            /// <summary>
            /// The default instance of the scripting type. Used by serialization system for comparison to save only modified properties of the object.
            /// </summary>
            mutable ScriptingObject* DefaultInstance;
        } Script;

        struct
        {
            // Structure constructor method pointer
            Ctor Ctor;

            // Structure destructor method pointer
            Dtor Dtor;

            // Structure data copy operator method pointer
            Copy Copy;

            // Structure data boxing handler to convert it into managed structure object
            Box Box;

            // Structure data unboxing handler to convert it back from the managed structure into native data
            Unbox Unbox;

            // Structure field value getter
            GetField GetField;

            // Structure field value setter
            SetField SetField;
        } Struct;

        struct
        {
            // Class constructor method pointer
            Ctor Ctor;

            // Class destructor method pointer
            Dtor Dtor;
        } Class;
    };

    ScriptingType();
    ScriptingType(const StringAnsiView& fullname, BinaryModule* module, int32 size, InitRuntimeHandler initRuntime, SpawnHandler spawn, const ScriptingTypeHandle& baseType, SetupScriptVTableHandler setupScriptVTable = nullptr, SetupScriptObjectVTableHandler setupScriptObjectVTable = nullptr, const InterfaceImplementation* interfaces = nullptr);
    ScriptingType(const StringAnsiView& fullname, BinaryModule* module, int32 size, InitRuntimeHandler initRuntime = DefaultInitRuntime, SpawnHandler spawn = DefaultSpawn, ScriptingTypeInitializer* baseType = nullptr, SetupScriptVTableHandler setupScriptVTable = nullptr, SetupScriptObjectVTableHandler setupScriptObjectVTable = nullptr, const InterfaceImplementation* interfaces = nullptr);
    ScriptingType(const StringAnsiView& fullname, BinaryModule* module, int32 size, InitRuntimeHandler initRuntime, Ctor ctor, Dtor dtor, ScriptingTypeInitializer* baseType, const InterfaceImplementation* interfaces = nullptr);
    ScriptingType(const StringAnsiView& fullname, BinaryModule* module, int32 size, InitRuntimeHandler initRuntime, Ctor ctor, Dtor dtor, Copy copy, Box box, Unbox unbox, GetField getField, SetField setField, ScriptingTypeInitializer* baseType, const InterfaceImplementation* interfaces = nullptr);
    ScriptingType(const StringAnsiView& fullname, BinaryModule* module, InitRuntimeHandler initRuntime, ScriptingTypeInitializer* baseType, const InterfaceImplementation* interfaces = nullptr);
    ScriptingType(const ScriptingType& other);
    ScriptingType(ScriptingType&& other);
    ScriptingType& operator=(ScriptingType&& other) = delete;
    ScriptingType& operator=(const ScriptingType& other) = delete;
    ~ScriptingType();

    static void DefaultInitRuntime()
    {
    }

    static ScriptingObject* DefaultSpawn(const ScriptingObjectSpawnParams& params)
    {
        return nullptr;
    }

    /// <summary>
    /// Gets the handle to this type.
    /// </summary>
    /// <returns>This type handle.</returns>
    ScriptingTypeHandle GetHandle() const;

    /// <summary>
    /// Gets the handle to the base type of this type.
    /// </summary>
    /// <returns>The base type handle (might be empty).</returns>
    ScriptingTypeHandle GetBaseType() const
    {
        return BaseTypePtr ? *BaseTypePtr : BaseTypeHandle;
    }

    /// <summary>
    /// Gets the default instance of the scripting type. Used by serialization system for comparison to save only modified properties of the object.
    /// </summary>
    ScriptingObject* GetDefaultInstance() const;

    /// <summary>
    /// Gets the pointer to the implementation of the given interface type for this scripting type (including base types). Returns null if given interface is not implemented.
    /// </summary>
    const InterfaceImplementation* GetInterface(const ScriptingTypeInitializer* interfaceType) const;

    String ToString() const;
};

/// <summary>
/// The helper type for scripting type initialization in the assembly.
/// </summary>
struct FLAXENGINE_API ScriptingTypeInitializer : ScriptingTypeHandle
{
    ScriptingTypeInitializer(BinaryModule* module, const StringAnsiView& fullname, int32 size, ScriptingType::InitRuntimeHandler initRuntime = ScriptingType::DefaultInitRuntime, ScriptingType::SpawnHandler spawn = ScriptingType::DefaultSpawn, ScriptingTypeInitializer* baseType = nullptr, ScriptingType::SetupScriptVTableHandler setupScriptVTable = nullptr, ScriptingType::SetupScriptObjectVTableHandler setupScriptObjectVTable = nullptr, const ScriptingType::InterfaceImplementation* interfaces = nullptr);
    ScriptingTypeInitializer(BinaryModule* module, const StringAnsiView& fullname, int32 size, ScriptingType::InitRuntimeHandler initRuntime, ScriptingType::Ctor ctor, ScriptingType::Dtor dtor, ScriptingTypeInitializer* baseType = nullptr, const ScriptingType::InterfaceImplementation* interfaces = nullptr);
    ScriptingTypeInitializer(BinaryModule* module, const StringAnsiView& fullname, int32 size, ScriptingType::InitRuntimeHandler initRuntime, ScriptingType::Ctor ctor, ScriptingType::Dtor dtor, ScriptingType::Copy copy, ScriptingType::Box box, ScriptingType::Unbox unbox, ScriptingType::GetField getField, ScriptingType::SetField setField, ScriptingTypeInitializer* baseType = nullptr, const ScriptingType::InterfaceImplementation* interfaces = nullptr);
    ScriptingTypeInitializer(BinaryModule* module, const StringAnsiView& fullname, ScriptingType::InitRuntimeHandler initRuntime, ScriptingTypeInitializer* baseType = nullptr, const ScriptingType::InterfaceImplementation* interfaces = nullptr);
};

/// <summary>
/// Scripting object initialization parameters.
/// </summary>
struct ScriptingObjectSpawnParams
{
    /// <summary>
    /// The unique object ID.
    /// </summary>
    Guid ID;

    /// <summary>
    /// The object type handle (script class might not be loaded yet).
    /// </summary>
    const ScriptingTypeHandle Type;

    FORCE_INLINE ScriptingObjectSpawnParams(const Guid& id, const ScriptingTypeHandle& typeHandle)
        : ID(id)
        , Type(typeHandle)
    {
    }
};

/// <summary>
/// Helper define used to declare required components for native structures that have managed type.
/// </summary>
#define DECLARE_SCRIPTING_TYPE_STRUCTURE(type) \
    public: \
    friend class type##Internal; \
    static ScriptingTypeInitializer TypeInitializer; \
    FORCE_INLINE static const ScriptingType& GetStaticType() { return TypeInitializer.GetType(); } \
    FORCE_INLINE static MClass* GetStaticClass() { return TypeInitializer.GetType().ManagedClass; }

/// <summary>
/// Helper define used to declare required components for native types that have managed type (for objects that cannot be spawned).
/// </summary>
#define DECLARE_SCRIPTING_TYPE_NO_SPAWN(type) \
    public: \
    friend class type##Internal; \
    static ScriptingTypeInitializer TypeInitializer; \
    FORCE_INLINE static const ScriptingType& GetStaticType() { return TypeInitializer.GetType(); } \
    FORCE_INLINE static MClass* GetStaticClass() { return TypeInitializer.GetType().ManagedClass; }

/// <summary>
/// Helper define used to declare required components for native types that have managed type (for objects that can be spawned).
/// </summary>
#define DECLARE_SCRIPTING_TYPE(type) \
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(type); \
    static type* Spawn(const SpawnParams& params) { return ::New<type>(params); } \
    explicit type() : type(SpawnParams(Guid::New(), type::TypeInitializer)) { } \
    explicit type(const SpawnParams& params)

/// <summary>
/// Helper define used to declare required components for native types that have managed type (for objects that can be spawned) including default constructors implementations.
/// </summary>
#define DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(type, baseType) \
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(type); \
    static type* Spawn(const SpawnParams& params) { return ::New<type>(params); } \
    explicit type(const SpawnParams& params) : baseType(params) { } \
    explicit type() : baseType(SpawnParams(Guid::New(), type::TypeInitializer)) { }

/// <summary>
/// Helper define used to implement required components for native types that have managed type (for objects that can be spawned).
/// </summary>
#define IMPLEMENT_SCRIPTING_TYPE(type, baseType, assemblyType, typeName, setupScriptVTable, setupScriptObjectVTable) \
    ScriptingTypeInitializer type::TypeInitializer \
    ( \
	    (BinaryModule*)CONCAT_MACROS(GetBinaryModule, assemblyType)(), \
	    StringAnsiView(typeName, ARRAY_COUNT(typeName) - 1), \
        sizeof(type), \
	    &type##Internal::InitRuntime, \
	    (ScriptingType::SpawnHandler)&type::Spawn, \
	    &baseType::TypeInitializer, \
	    setupScriptVTable, \
	    setupScriptObjectVTable \
    );

/// <summary>
/// Helper define used to implement required components for native types that have managed type (for objects that can be spawned).
/// </summary>
#define IMPLEMENT_SCRIPTING_TYPE_NO_BASE(type, assemblyType, typeName, setupScriptVTable, setupScriptObjectVTable) \
    ScriptingTypeInitializer type::TypeInitializer \
    ( \
	    (BinaryModule*)CONCAT_MACROS(GetBinaryModule, assemblyType)(), \
	    StringAnsiView(typeName, ARRAY_COUNT(typeName) - 1), \
        sizeof(type), \
	    &type##Internal::InitRuntime, \
	    (ScriptingType::SpawnHandler)&type::Spawn, \
	    nullptr, \
	    setupScriptVTable, \
	    setupScriptObjectVTable \
    );

/// <summary>
/// Helper define used to implement required components for native types that have managed type (for objects that cannot be spawned). With base class specified.
/// </summary>
#define IMPLEMENT_SCRIPTING_TYPE_NO_SPAWN_WITH_BASE(type, baseType, assemblyType, typeName, setupScriptVTable, setupScriptObjectVTable) \
    ScriptingTypeInitializer type::TypeInitializer \
    ( \
	    (BinaryModule*)CONCAT_MACROS(GetBinaryModule, assemblyType)(), \
	    StringAnsiView(typeName, ARRAY_COUNT(typeName) - 1), \
        sizeof(type), \
	    &type##Internal::InitRuntime, \
	    &ScriptingType::DefaultSpawn, \
	    &baseType::TypeInitializer, \
	    setupScriptVTable, \
	    setupScriptObjectVTable \
    );

/// <summary>
/// Helper define used to implement required components for native types that have managed type (for objects that cannot be spawned).
/// </summary>
#define IMPLEMENT_SCRIPTING_TYPE_NO_SPAWN(type, assemblyType, typeName, setupScriptVTable, setupScriptObjectVTable) \
    ScriptingTypeInitializer type::TypeInitializer \
    ( \
	    (BinaryModule*)CONCAT_MACROS(GetBinaryModule, assemblyType)(), \
	    StringAnsiView(typeName, ARRAY_COUNT(typeName) - 1), \
        sizeof(type), \
	    &type##Internal::InitRuntime, \
	    &ScriptingType::DefaultSpawn, \
	    nullptr, \
	    setupScriptVTable, \
	    setupScriptObjectVTable \
    );

/// <summary>
/// The core library assembly. Main C# library with core functionalities.
/// </summary>
extern "C" FLAXENGINE_API ManagedBinaryModule* GetBinaryModuleCorlib();

FORCE_INLINE int32 GetVTablePrefix()
{
#if defined(_MSC_VER)
    // Include size of the RTTI Complete Object Locator that lays before the vtable in the memory.
    return 96;
#elif defined(__clang__)
    // Something important is before vtable in memory.
    return 48;
#else
    return 0;
#endif
}

FORCE_INLINE int32 GetVTableIndex(void** vtable, int32 entriesCount, void* func)
{
#if defined(_MSC_VER) && (PLATFORM_ARCH_X86 || PLATFORM_ARCH_X64)
    // On Microsoft x86/x64 pointer to member function points to a thunk jump instruction,
    // which points to the piece of code that loads vtable from a register and jump to one of it's entries.
    // Here we have to deduct this based on x86 instructions. Member function thunk points to the following code:
    // mov rax, qword ptr [rcx]
    // jmp qword ptr [rax+XXX]
    // where XXX is the actual vtable offset we need to read.
    byte* funcJmp = *(byte*)func == 0x48 ? (byte*)func : (byte*)func + 5 + *(int32*)((byte*)func + 1);
    funcJmp += 5;
    if (*(funcJmp - 1) == 0xa0)
        return *(int32*)funcJmp / sizeof(void*);
    return *(byte*)funcJmp / sizeof(void*);
#elif defined(__clang__)
    // On Clang member function pointer represents the offset from the vtable begin.
    return (int32)(intptr)func / sizeof(void*);
#else
#error "TODO: implement C++ objects vtable hacking for C# scripts."
#endif
}
