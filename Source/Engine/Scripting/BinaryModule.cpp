// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "BinaryModule.h"
#include "ScriptingObject.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Utilities.h"
#include "Engine/Threading/Threading.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "ManagedCLR/MAssembly.h"
#include "ManagedCLR/MClass.h"
#include "ManagedCLR/MMethod.h"
#include "ManagedCLR/MField.h"
#include "ManagedCLR/MProperty.h"
#include "ManagedCLR/MUtils.h"
#include "ManagedCLR/MException.h"
#include "FlaxEngine.Gen.h"
#include "Scripting.h"
#include "Events.h"
#include "Internal/StdTypesContainer.h"

Dictionary<Pair<ScriptingTypeHandle, StringView>, void(*)(ScriptingObject*, void*, bool)> ScriptingEvents::EventsTable;
Delegate<ScriptingObject*, Span<Variant>, ScriptingTypeHandle, StringView> ScriptingEvents::Event;

ManagedBinaryModule* GetBinaryModuleCorlib()
{
#if COMPILE_WITHOUT_CSHARP
    return nullptr;
#else
    static ManagedBinaryModule assembly("corlib");
    return &assembly;
#endif
}

ScriptingTypeHandle::ScriptingTypeHandle(const ScriptingTypeInitializer& initializer)
    : Module(initializer.Module)
    , TypeIndex(initializer.TypeIndex)
{
}

String ScriptingTypeHandle::ToString(bool withAssembly) const
{
    String result = GetType().ToString();
    if (withAssembly)
    {
        result += TEXT("(module ");
        result += String(Module->GetName());
        result += TEXT(")");
    }
    return result;
}

const ScriptingType& ScriptingTypeHandle::GetType() const
{
    ASSERT_LOW_LAYER(Module);
    return Module->Types[TypeIndex];
}

#if USE_CSHARP

MClass* ScriptingTypeHandle::GetClass() const
{
    ASSERT_LOW_LAYER(Module && Module->Types[TypeIndex].ManagedClass);
    return Module->Types[TypeIndex].ManagedClass;
}

#endif

bool ScriptingTypeHandle::IsSubclassOf(ScriptingTypeHandle c) const
{
    auto type = *this;
    if (type == c)
        return false;
    for (; type; type = type.GetType().GetBaseType())
    {
        if (type == c)
            return true;
    }
    return false;
}

bool ScriptingTypeHandle::IsAssignableFrom(ScriptingTypeHandle c) const
{
    while (c)
    {
        if (c == *this)
            return true;
        c = c.GetType().GetBaseType();
    }
    return false;
}

bool ScriptingTypeHandle::operator==(const ScriptingTypeInitializer& other) const
{
    return Module == other.Module && TypeIndex == other.TypeIndex;
}

bool ScriptingTypeHandle::operator!=(const ScriptingTypeInitializer& other) const
{
    return Module != other.Module || TypeIndex != other.TypeIndex;
}

ScriptingType::ScriptingType()
    : ManagedClass(nullptr)
    , Module(nullptr)
    , InitRuntime(nullptr)
    , Fullname(nullptr, 0)
    , Type(ScriptingTypes::Script)
    , BaseTypePtr(nullptr)
    , Interfaces(nullptr)
{
    Script.Spawn = nullptr;
    Script.VTable = nullptr;
    Script.InterfacesOffsets = nullptr;
    Script.ScriptVTable = nullptr;
    Script.ScriptVTableBase = nullptr;
    Script.SetupScriptVTable = nullptr;
    Script.SetupScriptObjectVTable = nullptr;
    Script.DefaultInstance = nullptr;
}

ScriptingType::ScriptingType(const StringAnsiView& fullname, BinaryModule* module, int32 size, InitRuntimeHandler initRuntime, SpawnHandler spawn, const ScriptingTypeHandle& baseType, SetupScriptVTableHandler setupScriptVTable, SetupScriptObjectVTableHandler setupScriptObjectVTable, const InterfaceImplementation* interfaces)
    : ManagedClass(nullptr)
    , Module(module)
    , InitRuntime(initRuntime)
    , Fullname(fullname)
    , Type(ScriptingTypes::Script)
    , BaseTypeHandle(baseType)
    , BaseTypePtr(nullptr)
    , Interfaces(interfaces)
    , Size(size)
{
    Script.Spawn = spawn;
    Script.VTable = nullptr;
    Script.InterfacesOffsets = nullptr;
    Script.ScriptVTable = nullptr;
    Script.ScriptVTableBase = nullptr;
    Script.SetupScriptVTable = setupScriptVTable;
    Script.SetupScriptObjectVTable = setupScriptObjectVTable;
    Script.DefaultInstance = nullptr;
}

ScriptingType::ScriptingType(const StringAnsiView& fullname, BinaryModule* module, int32 size, InitRuntimeHandler initRuntime, SpawnHandler spawn, ScriptingTypeInitializer* baseType, SetupScriptVTableHandler setupScriptVTable, SetupScriptObjectVTableHandler setupScriptObjectVTable, const InterfaceImplementation* interfaces)
    : ManagedClass(nullptr)
    , Module(module)
    , InitRuntime(initRuntime)
    , Fullname(fullname)
    , Type(ScriptingTypes::Script)
    , BaseTypePtr(baseType)
    , Interfaces(interfaces)
    , Size(size)
{
    Script.Spawn = spawn;
    Script.VTable = nullptr;
    Script.InterfacesOffsets = nullptr;
    Script.ScriptVTable = nullptr;
    Script.ScriptVTableBase = nullptr;
    Script.SetupScriptVTable = setupScriptVTable;
    Script.SetupScriptObjectVTable = setupScriptObjectVTable;
    Script.DefaultInstance = nullptr;
}

ScriptingType::ScriptingType(const StringAnsiView& fullname, BinaryModule* module, int32 size, InitRuntimeHandler initRuntime, Ctor ctor, Dtor dtor, ScriptingTypeInitializer* baseType, const InterfaceImplementation* interfaces)
    : ManagedClass(nullptr)
    , Module(module)
    , InitRuntime(initRuntime)
    , Fullname(fullname)
    , Type(ScriptingTypes::Class)
    , BaseTypePtr(baseType)
    , Interfaces(interfaces)
    , Size(size)
{
    Class.Ctor = ctor;
    Class.Dtor = dtor;
}

ScriptingType::ScriptingType(const StringAnsiView& fullname, BinaryModule* module, int32 size, InitRuntimeHandler initRuntime, Ctor ctor, Dtor dtor, Copy copy, Box box, Unbox unbox, GetField getField, SetField setField, ScriptingTypeInitializer* baseType, const InterfaceImplementation* interfaces)
    : ManagedClass(nullptr)
    , Module(module)
    , InitRuntime(initRuntime)
    , Fullname(fullname)
    , Type(ScriptingTypes::Structure)
    , BaseTypePtr(baseType)
    , Interfaces(interfaces)
    , Size(size)
{
    Struct.Ctor = ctor;
    Struct.Dtor = dtor;
    Struct.Copy = copy;
    Struct.Box = box;
    Struct.Unbox = unbox;
    Struct.GetField = getField;
    Struct.SetField = setField;
}

ScriptingType::ScriptingType(const StringAnsiView& fullname, BinaryModule* module, int32 size, EnumItem* items)
    : ManagedClass(nullptr)
    , Module(module)
    , InitRuntime(DefaultInitRuntime)
    , Fullname(fullname)
    , Type(ScriptingTypes::Enum)
    , BaseTypePtr(nullptr)
    , Interfaces(nullptr)
    , Size(size)
{
    Enum.Items = items;
}

