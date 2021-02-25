// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "BinaryModule.h"
#include "ScriptingObject.h"
#include "Engine/Core/Log.h"
#include "Engine/Threading/Threading.h"
#include "ManagedCLR/MAssembly.h"
#include "ManagedCLR/MClass.h"
#include "ManagedCLR/MType.h"
#include "ManagedCLR/MMethod.h"
#include "ManagedCLR/MField.h"
#include "ManagedCLR/MUtils.h"
#include "FlaxEngine.Gen.h"
#include "MException.h"
#include "Scripting.h"
#include "StdTypesContainer.h"
#include "Events.h"

Dictionary<Pair<ScriptingTypeHandle, StringView>, void(*)(ScriptingObject*, void*, bool)> ScriptingEvents::EventsTable;
Delegate<ScriptingObject*, Span<Variant>, ScriptingTypeHandle, StringView> ScriptingEvents::Event;

ManagedBinaryModule* GetBinaryModuleCorlib()
{
    static ManagedBinaryModule assembly("corlib", MAssemblyOptions(false)); // Don't precache all corlib classes
    return &assembly;
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

ScriptingType::ScriptingType(const StringAnsiView& fullname, BinaryModule* module, InitRuntimeHandler initRuntime, ScriptingTypeInitializer* baseType, const InterfaceImplementation* interfaces)
    : ManagedClass(nullptr)
    , Module(module)
    , InitRuntime(initRuntime)
    , Fullname(fullname)
    , Type(ScriptingTypes::Interface)
    , BaseTypePtr(baseType)
    , Interfaces(interfaces)
    , Size(0)
{
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
        break;
    case ScriptingTypes::Interface:
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
        break;
    case ScriptingTypes::Interface:
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

const ScriptingType::InterfaceImplementation* ScriptingType::GetInterface(const ScriptingTypeInitializer* interfaceType) const
{
    const InterfaceImplementation* interfaces = Interfaces;
    if (interfaces)
    {
        while (interfaces->InterfaceType)
        {
            if (interfaces->InterfaceType == interfaceType)
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

String ScriptingType::ToString() const
{
    return String(Fullname.Get(), Fullname.Length());
}

ScriptingTypeInitializer::ScriptingTypeInitializer(BinaryModule* module, const StringAnsiView& fullname, int32 size, ScriptingType::InitRuntimeHandler initRuntime, ScriptingType::SpawnHandler spawn, ScriptingTypeInitializer* baseType, ScriptingType::SetupScriptVTableHandler setupScriptVTable, ScriptingType::SetupScriptObjectVTableHandler setupScriptObjectVTable, const ScriptingType::InterfaceImplementation* interfaces)
    : ScriptingTypeHandle(module, module->Types.Count())
{
    // Script
    module->Types.AddUninitialized();
    new(module->Types.Get() + TypeIndex)ScriptingType(fullname, module, size, initRuntime, spawn, baseType, setupScriptVTable, setupScriptObjectVTable, interfaces);
    const MString typeName(fullname.Get(), fullname.Length());
#if BUILD_DEBUG
    if (module->TypeNameToTypeIndex.ContainsKey(typeName))
    {
        LOG(Error, "Duplicated native typename {0} from module {1}.", String(fullname), String(module->GetName()));
    }
#endif
    module->TypeNameToTypeIndex[typeName] = TypeIndex;
}

ScriptingTypeInitializer::ScriptingTypeInitializer(BinaryModule* module, const StringAnsiView& fullname, int32 size, ScriptingType::InitRuntimeHandler initRuntime, ScriptingType::Ctor ctor, ScriptingType::Dtor dtor, ScriptingTypeInitializer* baseType, const ScriptingType::InterfaceImplementation* interfaces)
    : ScriptingTypeHandle(module, module->Types.Count())
{
    // Class
    module->Types.AddUninitialized();
    new(module->Types.Get() + TypeIndex)ScriptingType(fullname, module, size, initRuntime, ctor, dtor, baseType, interfaces);
    const MString typeName(fullname.Get(), fullname.Length());
#if BUILD_DEBUG
    if (module->TypeNameToTypeIndex.ContainsKey(typeName))
    {
        LOG(Error, "Duplicated native typename {0} from module {1}.", String(fullname), String(module->GetName()));
    }
#endif
    module->TypeNameToTypeIndex[typeName] = TypeIndex;
}

ScriptingTypeInitializer::ScriptingTypeInitializer(BinaryModule* module, const StringAnsiView& fullname, int32 size, ScriptingType::InitRuntimeHandler initRuntime, ScriptingType::Ctor ctor, ScriptingType::Dtor dtor, ScriptingType::Copy copy, ScriptingType::Box box, ScriptingType::Unbox unbox, ScriptingType::GetField getField, ScriptingType::SetField setField, ScriptingTypeInitializer* baseType, const ScriptingType::InterfaceImplementation* interfaces)
    : ScriptingTypeHandle(module, module->Types.Count())
{
    // Structure
    module->Types.AddUninitialized();
    new(module->Types.Get() + TypeIndex)ScriptingType(fullname, module, size, initRuntime, ctor, dtor, copy, box, unbox, getField, setField, baseType, interfaces);
    const MString typeName(fullname.Get(), fullname.Length());
#if BUILD_DEBUG
    if (module->TypeNameToTypeIndex.ContainsKey(typeName))
    {
        LOG(Error, "Duplicated native typename {0} from module {1}.", String(fullname), String(module->GetName()));
    }
#endif
    module->TypeNameToTypeIndex[typeName] = TypeIndex;
}

ScriptingTypeInitializer::ScriptingTypeInitializer(BinaryModule* module, const StringAnsiView& fullname, ScriptingType::InitRuntimeHandler initRuntime, ScriptingTypeInitializer* baseType, const ScriptingType::InterfaceImplementation* interfaces)
    : ScriptingTypeHandle(module, module->Types.Count())
{
    // Interface
    module->Types.AddUninitialized();
    new(module->Types.Get() + TypeIndex)ScriptingType(fullname, module, initRuntime, baseType, interfaces);
    const MString typeName(fullname.Get(), fullname.Length());
#if BUILD_DEBUG
    if (module->TypeNameToTypeIndex.ContainsKey(typeName))
    {
        LOG(Error, "Duplicated native typename {0} from module {1}.", String(fullname), String(module->GetName()));
    }
#endif
    module->TypeNameToTypeIndex[typeName] = TypeIndex;
}

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

void BinaryModule::Destroy(bool isReloading)
{
    // Unregister
    GetModules().RemoveKeepOrder(this);
}

ManagedBinaryModule::ManagedBinaryModule(const StringAnsiView& name, const MAssemblyOptions& options)
    : ManagedBinaryModule(New<MAssembly>(nullptr, name, options))
{
}

ManagedBinaryModule::ManagedBinaryModule(MAssembly* assembly)
{
    Assembly = assembly;

    // Bind for C# assembly events
    assembly->Loading.Bind<ManagedBinaryModule, &ManagedBinaryModule::OnLoading>(this);
    assembly->Loaded.Bind<ManagedBinaryModule, &ManagedBinaryModule::OnLoaded>(this);
    assembly->Unloading.Bind<ManagedBinaryModule, &ManagedBinaryModule::OnUnloading>(this);

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
        return nullptr;
    }

    // Beware! Hacking vtables incoming! Undefined behaviors exploits! Low-level programming!
    // What's happening here?
    // We create a custom vtable for the C# objects that use a native class object with virtual functions overrides.
    // To make it easy to use in C++ we inject custom wrapper methods into C++ object vtable to call C# code from them.
    // Because virtual member functions calls are C++ ABI and impl-defined this is quite hard. But works.
    if (managedType.Script.ScriptVTable)
    {
        if (!managedType.Script.VTable)
        {
            // Duplicate vtable
            void** vtable = *(void***)object;
            const int32 prefixSize = GetVTablePrefix();
            int32 entriesCount = 0;
            while (vtable[entriesCount] && entriesCount < 200)
                entriesCount++;
            const int32 size = entriesCount * sizeof(void*);
            managedType.Script.VTable = (void**)((byte*)Platform::Allocate(prefixSize + size, 16) + prefixSize);
            Platform::MemoryCopy((byte*)managedType.Script.VTable - prefixSize, (byte*)vtable - prefixSize, prefixSize + size);

            // Override vtable entries by the class
            for (ScriptingTypeHandle e = nativeTypeHandle; e;)
            {
                const ScriptingType& eType = e.GetType();
                if (eType.Script.SetupScriptObjectVTable)
                {
                    eType.Script.SetupScriptObjectVTable(managedType.Script.ScriptVTable, managedType.Script.ScriptVTableBase, managedType.Script.VTable, entriesCount, 0);
                }
                e = eType.GetBaseType();
            }
        }

        // Override object vtable with hacked one that has C# functions calls
        *(void**)object = managedType.Script.VTable;
    }

    // Mark as managed type
    object->Flags |= ObjectFlags::IsManagedType;

    return object;
}

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
}

MMethod* ManagedBinaryModule::FindMethod(MClass* mclass, const ScriptingTypeMethodSignature& signature)
{
    if (!mclass)
        return nullptr;
    const auto& methods = mclass->GetMethods();
    for (MMethod* method : methods)
    {
        MonoMethodSignature* sig = mono_method_signature(method->GetNative());
        if (method->IsStatic() != signature.IsStatic ||
            method->GetName() != signature.Name ||
            (int32)mono_signature_get_param_count(sig) != signature.Params.Count())
            continue;
        void* sigParams = nullptr;
        mono_signature_get_params(sig, &sigParams);
        bool isValid = true;
        for (int32 paramIdx = 0; paramIdx < signature.Params.Count(); paramIdx++)
        {
            auto& param = signature.Params[paramIdx];
            if (param.IsOut != (mono_signature_param_is_out(sig, paramIdx) != 0) ||
                MUtils::GetClass(param.Type) != mono_class_from_mono_type(((MonoType**)sigParams)[paramIdx]))
            {
                isValid = false;
                break;
            }
        }
        if (isValid && MUtils::GetClass(signature.ReturnType) == mono_class_from_mono_type(mono_signature_get_return_type(sig)))
            return method;
    }
    return nullptr;
}

void ManagedBinaryModule::OnLoading(MAssembly* assembly)
{
    for (ScriptingType& type : Types)
    {
        type.InitRuntime();
    }
}

void ManagedBinaryModule::OnLoaded(MAssembly* assembly)
{
    ASSERT(ClassToTypeIndex.IsEmpty());

    const auto& classes = assembly->GetClasses();

    // Cache managed types information
    ClassToTypeIndex.EnsureCapacity(Types.Count() * 4);
    for (int32 typeIndex = 0; typeIndex < Types.Count(); typeIndex++)
    {
        ScriptingType& type = Types[typeIndex];
        if (type.Type == ScriptingTypes::Interface)
            continue; // TODO: generate C# class for interfaces in API
        ASSERT(type.ManagedClass == nullptr);

        // Cache class
        const MString typeName(type.Fullname.Get(), type.Fullname.Length());
        classes.TryGet(typeName, type.ManagedClass);
        if (type.ManagedClass == nullptr)
        {
            LOG(Error, "Missing class {0} from assembly {1}.", type.ToString(), assembly->ToString());
            continue;
        }

        // Cache klass -> type index lookup
        MonoClass* klass = type.ManagedClass->GetNative();
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
            const MString& typeName = mclass->GetFullName();

            // Check if C# class inherits from C++ object class it has no C++ representation
            if (
                TypeNameToTypeIndex.Find(typeName) != TypeNameToTypeIndex.End() ||
                mclass->IsStatic() ||
                mclass->IsAbstract() ||
                mclass->IsInterface() ||
                !mclass->IsSubClassOf(scriptingObjectType)
            )
            {
                continue;
            }

            // Find first native base C++ class of this C# class
            MClass* baseClass = mclass->GetBaseClass();
            ScriptingTypeHandle nativeType;
            while (baseClass)
            {
                int32 typeIndex;
                BinaryModule* baseClassModule = GetModule(baseClass->GetAssembly());
                ASSERT(baseClassModule);
                if (baseClassModule->TypeNameToTypeIndex.TryGet(baseClass->GetFullName(), typeIndex))
                {
                    nativeType = ScriptingTypeHandle(baseClassModule, typeIndex);
                    if (nativeType.GetType().Script.Spawn != &ManagedObjectSpawn)
                        break;
                }
                baseClass = baseClass->GetBaseClass();
            }
            if (!nativeType)
            {
                LOG(Error, "Missing native base class for managed class {0} from assembly {1}.", String(typeName), assembly->ToString());
                continue;
            }

            // Scripting Type has Fullname span pointing to the string in memory (usually static data) so store the name in assembly
            char* typeNameData = (char*)Allocator::Allocate(typeName.Length() + 1);
            Platform::MemoryCopy(typeNameData, typeName.Get(), typeName.Length());
            typeNameData[typeName.Length()] = 0;
            _managedTypesNames.Add(typeNameData);

            // Create scripting type descriptor for managed-only type based on the native base class
            const int32 typeIndex = Types.Count();
            Types.AddUninitialized();
            new(Types.Get() + Types.Count() - 1)ScriptingType(typeName, this, nativeType.GetType().Size, ScriptingType::DefaultInitRuntime, ManagedObjectSpawn, nativeType);
            TypeNameToTypeIndex[typeName] = typeIndex;
            auto& type = Types[typeIndex];
            type.ManagedClass = mclass;

            // Register Mono class
            MonoClass* klass = mclass->GetNative();
            ASSERT(!ClassToTypeIndex.ContainsKey(klass));
            ClassToTypeIndex[klass] = typeIndex;

            // Create managed vtable for this class (build out of the wrapper C++ methods that call C# methods)
            // Call setup for all class starting from the first native type (first that uses virtual calls will allocate table of a proper size, further base types will just add own methods)
            for (ScriptingTypeHandle e = nativeType; e;)
            {
                const ScriptingType& eType = e.GetType();
                if (eType.Script.SetupScriptVTable)
                {
                    ASSERT(eType.ManagedClass);
                    eType.Script.SetupScriptVTable(eType.ManagedClass, type.Script.ScriptVTable, type.Script.ScriptVTableBase);
                }
                e = eType.GetBaseType();
            }
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
                            // TODO: encapsulate it into MClass to support inflated methods
                            auto parentClass = mono_class_get_parent(mclass->GetNative());
                            auto parentMethod = mono_class_get_method_from_name(parentClass, referenceMethod->GetName().Get(), 0);
                            auto inflatedMethod = mono_class_inflate_generic_method(parentMethod, nullptr);
                            method = New<MMethod>(inflatedMethod, baseClass);
                        }

                        baseClass = baseClass->GetBaseClass();
                    }
                }

                // Set the method to call (null entry marks unused entries that won't use C# wrapper calls)
                *scriptVTable = method;

                // Move to the next entry (table is null terminated)
                scriptVTable++;
            }
        }
    }
}

