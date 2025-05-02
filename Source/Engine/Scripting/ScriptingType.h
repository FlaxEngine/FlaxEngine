// Copyright (c) Wojciech Figat. All rights reserved.

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

    FORCE_INLINE ScriptingTypeHandle(ScriptingTypeHandle&& other)
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
#if USE_CSHARP
    MClass* GetClass() const;
#endif
    bool IsSubclassOf(ScriptingTypeHandle c) const;
    bool IsAssignableFrom(ScriptingTypeHandle c) const;

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

// C++ templated type accessor for scripting types.
template<typename Type> ScriptingTypeHandle StaticType();

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
    typedef MObject* (*Box)(void* ptr);
    typedef void (*Unbox)(void* ptr, MObject* managed);
    typedef void (*GetField)(void* ptr, const String& name, Variant& value);
    typedef void (*SetField)(void* ptr, const String& name, const Variant& value);
    typedef void* (*GetInterfaceWrapper)(ScriptingObject* obj);
    typedef struct { uint64 Value; const char* Name; } EnumItem;

    struct InterfaceImplementation
    {
        // Pointer to the type of the implemented interface.
        const ScriptingTypeHandle* InterfaceType;

        // The offset (in bytes) from the object pointer to the interface implementation. Used for casting object to the interface.
        int16 VTableOffset;

        // The offset (in entries) from the script vtable to the interface implementation. Used for initializing interface virtual script methods.
        int16 ScriptVTableOffset;

        // True if interface implementation is native (inside C++ object), otherwise it's injected at scripting level (cannot call interface directly).
        bool IsNative;
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
            /// List of offsets from native methods VTable for each interface (with virtual methods). Null if not using interfaces with method overrides.
            /// </summary>
            uint16* InterfacesOffsets;

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
            // Enum items table (the last item name is null)
            EnumItem* Items;
        } Enum;

        struct
        {
            // Class constructor method pointer
            Ctor Ctor;

            // Class destructor method pointer
            Dtor Dtor;
        } Class;

        struct
        {
            SetupScriptVTableHandler SetupScriptVTable;
            SetupScriptObjectVTableHandler SetupScriptObjectVTable;
            GetInterfaceWrapper GetInterfaceWrapper;
        } Interface;
    };

    ScriptingType();
    ScriptingType(const StringAnsiView& fullname, BinaryModule* module, int32 size, InitRuntimeHandler initRuntime, SpawnHandler spawn, const ScriptingTypeHandle& baseType, SetupScriptVTableHandler setupScriptVTable = nullptr, SetupScriptObjectVTableHandler setupScriptObjectVTable = nullptr, const InterfaceImplementation* interfaces = nullptr);
    ScriptingType(const StringAnsiView& fullname, BinaryModule* module, int32 size, InitRuntimeHandler initRuntime = DefaultInitRuntime, SpawnHandler spawn = DefaultSpawn, ScriptingTypeInitializer* baseType = nullptr, SetupScriptVTableHandler setupScriptVTable = nullptr, SetupScriptObjectVTableHandler setupScriptObjectVTable = nullptr, const InterfaceImplementation* interfaces = nullptr);
    ScriptingType(const StringAnsiView& fullname, BinaryModule* module, int32 size, InitRuntimeHandler initRuntime, Ctor ctor, Dtor dtor, ScriptingTypeInitializer* baseType, const InterfaceImplementation* interfaces = nullptr);
    ScriptingType(const StringAnsiView& fullname, BinaryModule* module, int32 size, InitRuntimeHandler initRuntime, Ctor ctor, Dtor dtor, Copy copy, Box box, Unbox unbox, GetField getField, SetField setField, ScriptingTypeInitializer* baseType, const InterfaceImplementation* interfaces = nullptr);
    ScriptingType(const StringAnsiView& fullname, BinaryModule* module, int32 size, EnumItem* items);
    ScriptingType(const StringAnsiView& fullname, BinaryModule* module, InitRuntimeHandler initRuntime, SetupScriptVTableHandler setupScriptVTable, SetupScriptObjectVTableHandler setupScriptObjectVTable, GetInterfaceWrapper getInterfaceWrapper);
    ScriptingType(const ScriptingType& other);
    ScriptingType(ScriptingType&& other);
    ScriptingType& operator=(ScriptingType&& other) = delete;
    ScriptingType& operator=(const ScriptingType& other) = delete;
    ~ScriptingType();

    static void DefaultInitRuntime();
    static ScriptingObject* DefaultSpawn(const ScriptingObjectSpawnParams& params);

    /// <summary>
    /// Gets the handle to this type.
    /// </summary>
    ScriptingTypeHandle GetHandle() const;

    /// <summary>
    /// Gets the handle to the base type of this type.
    /// </summary>
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
    const InterfaceImplementation* GetInterface(const ScriptingTypeHandle& interfaceType) const;

    void SetupScriptVTable(ScriptingTypeHandle baseTypeHandle);
    void SetupScriptObjectVTable(void* object, ScriptingTypeHandle baseTypeHandle, int32 wrapperIndex);
    void HackObjectVTable(void* object, ScriptingTypeHandle baseTypeHandle, int32 wrapperIndex);
    String ToString() const;
    StringAnsiView GetName() const;
};