ScriptingType::ScriptingType(const StringAnsiView& fullname, BinaryModule* module, InitRuntimeHandler initRuntime, SetupScriptVTableHandler setupScriptVTable, SetupScriptObjectVTableHandler setupScriptObjectVTable, GetInterfaceWrapper getInterfaceWrapper)
    : ManagedClass(nullptr)
    , Module(module)
    , InitRuntime(initRuntime)
    , Fullname(fullname)
    , Type(ScriptingTypes::Interface)
    , BaseTypePtr(nullptr)
    , Interfaces(nullptr)
    , Size(0)
{
    Interface.SetupScriptVTable = setupScriptVTable;
    Interface.SetupScriptObjectVTable = setupScriptObjectVTable;
    Interface.GetInterfaceWrapper = getInterfaceWrapper;
}

ScriptingType::ScriptingType(const ScriptingType& other)
    : ManagedClass(other.ManagedClass)
    , Module(other.Module)
    , InitRuntime(other.InitRuntime)
    , Fullname(other.Fullname)
    , Type(other.Type)
    , BaseTypeHandle(other.BaseTypeHandle)
    , BaseTypePtr(other.BaseTypePtr)
    , Interfaces(other.Interfaces)
    , Size(other.Size)
{
    switch (other.Type)
    {
    case ScriptingTypes::Script:
        Script.Spawn = other.Script.Spawn;
        Script.VTable = nullptr;
        Script.InterfacesOffsets = nullptr;
        Script.ScriptVTable = nullptr;
        Script.ScriptVTableBase = nullptr;
        Script.SetupScriptVTable = other.Script.SetupScriptVTable;
        Script.SetupScriptObjectVTable = other.Script.SetupScriptObjectVTable;
        Script.DefaultInstance = nullptr;
        break;
    case ScriptingTypes::Structure:
        Struct.Ctor = other.Struct.Ctor;
        Struct.Dtor = other.Struct.Dtor;
        Struct.Copy = other.Struct.Copy;
        Struct.Box = other.Struct.Box;
        Struct.Unbox = other.Struct.Unbox;
        Struct.GetField = other.Struct.GetField;
        Struct.SetField = other.Struct.SetField;
        break;
    case ScriptingTypes::Class:
        Class.Ctor = other.Class.Ctor;
        Class.Dtor = other.Class.Dtor;
        break;
    case ScriptingTypes::Enum:
        Enum.Items = other.Enum.Items;
        break;
    case ScriptingTypes::Interface:
        Interface.SetupScriptVTable = other.Interface.SetupScriptVTable;
        Interface.SetupScriptObjectVTable = other.Interface.SetupScriptObjectVTable;
        Interface.GetInterfaceWrapper = other.Interface.GetInterfaceWrapper;
        break;
    default: ;
    }
}

ScriptingType::ScriptingType(ScriptingType&& other)
    : ManagedClass(other.ManagedClass)
    , Module(other.Module)
    , InitRuntime(other.InitRuntime)
    , Fullname(other.Fullname)
    , Type(other.Type)
    , BaseTypeHandle(other.BaseTypeHandle)
    , BaseTypePtr(other.BaseTypePtr)
    , Interfaces(other.Interfaces)
    , Size(other.Size)
{
    switch (other.Type)
    {
    case ScriptingTypes::Script:
        Script.Spawn = other.Script.Spawn;
        Script.VTable = other.Script.VTable;
        other.Script.VTable = nullptr;
        Script.InterfacesOffsets = other.Script.InterfacesOffsets;
        other.Script.InterfacesOffsets = nullptr;
        Script.ScriptVTable = other.Script.ScriptVTable;
        other.Script.ScriptVTable = nullptr;
        Script.ScriptVTableBase = other.Script.ScriptVTableBase;
        other.Script.ScriptVTableBase = nullptr;
        Script.SetupScriptVTable = other.Script.SetupScriptVTable;
        Script.SetupScriptObjectVTable = other.Script.SetupScriptObjectVTable;
        Script.DefaultInstance = other.Script.DefaultInstance;
        other.Script.DefaultInstance = nullptr;
        break;
    case ScriptingTypes::Structure:
        Struct.Ctor = other.Struct.Ctor;
        Struct.Dtor = other.Struct.Dtor;
        Struct.Copy = other.Struct.Copy;
        Struct.Box = other.Struct.Box;
        Struct.Unbox = other.Struct.Unbox;
        Struct.GetField = other.Struct.GetField;
        Struct.SetField = other.Struct.SetField;
        break;
    case ScriptingTypes::Class:
        Class.Ctor = other.Class.Ctor;
        Class.Dtor = other.Class.Dtor;
        break;
    case ScriptingTypes::Enum:
        Enum.Items = other.Enum.Items;
        break;
    case ScriptingTypes::Interface:
        Interface.SetupScriptVTable = other.Interface.SetupScriptVTable;
        Interface.SetupScriptObjectVTable = other.Interface.SetupScriptObjectVTable;
        Interface.GetInterfaceWrapper = other.Interface.GetInterfaceWrapper;
        break;
    default: ;
    }
}

ScriptingType::~ScriptingType()
{
    switch (Type)
    {
    case ScriptingTypes::Script:
        if (Script.DefaultInstance)
            Delete(Script.DefaultInstance);
        if (Script.VTable)
            Platform::Free((byte*)Script.VTable - GetVTablePrefix());
        Platform::Free(Script.InterfacesOffsets);
        Platform::Free(Script.ScriptVTable);
        Platform::Free(Script.ScriptVTableBase);
        break;
    case ScriptingTypes::Structure:
        break;
    case ScriptingTypes::Enum:
        break;
    case ScriptingTypes::Interface:
        break;
    default: ;
    }
}

void ScriptingType::DefaultInitRuntime()
{
}

ScriptingObject* ScriptingType::DefaultSpawn(const ScriptingObjectSpawnParams& params)
{
    return nullptr;
}

ScriptingTypeHandle ScriptingType::GetHandle() const
{
    int32 typeIndex;
    if (Module && Module->FindScriptingType(Fullname, typeIndex))
    {
        return ScriptingTypeHandle(Module, typeIndex);
    }
    return ScriptingTypeHandle();
}

ScriptingObject* ScriptingType::GetDefaultInstance() const
{
    ASSERT(Type == ScriptingTypes::Script);
    if (!Script.DefaultInstance)
    {
        const ScriptingObjectSpawnParams params(Guid::New(), GetHandle());
        Script.DefaultInstance = Script.Spawn(params);
        if (!Script.DefaultInstance)
        {
            LOG(Error, "Failed to create default instance of type {0}", ToString());
        }
    }
    return Script.DefaultInstance;
}

const ScriptingType::InterfaceImplementation* ScriptingType::GetInterface(const ScriptingTypeHandle& interfaceType) const
{
    const InterfaceImplementation* interfaces = Interfaces;
    if (interfaces)
    {
        while (interfaces->InterfaceType)
        {
            if (*interfaces->InterfaceType == interfaceType)
                return interfaces;
            interfaces++;
        }
    }
    if (BaseTypeHandle)
    {
        return BaseTypeHandle.GetType().GetInterface(interfaceType);
    }
    if (BaseTypePtr)
    {
        return BaseTypePtr->GetType().GetInterface(interfaceType);
    }
    return nullptr;
}