void ManagedBinaryModule::OnUnloading(MAssembly* assembly)
{
    // Clear managed-only types
    for (int32 i = _firstManagedTypeIndex; i < Types.Count(); i++)
    {
        const ScriptingType& type = Types[i];
        const MString typeName(type.Fullname.Get(), type.Fullname.Length());
        TypeNameToTypeIndex.Remove(typeName);
    }
    Types.Resize(_firstManagedTypeIndex);
    for (int32 i = 0; i < _managedTypesNames.Count(); i++)
        Allocator::Free(_managedTypesNames[i]);
    _managedTypesNames.Clear();

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
    ClassToTypeIndex.Clear();
}

const StringAnsi& ManagedBinaryModule::GetName() const
{
    return Assembly->GetName();
}

bool ManagedBinaryModule::IsLoaded() const
{
    return Assembly->IsLoaded();
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
    const auto mMethod = (MMethod*)method;
    MonoMethodSignature* signature = mono_method_signature(mMethod->GetNative());
    void* signatureParams = nullptr;
    mono_signature_get_params(signature, &signatureParams);
    const int32 parametersCount = mono_signature_get_param_count(signature);
    if (paramValues.Length() != parametersCount)
    {
        LOG(Error, "Failed to call method '{0}.{1}' (args count: {2}) with invalid parameters amount ({3})", String(mMethod->GetParentClass()->GetFullName()), String(mMethod->GetName()), parametersCount, paramValues.Length());
        return true;
    }

    // Get instance object
    void* mInstance = nullptr;
    if (!mMethod->IsStatic())
    {
        // Box instance into C# object
        MonoObject* instanceObject = MUtils::BoxVariant(instance);

        // Validate instance
        if (!instanceObject || !mono_class_is_subclass_of(mono_object_get_class(instanceObject), mMethod->GetParentClass()->GetNative(), false))
        {
            if (!instanceObject)
                LOG(Error, "Failed to call method '{0}.{1}' (args count: {2}) without object instance", String(mMethod->GetParentClass()->GetFullName()), String(mMethod->GetName()), parametersCount);
            else
                LOG(Error, "Failed to call method '{0}.{1}' (args count: {2}) with invalid object instance of type '{3}'", String(mMethod->GetParentClass()->GetFullName()), String(mMethod->GetName()), parametersCount, String(MUtils::GetClassFullname(instanceObject)));
            return true;
        }

        // For value-types instance is the actual boxed object data, not te object itself
        mInstance = mono_class_is_valuetype(mono_object_get_class(instanceObject)) ? mono_object_unbox(instanceObject) : instanceObject;
    }

    // Marshal parameters
    void** params = (void**)alloca(parametersCount * sizeof(void*));
    bool failed = false;
    bool hasOutParams = false;
    for (int32 paramIdx = 0; paramIdx < parametersCount; paramIdx++)
    {
        auto& paramValue = paramValues[paramIdx];
        const bool isOut = mono_signature_param_is_out(signature, paramIdx) != 0;
        hasOutParams |= isOut;

        // Marshal parameter for managed method
        MType paramType(((MonoType**)signatureParams)[paramIdx]);
        params[paramIdx] = MUtils::VariantToManagedArgPtr(paramValue, paramType, failed);
        if (failed)
        {
            LOG(Error, "Failed to marshal parameter {5}:{4} of method '{0}.{1}' (args count: {2}), value type: {6}, value: {3}", String(mMethod->GetParentClass()->GetFullName()), String(mMethod->GetName()), parametersCount, paramValue, paramType.ToString(), paramIdx, paramValue.Type);
            return true;
        }
    }

    // Invoke the method
    MonoObject* exception = nullptr;
    MonoObject* resultObject = mMethod->Invoke(mInstance, params, &exception);
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
            const bool isOut = mono_signature_param_is_out(signature, paramIdx) != 0;
            if (isOut)
            {
                auto& paramValue = paramValues[paramIdx];
                auto param = params[paramIdx];
                switch (paramValue.Type.Type)
                {
                case VariantType::String:
                    paramValue.SetString(MUtils::ToString((MonoString*)param));
                    break;
                case VariantType::Object:
                    paramValue = MUtils::UnboxVariant((MonoObject*)param);
                    break;
                case VariantType::Structure:
                {
                    const ScriptingTypeHandle paramTypeHandle = Scripting::FindScriptingType(StringAnsiView(paramValue.Type.TypeName));
                    if (paramTypeHandle)
                    {
                        auto& valueType = paramTypeHandle.GetType();
                        valueType.Struct.Unbox(paramValue.AsBlob.Data, (MonoObject*)((byte*)param - sizeof(MonoObject)));
                    }
                    break;
                }
                }
            }
        }
    }

    return false;
}