/// <summary>
/// The helper type for scripting type initialization in the assembly.
/// </summary>
struct FLAXENGINE_API ScriptingTypeInitializer : ScriptingTypeHandle
{
    ScriptingTypeInitializer(BinaryModule* module, const StringAnsiView& fullname, int32 size, ScriptingType::InitRuntimeHandler initRuntime = ScriptingType::DefaultInitRuntime, ScriptingType::SpawnHandler spawn = ScriptingType::DefaultSpawn, ScriptingTypeInitializer* baseType = nullptr, ScriptingType::SetupScriptVTableHandler setupScriptVTable = nullptr, ScriptingType::SetupScriptObjectVTableHandler setupScriptObjectVTable = nullptr, const ScriptingType::InterfaceImplementation* interfaces = nullptr);
    ScriptingTypeInitializer(BinaryModule* module, const StringAnsiView& fullname, int32 size, ScriptingType::InitRuntimeHandler initRuntime, ScriptingType::Ctor ctor, ScriptingType::Dtor dtor, ScriptingTypeInitializer* baseType = nullptr, const ScriptingType::InterfaceImplementation* interfaces = nullptr);
    ScriptingTypeInitializer(BinaryModule* module, const StringAnsiView& fullname, int32 size, ScriptingType::InitRuntimeHandler initRuntime, ScriptingType::Ctor ctor, ScriptingType::Dtor dtor, ScriptingType::Copy copy, ScriptingType::Box box, ScriptingType::Unbox unbox, ScriptingType::GetField getField, ScriptingType::SetField setField, ScriptingTypeInitializer* baseType = nullptr, const ScriptingType::InterfaceImplementation* interfaces = nullptr);
    ScriptingTypeInitializer(BinaryModule* module, const StringAnsiView& fullname, int32 size, ScriptingType::EnumItem* items);
    ScriptingTypeInitializer(BinaryModule* module, const StringAnsiView& fullname, ScriptingType::InitRuntimeHandler initRuntime, ScriptingType::SetupScriptVTableHandler setupScriptVTable, ScriptingType::SetupScriptObjectVTableHandler setupScriptObjectVTable, ScriptingType::GetInterfaceWrapper getInterfaceWrapper);
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
    /// The object type handle.
    /// </summary>
    const ScriptingTypeHandle Type;

    /// <summary>
    /// Optional C# object instance to use for unmanaged object.
    /// </summary>
    void* Managed;

    FORCE_INLINE ScriptingObjectSpawnParams(const Guid& id, const ScriptingTypeHandle& typeHandle)
        : ID(id)
        , Type(typeHandle)
        , Managed(nullptr)
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
    const byte op = *(funcJmp - 1);
    if (op == 0xa0)
        return *(int32*)funcJmp / sizeof(void*);
    if (op == 0x20)
        return 0;
    return *(byte*)funcJmp / sizeof(void*);
#elif defined(_MSC_VER) && PLATFORM_ARCH_ARM64
    // For MSVC ARM64, the following thunk takes a relative jump from the function pointer to the next thunk:
    // adrp xip0, offset_high
    // add xip0, xip0, offset_low
    // br xip0
    // The last thunk contains the offset to the vtable:
    // ldr xip0, [x0]
    // ldr xip0, [xip0, XXX]
    uint32_t* op = (uint32_t*)func;

    uint32_t def = *op;
    if ((*op & 0x9F000000) == 0x90000000)
    {
        // adrp
        uint32_t imm20 = (((*op & 0x60000000) >> 29) + ((*op & 0xFFFFE0) >> 3)) << 12;
        op++;

        // add
        def = *op;
        uint32_t imm12 = (*op & 0x3FFC00) >> 10;
        imm12 = (*op & 0x400000) != 0 ? (imm12 << 12) : imm12;

        // br
        op = (uint32_t*)(((uintptr)func & ((uintptr)-1 << 12)) + imm20 + imm12) + 1;

        // ldr + offset
        def = *op;
        uint32_t offset = ((*op & 0x3FFC00) >> 10) * ((*op & 0x40000000) != 0 ? 8 : 4);
        return offset / sizeof(void*);
    }
    else if ((*op & 0xBFC00000) == 0xB9400000)
    {
        // ldr + offset
        uint32_t offset = ((*op & 0x3FFC00) >> 10) * ((*op & 0x40000000) != 0 ? 8 : 4);
        op++;

        // ldr + offset
        def = *op;
        if ((*op & 0xBFE00C00) == 0xB8400400)
        {
            // offset is stored in the register as is
            uint32_t postindex = (*op & 0x1FF000) >> 12;
            offset = postindex;
            return offset / sizeof(void*);
        }
        else if ((*op & 0xBFE00C00) == 0xB8400C00)
        {
            // offset is added to the value in base register... updated to the same register
            uint32_t preindex = (*op & 0x1FF000) >> 12;
            offset += preindex;
            return offset / sizeof(void*);
        }
        else if ((*op & 0xBFC00000) == 0xB9400000)
        {
            // 20-bit offset
            offset = ((*op & 0x3FFC00) >> 10) * ((*op & 0x40000000) != 0 ? 8 : 4);
            return offset / sizeof(void*);
        }
        CRASH;
    }
#elif defined(__clang__)
    // On Clang member function pointer represents the offset from the vtable begin.
    return (int32)(intptr)func / sizeof(void*);
#else
#error "TODO: implement C++ objects vtable hacking for C# scripts."
#endif
}