void ScriptingType::SetupScriptVTable(ScriptingTypeHandle baseTypeHandle)
{
    // Call setup for all class starting from the first native type (first that uses virtual calls will allocate table of a proper size, further base types will just add own methods)
    for (ScriptingTypeHandle e = baseTypeHandle; e;)
    {
        const ScriptingType& eType = e.GetType();

        if (eType.Script.SetupScriptVTable)
        {
            ASSERT(eType.ManagedClass);
            eType.Script.SetupScriptVTable(eType.ManagedClass, Script.ScriptVTable, Script.ScriptVTableBase);
        }

        auto interfaces = eType.Interfaces;
        if (interfaces && Script.ScriptVTable)
        {
            while (interfaces->InterfaceType)
            {
                auto& interfaceType = interfaces->InterfaceType->GetType();
                if (interfaceType.Interface.SetupScriptVTable)
                {
                    ASSERT(eType.ManagedClass);
                    const auto scriptOffset = interfaces->ScriptVTableOffset; // Shift the script vtable for the interface implementation start
                    Script.ScriptVTable += scriptOffset;
                    Script.ScriptVTableBase += scriptOffset;
                    interfaceType.Interface.SetupScriptVTable(eType.ManagedClass, Script.ScriptVTable, Script.ScriptVTableBase);
                    Script.ScriptVTable -= scriptOffset;
                    Script.ScriptVTableBase -= scriptOffset;
                }
                interfaces++;
            }
        }
        e = eType.GetBaseType();
    }
}

NO_SANITIZE_ADDRESS
void ScriptingType::SetupScriptObjectVTable(void* object, ScriptingTypeHandle baseTypeHandle, int32 wrapperIndex)
{
    // Analyze vtable size
    void** vtable = *(void***)object;
    const int32 prefixSize = GetVTablePrefix();
    int32 entriesCount = 0;
    while (vtable[entriesCount] && entriesCount < 200)
        entriesCount++;

    // Calculate total vtable size by adding all implemented interfaces that use virtual methods
    const int32 size = entriesCount * sizeof(void*);
    int32 totalSize = prefixSize + size;
    int32 interfacesCount = 0;
    for (ScriptingTypeHandle e = baseTypeHandle; e;)
    {
        const ScriptingType& eType = e.GetType();
        auto interfaces = eType.Interfaces;
        if (interfaces)
        {
            while (interfaces->InterfaceType)
            {
                auto& interfaceType = interfaces->InterfaceType->GetType();
                if (interfaceType.Interface.SetupScriptObjectVTable)
                {
                    void** vtableInterface = *(void***)((byte*)object + interfaces->VTableOffset);
                    int32 interfaceCount = 0;
                    while (vtableInterface[interfaceCount] && interfaceCount < 200)
                        interfaceCount++;
                    totalSize += prefixSize + interfaceCount * sizeof(void*);
                    interfacesCount++;
                }
                interfaces++;
            }
        }
        e = eType.GetBaseType();
    }

    // Duplicate vtable
    Script.VTable = (void**)((byte*)Platform::Allocate(totalSize, 16) + prefixSize);
    Utilities::UnsafeMemoryCopy((byte*)Script.VTable - prefixSize, (byte*)vtable - prefixSize, prefixSize + size);

    // Override vtable entries
    if (interfacesCount)
        Script.InterfacesOffsets = (uint16*)Platform::Allocate(interfacesCount * sizeof(uint16*), 16);
    int32 interfaceOffset = size;
    interfacesCount = 0;
    for (ScriptingTypeHandle e = baseTypeHandle; e;)
    {
        const ScriptingType& eType = e.GetType();

        if (eType.Script.SetupScriptObjectVTable)
        {
            // Override vtable entries for this class
            eType.Script.SetupScriptObjectVTable(Script.ScriptVTable, Script.ScriptVTableBase, Script.VTable, entriesCount, wrapperIndex);
        }

        auto interfaces = eType.Interfaces;
        if (interfaces)
        {
            while (interfaces->InterfaceType)
            {
                auto& interfaceType = interfaces->InterfaceType->GetType();
                if (interfaceType.Interface.SetupScriptObjectVTable)
                {
                    // Analyze interface vtable size
                    void** vtableInterface = *(void***)((byte*)object + interfaces->VTableOffset);
                    int32 interfaceCount = 0;
                    while (vtableInterface[interfaceCount] && interfaceCount < 200)
                        interfaceCount++;
                    const int32 interfaceSize = interfaceCount * sizeof(void*);

                    // Duplicate interface vtable
                    Utilities::UnsafeMemoryCopy((byte*)Script.VTable + interfaceOffset, (byte*)vtableInterface - prefixSize, prefixSize + interfaceSize);

                    // Override interface vtable entries
                    const auto scriptOffset = interfaces->ScriptVTableOffset;
                    const auto nativeOffset = interfaceOffset + prefixSize;
                    void** interfaceVTable = (void**)((byte*)Script.VTable + nativeOffset);
                    interfaceType.Interface.SetupScriptObjectVTable(Script.ScriptVTable + scriptOffset, Script.ScriptVTableBase + scriptOffset, interfaceVTable, interfaceCount, wrapperIndex);

                    Script.InterfacesOffsets[interfacesCount++] = (uint16)nativeOffset;
                    interfaceOffset += prefixSize + interfaceSize;
                }
                interfaces++;
            }
        }
        e = eType.GetBaseType();
    }
}

void ScriptingType::HackObjectVTable(void* object, ScriptingTypeHandle baseTypeHandle, int32 wrapperIndex)
{
    if (!Script.ScriptVTable)
        return;
    if (!Script.VTable)
    {
        // Ensure to have valid Script VTable hacked
        BinaryModule::Locker.Lock();
        if (!Script.VTable)
        {
            SetupScriptObjectVTable(object, baseTypeHandle, wrapperIndex);
        }
        BinaryModule::Locker.Unlock();
    }

    // Override object vtable with hacked one that has calls to overriden scripting functions
    *(void**)object = Script.VTable;

    if (Script.InterfacesOffsets)
    {
        // Override vtables for interfaces
        int32 interfacesCount = 0;
        for (ScriptingTypeHandle e = baseTypeHandle; e;)
        {
            const ScriptingType& eType = e.GetType();
            auto interfaces = eType.Interfaces;
            if (interfaces)
            {
                while (interfaces->InterfaceType)
                {
                    auto& interfaceType = interfaces->InterfaceType->GetType();
                    if (interfaceType.Interface.SetupScriptObjectVTable)
                    {
                        void** interfaceVTable = (void**)((byte*)Script.VTable + Script.InterfacesOffsets[interfacesCount++]);
                        *(void**)((byte*)object + interfaces->VTableOffset) = interfaceVTable;
                        interfacesCount++;
                    }
                    interfaces++;
                }
            }
            e = eType.GetBaseType();
        }
    }
}

String ScriptingType::ToString() const
{
    return String(Fullname.Get(), Fullname.Length());
}

StringAnsiView ScriptingType::GetName() const
{
    int32 lastDotIndex = Fullname.FindLast('.');
    if (lastDotIndex != -1)
    {
        lastDotIndex++;
        return StringAnsiView(Fullname.Get() + lastDotIndex, Fullname.Length() - lastDotIndex);
    }
    return Fullname;
}

ScriptingTypeInitializer::ScriptingTypeInitializer(BinaryModule* module, const StringAnsiView& fullname, int32 size, ScriptingType::InitRuntimeHandler initRuntime, ScriptingType::SpawnHandler spawn, ScriptingTypeInitializer* baseType, ScriptingType::SetupScriptVTableHandler setupScriptVTable, ScriptingType::SetupScriptObjectVTableHandler setupScriptObjectVTable, const ScriptingType::InterfaceImplementation* interfaces)
    : ScriptingTypeHandle(module, module->Types.Count())
{
    // Script
    module->Types.AddUninitialized();
    new(module->Types.Get() + TypeIndex)ScriptingType(fullname, module, size, initRuntime, spawn, baseType, setupScriptVTable, setupScriptObjectVTable, interfaces);
#if BUILD_DEBUG
    if (module->TypeNameToTypeIndex.ContainsKey(fullname))
        LOG(Error, "Duplicated native typename {0} from module {1}.", String(fullname), String(module->GetName()));
#endif
    module->TypeNameToTypeIndex[fullname] = TypeIndex;
}