void ManagedBinaryModule::GetMethodSignature(void* method, ScriptingTypeMethodSignature& signature)
{
    const auto mMethod = (MMethod*)method;
    signature.Name = mMethod->GetName();
    signature.IsStatic = mMethod->IsStatic();
    MonoMethodSignature* sig = mono_method_signature(mMethod->GetNative());
    signature.ReturnType = MoveTemp(MUtils::UnboxVariantType(mono_signature_get_return_type(sig)));
    void* signatureParams = nullptr;
    mono_signature_get_params(sig, &signatureParams);
    const int32 paramsCount = (int32)mono_signature_get_param_count(sig);
    signature.Params.Resize(paramsCount);
    for (int32 paramIdx = 0; paramIdx < paramsCount; paramIdx++)
    {
        auto& param = signature.Params[paramIdx];
        param.Type = MoveTemp(MUtils::UnboxVariantType(((MonoType**)signatureParams)[paramIdx]));
        param.IsOut = mono_signature_param_is_out(sig, paramIdx) != 0;
    }
}

void* ManagedBinaryModule::FindField(const ScriptingTypeHandle& typeHandle, const StringAnsiView& name)
{
    const ScriptingType& type = typeHandle.GetType();
    return type.ManagedClass ? type.ManagedClass->GetField(name.Get()) : nullptr;
}