ScriptingTypeInitializer::ScriptingTypeInitializer(BinaryModule* module, const StringAnsiView& fullname, int32 size, ScriptingType::InitRuntimeHandler initRuntime, ScriptingType::Ctor ctor, ScriptingType::Dtor dtor, ScriptingTypeInitializer* baseType, const ScriptingType::InterfaceImplementation* interfaces)
    : ScriptingTypeHandle(module, module->Types.Count())
{
    // Class
    module->Types.AddUninitialized();
    new(module->Types.Get() + TypeIndex)ScriptingType(fullname, module, size, initRuntime, ctor, dtor, baseType, interfaces);
#if BUILD_DEBUG
    if (module->TypeNameToTypeIndex.ContainsKey(fullname))
        LOG(Error, "Duplicated native typename {0} from module {1}.", String(fullname), String(module->GetName()));
#endif
    module->TypeNameToTypeIndex[fullname] = TypeIndex;
}

ScriptingTypeInitializer::ScriptingTypeInitializer(BinaryModule* module, const StringAnsiView& fullname, int32 size, ScriptingType::InitRuntimeHandler initRuntime, ScriptingType::Ctor ctor, ScriptingType::Dtor dtor, ScriptingType::Copy copy, ScriptingType::Box box, ScriptingType::Unbox unbox, ScriptingType::GetField getField, ScriptingType::SetField setField, ScriptingTypeInitializer* baseType, const ScriptingType::InterfaceImplementation* interfaces)
    : ScriptingTypeHandle(module, module->Types.Count())
{
    // Structure
    module->Types.AddUninitialized();
    new(module->Types.Get() + TypeIndex)ScriptingType(fullname, module, size, initRuntime, ctor, dtor, copy, box, unbox, getField, setField, baseType, interfaces);
#if BUILD_DEBUG
    if (module->TypeNameToTypeIndex.ContainsKey(fullname))
        LOG(Error, "Duplicated native typename {0} from module {1}.", String(fullname), String(module->GetName()));
#endif
    module->TypeNameToTypeIndex[fullname] = TypeIndex;
}

ScriptingTypeInitializer::ScriptingTypeInitializer(BinaryModule* module, const StringAnsiView& fullname, int32 size, ScriptingType::EnumItem* items)
    : ScriptingTypeHandle(module, module->Types.Count())
{
    // Enum
    module->Types.AddUninitialized();
    new(module->Types.Get() + TypeIndex)ScriptingType(fullname, module, size, items);
#if BUILD_DEBUG
    if (module->TypeNameToTypeIndex.ContainsKey(fullname))
        LOG(Error, "Duplicated native typename {0} from module {1}.", String(fullname), String(module->GetName()));
#endif
    module->TypeNameToTypeIndex[fullname] = TypeIndex;
}

ScriptingTypeInitializer::ScriptingTypeInitializer(BinaryModule* module, const StringAnsiView& fullname, ScriptingType::InitRuntimeHandler initRuntime, ScriptingType::SetupScriptVTableHandler setupScriptVTable, ScriptingType::SetupScriptObjectVTableHandler setupScriptObjectVTable, ScriptingType::GetInterfaceWrapper getInterfaceWrapper)
    : ScriptingTypeHandle(module, module->Types.Count())
{
    // Interface
    module->Types.AddUninitialized();
    new(module->Types.Get() + TypeIndex)ScriptingType(fullname, module, initRuntime, setupScriptVTable, setupScriptObjectVTable, getInterfaceWrapper);
#if BUILD_DEBUG
    if (module->TypeNameToTypeIndex.ContainsKey(fullname))
        LOG(Error, "Duplicated native typename {0} from module {1}.", String(fullname), String(module->GetName()));
#endif
    module->TypeNameToTypeIndex[fullname] = TypeIndex;
}

CriticalSection BinaryModule::Locker;

BinaryModule::BinaryModulesList& BinaryModule::GetModules()
{
    static BinaryModulesList modules;
    return modules;
}

BinaryModule* BinaryModule::GetModule(const StringAnsiView& name)
{
    BinaryModule* result = nullptr;
    auto& modules = GetModules();
    for (int32 i = 0; i < modules.Count(); i++)
    {
        if (modules[i]->GetName() == name)
        {
            result = modules[i];
            break;
        }
    }
    return result;
}

BinaryModule::BinaryModule()
{
    // Register
    GetModules().Add(this);
}

void* BinaryModule::FindMethod(const ScriptingTypeHandle& typeHandle, const ScriptingTypeMethodSignature& signature)
{
    return FindMethod(typeHandle, signature.Name, signature.Params.Count());
}

void BinaryModule::Destroy(bool isReloading)
{
    // Destroy any default script instances
    for (const auto& type : Types)
    {
        if (type.Type == ScriptingTypes::Script && type.Script.DefaultInstance)
        {
            Delete(type.Script.DefaultInstance);
            type.Script.DefaultInstance = nullptr;
        }
    }

    // Remove any scripting events
    for (auto i = ScriptingEvents::EventsTable.Begin(); i.IsNotEnd(); ++i)
    {
        const ScriptingTypeHandle type = i->Key.First;
        if (type.Module == this)
            ScriptingEvents::EventsTable.Remove(i);
    }

    // Unregister
    GetModules().RemoveKeepOrder(this);
}

ManagedBinaryModule::ManagedBinaryModule(const StringAnsiView& name)
    : ManagedBinaryModule(New<MAssembly>(nullptr, name))
{
}

ManagedBinaryModule::ManagedBinaryModule(MAssembly* assembly)
{
    Assembly = assembly;

    // Bind for C# assembly events
    assembly->Loading.Bind<ManagedBinaryModule, &ManagedBinaryModule::OnLoading>(this);
    assembly->Loaded.Bind<ManagedBinaryModule, &ManagedBinaryModule::OnLoaded>(this);
    assembly->Unloading.Bind<ManagedBinaryModule, &ManagedBinaryModule::OnUnloading>(this);
    assembly->Unloaded.Bind<ManagedBinaryModule, &ManagedBinaryModule::OnUnloaded>(this);

    if (Assembly->IsLoaded())
    {
        // Cache stuff if the input assembly has been already loaded
        OnLoaded(Assembly);
    }
}

ManagedBinaryModule::~ManagedBinaryModule()
{
    // Unregister
    auto& modules = GetModules();
    modules.RemoveKeepOrder(this);

    // Cleanup
    Delete(Assembly);
}

ManagedBinaryModule* ManagedBinaryModule::GetModule(const MAssembly* assembly)
{
    ManagedBinaryModule* result = nullptr;
    auto& modules = GetModules();
    for (int32 i = 0; i < modules.Count(); i++)
    {
        auto e = dynamic_cast<ManagedBinaryModule*>(modules[i]);
        if (e && e->Assembly == assembly)
        {
            result = e;
            break;
        }
    }
    return result;
}

ScriptingObject* ManagedBinaryModule::ManagedObjectSpawn(const ScriptingObjectSpawnParams& params)
{
    // Create native object
    ScriptingTypeHandle managedTypeHandle = params.Type;
    const ScriptingType* managedTypePtr = &managedTypeHandle.GetType();
    if (managedTypePtr->ManagedClass && managedTypePtr->ManagedClass->IsAbstract())
    {
        LOG(Error, "Failed to spawn abstract type '{}'", managedTypePtr->ToString());
        return nullptr;
    }
    while (managedTypePtr->Script.Spawn != &ManagedObjectSpawn)
    {
        managedTypeHandle = managedTypePtr->GetBaseType();
        managedTypePtr = &managedTypeHandle.GetType();
    }
    ScriptingType& managedType = (ScriptingType&)*managedTypePtr;
    ScriptingTypeHandle nativeTypeHandle = managedType.GetBaseType();
    const ScriptingType* nativeTypePtr = &nativeTypeHandle.GetType();
    while (nativeTypePtr->Script.Spawn == &ManagedObjectSpawn)
    {
        nativeTypeHandle = nativeTypePtr->GetBaseType();
        nativeTypePtr = &nativeTypeHandle.GetType();
    }
    ScriptingObject* object = nativeTypePtr->Script.Spawn(params);
    if (!object)
    {
        LOG(Error, "Failed to spawn object of type {0} with native base type {1}.", managedTypePtr->ToString(), nativeTypePtr->ToString());
        return nullptr;
    }

    // Beware! Hacking vtables incoming! Undefined behaviors exploits! Low-level programming!
    managedType.HackObjectVTable(object, nativeTypeHandle, 0);

    // Mark as managed type
    object->Flags |= ObjectFlags::IsManagedType;

    return object;
}

#if !COMPILE_WITHOUT_CSHARP

namespace
{
    MMethod* FindMethod(MClass* mclass, const MMethod* referenceMethod)
    {
        const Array<MMethod*>& methods = mclass->GetMethods();
        for (int32 i = 0; i < methods.Count(); i++)
        {
            MMethod* method = methods[i];
            if (!method->IsStatic() &&
                method->GetName() == referenceMethod->GetName() &&
                method->GetParametersCount() == referenceMethod->GetParametersCount() &&
                method->GetReturnType() == referenceMethod->GetReturnType()
            )
            {
                return method;
            }
        }
        return nullptr;
    }

    bool VariantTypeEquals(const VariantType& type, MType* mType, bool isOut = false)
    {
        MClass* mClass = MCore::Type::GetClass(mType);
        MClass* variantClass = MUtils::GetClass(type);
        if (variantClass != mClass)
        {
            // Hack for Vector2/3/4 which alias with Float2/3/4 or Double2/3/4 (depending on USE_LARGE_WORLDS)
            const auto& stdTypes = *StdTypesContainer::Instance();
            if (mClass == stdTypes.Vector2Class && (type.Type == VariantType::Float2 || type.Type == VariantType::Double2))
                return true;
            if (mClass == stdTypes.Vector3Class && (type.Type == VariantType::Float3 || type.Type == VariantType::Double3))
                return true;
            if (mClass == stdTypes.Vector4Class && (type.Type == VariantType::Float4 || type.Type == VariantType::Double4))
                return true;

            return false;
        }
        return true;
    }
}

#endif

MMethod* ManagedBinaryModule::FindMethod(MClass* mclass, const ScriptingTypeMethodSignature& signature)
{
#if USE_CSHARP
    if (!mclass)
        return nullptr;
    const auto& methods = mclass->GetMethods();
    for (MMethod* method : methods)
    {
        if (method->IsStatic() != signature.IsStatic)
            continue;
        if (method->GetName() != signature.Name)
            continue;
        if (method->GetParametersCount() != signature.Params.Count())
            continue;
        bool isValid = true;
        for (int32 paramIdx = 0; paramIdx < signature.Params.Count(); paramIdx++)
        {
            auto& param = signature.Params[paramIdx];
            MType* type = method->GetParameterType(paramIdx);
            if (param.IsOut != method->GetParameterIsOut(paramIdx) ||
                !VariantTypeEquals(param.Type, type, param.IsOut))
            {
                isValid = false;
                break;
            }
        }
        if (isValid && VariantTypeEquals(signature.ReturnType, method->GetReturnType()))
            return method;
    }
#endif
    return nullptr;
}

#if USE_CSHARP

ManagedBinaryModule* ManagedBinaryModule::FindModule(const MClass* klass)
{
    ManagedBinaryModule* module = nullptr;
    if (klass && klass->GetAssembly())
    {
        auto& modules = BinaryModule::GetModules();
        for (auto e : modules)
        {
            auto managedModule = dynamic_cast<ManagedBinaryModule*>(e);
            if (managedModule && managedModule->Assembly == klass->GetAssembly())
            {
                module = managedModule;
                break;
            }
        }
    }
    return module;
}

ScriptingTypeHandle ManagedBinaryModule::FindType(const MClass* klass)
{
    auto typeModule = FindModule(klass);
    if (typeModule)
    {
        int32 typeIndex;
        if (typeModule->ClassToTypeIndex.TryGet(klass, typeIndex))
            return ScriptingTypeHandle(typeModule, typeIndex);
    }
    return ScriptingTypeHandle();
}

#endif

void ManagedBinaryModule::OnLoading(MAssembly* assembly)
{
    PROFILE_CPU();
    for (ScriptingType& type : Types)
    {
        type.InitRuntime();
    }
}

void ManagedBinaryModule::OnLoaded(MAssembly* assembly)
{
#if !COMPILE_WITHOUT_CSHARP
    PROFILE_CPU();
    ASSERT(ClassToTypeIndex.IsEmpty());
    ScopeLock lock(Locker);

    const auto& classes = assembly->GetClasses();

    // Cache managed types information
    ClassToTypeIndex.EnsureCapacity(Types.Count() * 4);
    for (int32 typeIndex = 0; typeIndex < Types.Count(); typeIndex++)
    {
        ScriptingType& type = Types[typeIndex];
        ASSERT(type.ManagedClass == nullptr);

        // Cache class
        const StringAnsi typeName(type.Fullname.Get(), type.Fullname.Length());
        classes.TryGet(typeName, type.ManagedClass);
        if (type.ManagedClass == nullptr)
        {
            LOG(Error, "Missing class {0} from assembly {1}.", type.ToString(), assembly->ToString());
            continue;
        }

        // Cache klass -> type index lookup
        MClass* klass = type.ManagedClass;
#if !BUILD_RELEASE
        if (ClassToTypeIndex.ContainsKey(klass))
        {
            LOG(Error, "Duplicated native types for class {0} from assembly {1}.", type.ToString(), assembly->ToString());
            continue;
        }
#endif
        ClassToTypeIndex[klass] = typeIndex;
    }

    // Cache types for managed-only types that can be used in the engine
    _firstManagedTypeIndex = Types.Count();
    NativeBinaryModule* flaxEngine = (NativeBinaryModule*)GetBinaryModuleFlaxEngine();
    if (flaxEngine->Assembly->IsLoaded())
    {
        // TODO: check only assemblies that references FlaxEngine.CSharp.dll
        MClass* scriptingObjectType = this == flaxEngine ? classes["FlaxEngine.Object"] : ScriptingObject::GetStaticClass();
        for (auto i = classes.Begin(); i.IsNotEnd(); ++i)
        {
            MClass* mclass = i->Value;

            // Check if C# class inherits from C++ object class it has no C++ representation
            if (mclass->IsStatic() ||
                mclass->IsInterface() ||
                !mclass->IsSubClassOf(scriptingObjectType)
            )
            {
                continue;
            }

            InitType(mclass);
        }
    }

    // Invoke module initializers
    if (flaxEngine->Assembly->IsLoaded() && this != flaxEngine)
    {
        const MClass* attribute = flaxEngine->Assembly->GetClass("FlaxEngine.ModuleInitializerAttribute");
        ASSERT_LOW_LAYER(attribute);
        for (auto i = classes.Begin(); i.IsNotEnd(); ++i)
        {
            MClass* mclass = i->Value;
            if (mclass->IsStatic() && !mclass->IsInterface() && mclass->HasAttribute(attribute))
            {
                const auto& methods = mclass->GetMethods();
                for (const MMethod* method : methods)
                {
                    if (method->GetParametersCount() == 0)
                    {
                        MObject* exception = nullptr;
                        method->Invoke(nullptr, nullptr, &exception);
                        if (exception)
                        {
                            MException ex(exception);
                            String methodName = String(method->GetName());
                            ex.Log(LogType::Error, methodName.Get());
                            LOG(Error, "Failed to call module initializer for class {0} from assembly {1}.", String(mclass->GetFullName()), assembly->ToString());
                        }
                    }
                }
            }
        }
    }
#endif
}