void ManagedBinaryModule::GetFieldSignature(void* field, ScriptingTypeFieldSignature& fieldSignature)
{
    const auto mField = (MField*)field;
    fieldSignature.Name = mField->GetName();
    fieldSignature.ValueType = MoveTemp(MUtils::UnboxVariantType(mField->GetType().GetNative()));
    fieldSignature.IsStatic = mField->IsStatic();
}

bool ManagedBinaryModule::GetFieldValue(void* field, const Variant& instance, Variant& result)
{
    const auto mField = (MField*)field;

    // Get instance object
    MonoObject* instanceObject = nullptr;
    if (!mField->IsStatic())
    {
        // Box instance into C# object
        instanceObject = MUtils::BoxVariant(instance);

        // Validate instance
        if (!instanceObject || !mono_class_is_subclass_of(mono_object_get_class(instanceObject), mField->GetParentClass()->GetNative(), false))
        {
            if (!instanceObject)
                LOG(Error, "Failed to get field '{0}.{1}' without object instance", String(mField->GetParentClass()->GetFullName()), String(mField->GetName()));
            else
                LOG(Error, "Failed to get field '{0}.{1}' with invalid object instance of type '{2}'", String(mField->GetParentClass()->GetFullName()), String(mField->GetName()), String(MUtils::GetClassFullname(instanceObject)));
            return true;
        }
    }

    // Get the value
    MonoObject* resultObject = mField->GetValueBoxed(instanceObject);
    result = MUtils::UnboxVariant(resultObject);
    return false;
}