void ManagedBinaryModule::InitType(MClass* mclass)
{
#if !COMPILE_WITHOUT_CSHARP
    // Skip if already initialized
    const StringAnsi& typeName = mclass->GetFullName();
    if (TypeNameToTypeIndex.ContainsKey(typeName))
        return;

    // Find first native base C++ class of this C# class
    MClass* baseClass = mclass->GetBaseClass();
    ScriptingTypeHandle baseType;
    baseType.Module = FindModule(baseClass);
    if (!baseClass)
    {
        LOG(Error, "Missing base class for managed class {0} from assembly {1}.", String(typeName), Assembly->ToString());
        return;
    }
    if (baseType.Module == this)
        InitType(baseClass); // Ensure base is initialized before

    baseType.Module->TypeNameToTypeIndex.TryGet(baseClass->GetFullName(), *(int32*)&baseType.TypeIndex);

    // So we must special case this flow of a generic class of which its possible the generic base class is not in the same module
    if (baseType.TypeIndex == -1 && baseClass->IsGeneric())
    {
        auto genericNameIndex = baseClass->GetFullName().FindLast('`');
        // we add 2 because of the way generic names work its `N
        auto genericClassName = baseClass->GetFullName().Substring(0, genericNameIndex + 2);

        // We check for the generic class name instead of the baseclass fullname
        baseType.Module->TypeNameToTypeIndex.TryGet(genericClassName, *(int32*)&baseType.TypeIndex);
    }

    if (baseType.TypeIndex == -1 || baseType.Module == nullptr)
    {
        if (baseType.Module)
            LOG(Error, "Missing base class for managed class {0} from assembly {1}.", String(baseClass->GetFullName()), baseType.Module->GetName().ToString());
        else
            LOG(Error, "Missing base class for managed class {0} from unknown assembly.", String(baseClass->GetFullName()));
        return;
    }

    ScriptingTypeHandle nativeType = baseType;
    while (true)
    {
        auto& type = nativeType.GetType();
        if (type.Script.Spawn != &ManagedObjectSpawn)
            break;
        nativeType = type.GetBaseType();
        if (!nativeType)
        {
            LOG(Error, "Missing base class for managed class {0} from assembly {1}.", String(typeName), Assembly->ToString());
            return;
        }
    }

    // Scripting Type has Fullname span pointing to the string in memory (usually static data) so store the name in assembly
    char* typeNameData = (char*)Allocator::Allocate(typeName.Length() + 1);
    Platform::MemoryCopy(typeNameData, typeName.Get(), typeName.Length());
    typeNameData[typeName.Length()] = 0;
    _managedMemoryBlocks.Add(typeNameData);

    // Initialize scripting interfaces implemented in C#
    int32 interfacesCount = 0;
    MClass* klass = mclass;
    const Array<MClass*>& interfaceClasses = klass->GetInterfaces();
    for (const MClass* interfaceClass : interfaceClasses)
    {
        const ScriptingTypeHandle interfaceType = FindType(interfaceClass);
        if (interfaceType)
            interfacesCount++;
    }
    ScriptingType::InterfaceImplementation* interfaces = nullptr;
    if (interfacesCount != 0)
    {
        interfaces = (ScriptingType::InterfaceImplementation*)Allocator::Allocate((interfacesCount + 1) * sizeof(ScriptingType::InterfaceImplementation));
        interfacesCount = 0;
        for (const MClass* interfaceClass : interfaceClasses)
        {
            const ScriptingTypeHandle interfaceTypeHandle = FindType(interfaceClass);
            if (!interfaceTypeHandle)
                continue;
            auto& interface = interfaces[interfacesCount++];
            auto ptr = (ScriptingTypeHandle*)Allocator::Allocate(sizeof(ScriptingTypeHandle));
            *ptr = interfaceTypeHandle;
            _managedMemoryBlocks.Add(ptr);
            interface.InterfaceType = ptr;
            interface.VTableOffset = 0;
            interface.ScriptVTableOffset = 0;
            interface.IsNative = false;
        }
        Platform::MemoryClear(interfaces + interfacesCount, sizeof(ScriptingType::InterfaceImplementation));
        _managedMemoryBlocks.Add(interfaces);
    }

    // Create scripting type descriptor for managed-only type based on the native base class
    const int32 typeIndex = Types.Count();
    Types.AddUninitialized();
    new(Types.Get() + Types.Count() - 1)ScriptingType(typeName, this, baseType.GetType().Size, ScriptingType::DefaultInitRuntime, ManagedObjectSpawn, baseType, nullptr, nullptr, interfaces);
    TypeNameToTypeIndex[typeName] = typeIndex;
    auto& type = Types[typeIndex];
    type.ManagedClass = mclass;

    // Register C# class
    ASSERT(!ClassToTypeIndex.ContainsKey(klass));
    ClassToTypeIndex[klass] = typeIndex;

    // Create managed vtable for this class (build out of the wrapper C++ methods that call C# methods)
    type.SetupScriptVTable(nativeType);
    MMethod** scriptVTable = (MMethod**)type.Script.ScriptVTable;
    while (scriptVTable && *scriptVTable)
    {
        const MMethod* referenceMethod = *scriptVTable;

        // Find that method overriden in C# class (the current or one of the base classes in C#)
        MMethod* method = ::FindMethod(mclass, referenceMethod);
        if (method == nullptr)
        {
            // Check base classes (skip native class)
            baseClass = mclass->GetBaseClass();
            MClass* nativeBaseClass = nativeType.GetType().ManagedClass;
            while (baseClass && baseClass != nativeBaseClass && method == nullptr)
            {
                method = ::FindMethod(baseClass, referenceMethod);

                // Special case if method was found but the base class uses generic arguments
                if (method && baseClass->IsGeneric())
                {
                    MClass* parentClass = mclass->GetBaseClass();
                    MMethod* parentMethod = parentClass->GetMethod(referenceMethod->GetName().Get(), 0);
                    method = parentMethod->InflateGeneric();
                }

                baseClass = baseClass->GetBaseClass();
            }
        }

        // Set the method to call (null entry marks unused entries that won't use C# wrapper calls)
        *scriptVTable = method;

        // Move to the next entry (table is null terminated)
        scriptVTable++;
    }
#endif
}

void ManagedBinaryModule::OnUnloading(MAssembly* assembly)
{
    PROFILE_CPU();

    // Clear managed types typenames
    for (int32 i = _firstManagedTypeIndex; i < Types.Count(); i++)
    {
        const ScriptingType& type = Types[i];
        const StringAnsi typeName(type.Fullname.Get(), type.Fullname.Length());
        TypeNameToTypeIndex.Remove(typeName);
    }
}

void ManagedBinaryModule::OnUnloaded(MAssembly* assembly)
{
    PROFILE_CPU();

    // Clear managed-only types
    Types.Resize(_firstManagedTypeIndex);
    for (int32 i = 0; i < _managedMemoryBlocks.Count(); i++)
        Allocator::Free(_managedMemoryBlocks[i]);
    _managedMemoryBlocks.Clear();

    // Clear managed types information
    for (ScriptingType& type : Types)
    {
        type.ManagedClass = nullptr;
        if (type.Type == ScriptingTypes::Script && type.Script.ScriptVTable)
        {
            Platform::Free(type.Script.ScriptVTable);
            type.Script.ScriptVTable = nullptr;
        }
    }
#if !COMPILE_WITHOUT_CSHARP
    ClassToTypeIndex.Clear();
#endif
}

const StringAnsi& ManagedBinaryModule::GetName() const
{
    return Assembly->GetName();
}

bool ManagedBinaryModule::IsLoaded() const
{
#if COMPILE_WITHOUT_CSHARP
    return true;
#else
    return Assembly->IsLoaded();
#endif
}

void* ManagedBinaryModule::FindMethod(const ScriptingTypeHandle& typeHandle, const StringAnsiView& name, int32 numParams)
{
    const ScriptingType& type = typeHandle.GetType();
    return type.ManagedClass ? type.ManagedClass->GetMethod(name.Get(), numParams) : nullptr;
}

void* ManagedBinaryModule::FindMethod(const ScriptingTypeHandle& typeHandle, const ScriptingTypeMethodSignature& signature)
{
    const ScriptingType& type = typeHandle.GetType();
    return FindMethod(type.ManagedClass, signature);
}

bool ManagedBinaryModule::InvokeMethod(void* method, const Variant& instance, Span<Variant> paramValues, Variant& result)
{
#if USE_CSHARP
    const auto mMethod = (MMethod*)method;
    const int32 parametersCount = mMethod->GetParametersCount();;
    if (paramValues.Length() != parametersCount)
    {
        LOG(Error, "Failed to call method '{0}.{1}' (args count: {2}) with invalid parameters amount ({3})", String(mMethod->GetParentClass()->GetFullName()), String(mMethod->GetName()), parametersCount, paramValues.Length());
        return true;
    }

    // Get instance object
    void* mInstance = nullptr;
    const bool withInterfaces = !mMethod->IsStatic() && mMethod->GetParentClass()->IsInterface();
    if (!mMethod->IsStatic())
    {
        // Box instance into C# object (and validate the type)
        MObject* instanceObject = MUtils::BoxVariant(instance);
        if (!instanceObject)
        {
            LOG(Error, "Failed to call method '{0}.{1}' (args count: {2}) without object instance", String(mMethod->GetParentClass()->GetFullName()), String(mMethod->GetName()), parametersCount);
            return true;
        }
        const MClass* instanceObjectClass = MCore::Object::GetClass(instanceObject);
        if (!instanceObjectClass->IsSubClassOf(mMethod->GetParentClass(), withInterfaces))
        {
            LOG(Error, "Failed to call method '{0}.{1}' (args count: {2}) with invalid object instance of type '{3}'", String(mMethod->GetParentClass()->GetFullName()), String(mMethod->GetName()), parametersCount, String(MUtils::GetClassFullname(instanceObject)));
            return true;
        }

#if USE_NETCORE
        mInstance = instanceObject;
#else
        // For value-types instance is the actual boxed object data, not te object itself
        mInstance = instanceObjectClass->IsValueType() ? MCore::Object::Unbox(instanceObject) : instanceObject;
#endif
    }

    // Marshal parameters
    void** params = (void**)alloca(parametersCount * sizeof(void*));
    void** outParams = nullptr;
    bool failed = false;
    bool hasOutParams = false;
    for (int32 paramIdx = 0; paramIdx < parametersCount; paramIdx++)
    {
        Variant& paramValue = paramValues[paramIdx];
        const bool isOut = mMethod->GetParameterIsOut(paramIdx);
        hasOutParams |= isOut;

        // Marshal parameter for managed method
        MType* paramType = mMethod->GetParameterType(paramIdx);
        params[paramIdx] = MUtils::VariantToManagedArgPtr(paramValue, paramType, failed);
        if (failed)
        {
            LOG(Error, "Failed to marshal parameter {5}:{4} of method '{0}.{1}' (args count: {2}), value type: {6}, value: {3}", String(mMethod->GetParentClass()->GetFullName()), String(mMethod->GetName()), parametersCount, paramValue, MCore::Type::ToString(paramType), paramIdx, paramValue.Type);
            return true;
        }
        if (isOut && MCore::Type::IsReference(paramType) && MCore::Type::GetType(paramType) == MTypes::Object)
        {
            // Object passed as out param so pass pointer to the value storage for proper marshalling
            if (!outParams)
                outParams = (void**)alloca(parametersCount * sizeof(void*));
            outParams[paramIdx] = params[paramIdx];
            params[paramIdx] = &outParams[paramIdx];
        }
    }

    // Invoke the method
    MObject* exception = nullptr;
#if USE_NETCORE // NetCore uses the same path for both virtual and non-virtual calls
    MObject* resultObject = mMethod->Invoke(mInstance, params, &exception);
#else
    MObject* resultObject = withInterfaces ? mMethod->InvokeVirtual((MObject*)mInstance, params, &exception) : mMethod->Invoke(mInstance, params, &exception);
#endif
    if (exception)
    {
        MException ex(exception);
        ex.Log(LogType::Error, TEXT("InvokeMethod"));
        return true;
    }

    // Unbox result
    result = MUtils::UnboxVariant(resultObject);

#if 0
    // Helper method invocations logging
    String log;
    log += result.ToString();
    log += TEXT(" ");
    log += String(mMethod->GetName());
    log += TEXT("(");
    for (int32 paramIdx = 0; paramIdx < parametersCount; paramIdx++)
    {
        if (paramIdx != 0)
            log += TEXT(", ");
        log += paramValues[paramIdx].ToString();
    }
    log += TEXT(")");
    LOG_STR(Warning, log);
#endif

    // Unbox output parameters values
    if (hasOutParams)
    {
        for (int32 paramIdx = 0; paramIdx < parametersCount; paramIdx++)
        {
            if (mMethod->GetParameterIsOut(paramIdx))
            {
                auto& paramValue = paramValues[paramIdx];
                auto param = params[paramIdx];
                switch (paramValue.Type.Type)
                {
                case VariantType::String:
                    paramValue.SetString(MUtils::ToString((MString*)param));
                    break;
                case VariantType::Object:
                    paramValue = MUtils::UnboxVariant((MObject*)param);
                    break;
                case VariantType::Structure:
                {
                    const ScriptingTypeHandle paramTypeHandle = Scripting::FindScriptingType(StringAnsiView(paramValue.Type.TypeName));
                    if (paramTypeHandle)
                    {
                        auto& valueType = paramTypeHandle.GetType();
                        MObject* boxed = MCore::Object::Box(param, valueType.ManagedClass);
                        valueType.Struct.Unbox(paramValue.AsBlob.Data, boxed);
                    }
                    break;
                }
                default:
                {
                    MType* paramType = mMethod->GetParameterType(paramIdx);
                    if (MCore::Type::IsReference(paramType) && MCore::Type::GetType(paramType) == MTypes::Object)
                        paramValue = MUtils::UnboxVariant((MObject*)outParams[paramIdx]);
                    break;
                }
                }
            }
        }
    }

    return false;
#else
    return true;
#endif
}

void ManagedBinaryModule::GetMethodSignature(void* method, ScriptingTypeMethodSignature& signature)
{
#if USE_CSHARP
    const auto mMethod = (MMethod*)method;
    signature.Name = mMethod->GetName();
    signature.IsStatic = mMethod->IsStatic();
    signature.ReturnType = MoveTemp(MUtils::UnboxVariantType(mMethod->GetReturnType()));
    const int32 paramsCount = mMethod->GetParametersCount();
    signature.Params.Resize(paramsCount);
    for (int32 paramIdx = 0; paramIdx < paramsCount; paramIdx++)
    {
        auto& param = signature.Params[paramIdx];
        param.Type = MoveTemp(MUtils::UnboxVariantType(mMethod->GetParameterType(paramIdx)));
        param.IsOut = mMethod->GetParameterIsOut(paramIdx);
    }
#endif
}

// Pointers with the highest bit turned on are properties
#if PLATFORM_64BITS
#define ManagedBinaryModuleFieldIsPropertyBit (uintptr)(1ull << 63)
#else
#define ManagedBinaryModuleFieldIsPropertyBit (uintptr)(1ul << 31)
#endif