bool ManagedBinaryModule::SetFieldValue(void* field, const Variant& instance, Variant& value)
{
    const auto mField = (MField*)field;

    // Get instance object
    MonoObject* instanceObject = nullptr;
    if (!mField->IsStatic())
    {
        // Box instance into C# object
        instanceObject = MUtils::BoxVariant(instance);

        // Validate instance
        if (!instanceObject || !mono_class_is_subclass_of(mono_object_get_class(instanceObject), mField->GetParentClass()->GetNative(), false))
        {
            if (!instanceObject)
                LOG(Error, "Failed to set field '{0}.{1}' without object instance", String(mField->GetParentClass()->GetFullName()), String(mField->GetName()));
            else
                LOG(Error, "Failed to set field '{0}.{1}' with invalid object instance of type '{2}'", String(mField->GetParentClass()->GetFullName()), String(mField->GetName()), String(MUtils::GetClassFullname(instanceObject)));
            return true;
        }
    }

    // Set the value
    bool failed = false;
    mField->SetValue(instanceObject, MUtils::VariantToManagedArgPtr(value, mField->GetType(), failed));
    return failed;
}

void ManagedBinaryModule::Destroy(bool isReloading)
{
    BinaryModule::Destroy(isReloading);

    // Release managed assembly
    Assembly->Unload(isReloading);
}

NativeBinaryModule::NativeBinaryModule(const StringAnsiView& name, const MAssemblyOptions& options)
    : NativeBinaryModule(New<MAssembly>(nullptr, name, options))
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

Array<GetBinaryModuleFunc>& StaticallyLinkedBinaryModuleInitializer::GetStaticallyLinkedBinaryModules()
{
    static Array<GetBinaryModuleFunc> modules;
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