void* ManagedBinaryModule::FindField(const ScriptingTypeHandle& typeHandle, const StringAnsiView& name)
{
    const ScriptingType& type = typeHandle.GetType();
    void* result = type.ManagedClass ? type.ManagedClass->GetField(name.Get()) : nullptr;
    if (!result && type.ManagedClass)
    {
        result = type.ManagedClass->GetProperty(name.Get());
        if (result)
            result = (void*)((uintptr)result | ManagedBinaryModuleFieldIsPropertyBit);
    }
    return result;
}

void ManagedBinaryModule::GetFieldSignature(void* field, ScriptingTypeFieldSignature& fieldSignature)
{
#if USE_CSHARP
    if ((uintptr)field & ManagedBinaryModuleFieldIsPropertyBit)
    {
        const auto mProperty = (MProperty*)((uintptr)field & ~ManagedBinaryModuleFieldIsPropertyBit);
        fieldSignature.Name = mProperty->GetName();
        fieldSignature.ValueType = MoveTemp(MUtils::UnboxVariantType(mProperty->GetType()));
        fieldSignature.IsStatic = mProperty->IsStatic();
    }
    else
    {
        const auto mField = (MField*)field;
        fieldSignature.Name = mField->GetName();
        fieldSignature.ValueType = MoveTemp(MUtils::UnboxVariantType(mField->GetType()));
        fieldSignature.IsStatic = mField->IsStatic();
    }
#endif
}

bool ManagedBinaryModule::GetFieldValue(void* field, const Variant& instance, Variant& result)
{
#if USE_CSHARP
    bool isStatic;
    MClass* parentClass;
    StringAnsiView name;
    if ((uintptr)field & ManagedBinaryModuleFieldIsPropertyBit)
    {
        const auto mProperty = (MProperty*)((uintptr)field & ~ManagedBinaryModuleFieldIsPropertyBit);
        isStatic = mProperty->IsStatic();
        parentClass = mProperty->GetParentClass();
        name = mProperty->GetName();
    }
    else
    {
        const auto mField = (MField*)field;
        isStatic = mField->IsStatic();
        parentClass = mField->GetParentClass();
        name = mField->GetName();
    }

    // Get instance object
    MObject* instanceObject = nullptr;
    if (!isStatic)
    {
        // Box instance into C# object
        instanceObject = MUtils::BoxVariant(instance);

        // Validate instance
        if (!instanceObject || !MCore::Object::GetClass(instanceObject)->IsSubClassOf(parentClass))
        {
            if (!instanceObject)
                LOG(Error, "Failed to get '{0}.{1}' without object instance", String(parentClass->GetFullName()), String(name));
            else
                LOG(Error, "Failed to get '{0}.{1}' with invalid object instance of type '{2}'", String(parentClass->GetFullName()), String(name), String(MUtils::GetClassFullname(instanceObject)));
            return true;
        }
    }

    // Get the value
    MObject* resultObject;
    if ((uintptr)field & ManagedBinaryModuleFieldIsPropertyBit)
    {
        const auto mProperty = (MProperty*)((uintptr)field & ~ManagedBinaryModuleFieldIsPropertyBit);
        resultObject = mProperty->GetValue(instanceObject, nullptr);
    }
    else
    {
        const auto mField = (MField*)field;
        resultObject = mField->GetValueBoxed(instanceObject);
    }
    result = MUtils::UnboxVariant(resultObject);
    return false;
#else
    return true;
#endif
}

bool ManagedBinaryModule::SetFieldValue(void* field, const Variant& instance, Variant& value)
{
#if USE_CSHARP
    bool isStatic;
    MClass* parentClass;
    StringAnsiView name;
    if ((uintptr)field & ManagedBinaryModuleFieldIsPropertyBit)
    {
        const auto mProperty = (MProperty*)((uintptr)field & ~ManagedBinaryModuleFieldIsPropertyBit);
        isStatic = mProperty->IsStatic();
        parentClass = mProperty->GetParentClass();
        name = mProperty->GetName();
    }
    else
    {
        const auto mField = (MField*)field;
        isStatic = mField->IsStatic();
        parentClass = mField->GetParentClass();
        name = mField->GetName();
    }

    // Get instance object
    MObject* instanceObject = nullptr;
    if (!isStatic)
    {
        // Box instance into C# object
        instanceObject = MUtils::BoxVariant(instance);

        // Validate instance
        if (!instanceObject || !MCore::Object::GetClass(instanceObject)->IsSubClassOf(parentClass))
        {
            if (!instanceObject)
                LOG(Error, "Failed to set '{0}.{1}' without object instance", String(parentClass->GetFullName()), String(name));
            else
                LOG(Error, "Failed to set '{0}.{1}' with invalid object instance of type '{2}'", String(parentClass->GetFullName()), String(name), String(MUtils::GetClassFullname(instanceObject)));
            return true;
        }
    }

    // Set the value
    bool failed = false;
    if ((uintptr)field & ManagedBinaryModuleFieldIsPropertyBit)
    {
        const auto mProperty = (MProperty*)((uintptr)field & ~ManagedBinaryModuleFieldIsPropertyBit);
        mProperty->SetValue(instanceObject, MUtils::VariantToManagedArgPtr(value, mProperty->GetType(), failed), nullptr);
    }
    else
    {
        const auto mField = (MField*)field;
        mField->SetValue(instanceObject, MUtils::VariantToManagedArgPtr(value, mField->GetType(), failed));
    }
    return failed;
#else
    return true;
#endif
}

void ManagedBinaryModule::Destroy(bool isReloading)
{
    BinaryModule::Destroy(isReloading);

    // Release managed assembly
    Assembly->Unload(isReloading);
}

NativeBinaryModule::NativeBinaryModule(const StringAnsiView& name)
    : NativeBinaryModule(New<MAssembly>(nullptr, name))
{
}

NativeBinaryModule::NativeBinaryModule(MAssembly* assembly)
    : ManagedBinaryModule(assembly)
    , Library(nullptr)
{
}

void NativeBinaryModule::Destroy(bool isReloading)
{
    ManagedBinaryModule::Destroy(isReloading);

    // Skip native code unloading from core libs
    if (this == GetBinaryModuleCorlib() || this == GetBinaryModuleFlaxEngine())
        return;

    // Release native library
    const auto library = Library;
    if (library)
    {
        Library = nullptr;
        Platform::FreeLibrary(library);
        // Don't do anything after FreeLibrary (this pointer might be invalid)
    }
}

NativeOnlyBinaryModule::NativeOnlyBinaryModule(const StringAnsiView& name)
    : BinaryModule()
    , _name(name)
    , Library(nullptr)
{
}

const StringAnsi& NativeOnlyBinaryModule::GetName() const
{
    return _name;
}

bool NativeOnlyBinaryModule::IsLoaded() const
{
    return true;
}

void NativeOnlyBinaryModule::Destroy(bool isReloading)
{
    BinaryModule::Destroy(isReloading);

    // Release native library
    const auto library = Library;
    if (library)
    {
        Library = nullptr;
        Platform::FreeLibrary(library);
        // Don't do anything after FreeLibrary (this pointer might be invalid)
    }
}

Array<GetBinaryModuleFunc, InlinedAllocation<64>>& StaticallyLinkedBinaryModuleInitializer::GetStaticallyLinkedBinaryModules()
{
    static Array<GetBinaryModuleFunc, InlinedAllocation<64>> modules;
    return modules;
}

StaticallyLinkedBinaryModuleInitializer::StaticallyLinkedBinaryModuleInitializer(GetBinaryModuleFunc getter)
    : _getter(getter)
{
    GetStaticallyLinkedBinaryModules().Add(getter);
}

StaticallyLinkedBinaryModuleInitializer::~StaticallyLinkedBinaryModuleInitializer()
{
    GetStaticallyLinkedBinaryModules().Remove(_getter);
}
