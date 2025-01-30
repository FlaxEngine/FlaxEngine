// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "Engine/Scripting/Types.h"

#if USE_NETCORE

#pragma warning(default : 4297)

#include "Engine/Core/Log.h"
#include "Engine/Core/Types/DateTime.h"
#include "Engine/Core/Types/TimeSpan.h"
#include "Engine/Core/Types/Stopwatch.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Platform/Platform.h"
#include "Engine/Platform/File.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Scripting/Internal/InternalCalls.h"
#include "Engine/Scripting/ManagedCLR/MCore.h"
#include "Engine/Scripting/ManagedCLR/MAssembly.h"
#include "Engine/Scripting/ManagedCLR/MClass.h"
#include "Engine/Scripting/ManagedCLR/MDomain.h"
#include "Engine/Scripting/ManagedCLR/MEvent.h"
#include "Engine/Scripting/ManagedCLR/MField.h"
#include "Engine/Scripting/ManagedCLR/MMethod.h"
#include "Engine/Scripting/ManagedCLR/MProperty.h"
#include "Engine/Scripting/ManagedCLR/MException.h"
#include "Engine/Scripting/ManagedCLR/MUtils.h"
#include "Engine/Scripting/Scripting.h"
#include "Engine/Scripting/BinaryModule.h"
#include "Engine/Engine/Globals.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Threading/Threading.h"
#include "Engine/Debug/Exceptions/CLRInnerException.h"
#if DOTNET_HOST_CORECLR
#include <nethost.h>
#include <coreclr_delegates.h>
#include <hostfxr.h>
#elif DOTNET_HOST_MONO
#include "Engine/Engine/CommandLine.h"
#include "Engine/Utilities/StringConverter.h"
#include <mono/jit/jit.h>
#include <mono/jit/mono-private-unstable.h>
#include <mono/utils/mono-logger.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/appdomain.h>
#include <mono/metadata/class.h>
#include <mono/metadata/metadata.h>
#include <mono/metadata/threads.h>
#include <mono/metadata/reflection.h>
#include <mono/metadata/mono-gc.h>
#include <mono/metadata/mono-private-unstable.h>
typedef char char_t;
#define DOTNET_HOST_MONO_DEBUG 0
#ifdef USE_MONO_AOT_MODULE
void* MonoAotModuleHandle = nullptr;
#endif
MonoDomain* MonoDomainHandle = nullptr;
#else
#error "Unknown .NET runtime host."
#endif
#if PLATFORM_WINDOWS
#include <combaseapi.h>
#undef SetEnvironmentVariable
#undef GetEnvironmentVariable
#undef LoadLibrary
#undef LoadImage
#endif

#if defined(_WIN32)
#define CORECLR_DELEGATE_CALLTYPE __stdcall
#define FLAX_CORECLR_STRING String
#define FLAX_CORECLR_TEXT(x) TEXT(x)
#else
#define CORECLR_DELEGATE_CALLTYPE
#define FLAX_CORECLR_STRING StringAnsi
#define FLAX_CORECLR_TEXT(x) x
#endif

// System.Reflection.TypeAttributes
enum class MTypeAttributes : uint32
{
    VisibilityMask = 0x00000007,
    NotPublic = 0x00000000,
    Public = 0x00000001,
    NestedPublic = 0x00000002,
    NestedPrivate = 0x00000003,
    NestedFamily = 0x00000004,
    NestedAssembly = 0x00000005,
    NestedFamANDAssem = 0x00000006,
    NestedFamORAssem = 0x00000007,
    LayoutMask = 0x00000018,
    AutoLayout = 0x00000000,
    SequentialLayout = 0x00000008,
    ExplicitLayout = 0x00000010,
    ClassSemanticsMask = 0x00000020,
    Class = 0x00000000,
    Interface = 0x00000020,
    Abstract = 0x00000080,
    Sealed = 0x00000100,
    SpecialName = 0x00000400,
    Import = 0x00001000,
    Serializable = 0x00002000,
    WindowsRuntime = 0x00004000,
    StringFormatMask = 0x00030000,
    AnsiClass = 0x00000000,
    UnicodeClass = 0x00010000,
    AutoClass = 0x00020000,
    CustomFormatClass = 0x00030000,
    CustomFormatMask = 0x00C00000,
    BeforeFieldInit = 0x00100000,
    RTSpecialName = 0x00000800,
    HasSecurity = 0x00040000,
    ReservedMask = 0x00040800,
};

// System.Reflection.MethodAttributes
enum class MMethodAttributes : uint32
{
    MemberAccessMask = 0x0007,
    PrivateScope = 0x0000,
    Private = 0x0001,
    FamANDAssem = 0x0002,
    Assembly = 0x0003,
    Family = 0x0004,
    FamORAssem = 0x0005,
    Public = 0x0006,
    Static = 0x0010,
    Final = 0x0020,
    Virtual = 0x0040,
    HideBySig = 0x0080,
    CheckAccessOnOverride = 0x0200,
    VtableLayoutMask = 0x0100,
    ReuseSlot = 0x0000,
    NewSlot = 0x0100,
    Abstract = 0x0400,
    SpecialName = 0x0800,
    PinvokeImpl = 0x2000,
    UnmanagedExport = 0x0008,
    RTSpecialName = 0x1000,
    HasSecurity = 0x4000,
    RequireSecObject = 0x8000,
    ReservedMask = 0xd000,
};

// System.Reflection.FieldAttributes
enum class MFieldAttributes : uint32
{
    FieldAccessMask = 0x0007,
    PrivateScope = 0x0000,
    Private = 0x0001,
    FamANDAssem = 0x0002,
    Assembly = 0x0003,
    Family = 0x0004,
    FamORAssem = 0x0005,
    Public = 0x0006,
    Static = 0x0010,
    InitOnly = 0x0020,
    Literal = 0x0040,
    NotSerialized = 0x0080,
    SpecialName = 0x0200,
    PinvokeImpl = 0x2000,
    RTSpecialName = 0x0400,
    HasFieldMarshal = 0x1000,
    HasDefault = 0x8000,
    HasFieldRVA = 0x0100,
    ReservedMask = 0x9500,
};

DECLARE_ENUM_OPERATORS(MTypeAttributes);
DECLARE_ENUM_OPERATORS(MMethodAttributes);
DECLARE_ENUM_OPERATORS(MFieldAttributes);

// Multiple AppDomains are superseded by AssemblyLoadContext in .NET
extern MDomain* MRootDomain;
extern MDomain* MActiveDomain;
extern Array<MDomain*, FixedAllocation<4>> MDomains;

Dictionary<String, void*> CachedFunctions;
Dictionary<void*, MClass*> CachedClassHandles;
Dictionary<void*, MAssembly*> CachedAssemblyHandles;

/// <summary>
/// Returns the function pointer to the managed static method in NativeInterop class.
/// </summary>
void* GetStaticMethodPointer(const String& methodName);

/// <summary>
/// Calls the managed static method with given parameters.
/// </summary>
template<typename RetType, typename... Args>
FORCE_INLINE RetType CallStaticMethod(void* methodPtr, Args... args)
{
#if DOTNET_HOST_MONO
    ASSERT_LOW_LAYER(mono_domain_get()); // Ensure that Mono runtime has been attached to this thread
#endif
    typedef RetType (CORECLR_DELEGATE_CALLTYPE* fun)(Args...);
    return ((fun)methodPtr)(args...);
}

void RegisterNativeLibrary(const char* moduleName, const Char* modulePath)
{
    static void* RegisterNativeLibraryPtr = GetStaticMethodPointer(TEXT("RegisterNativeLibrary"));
    CallStaticMethod<void, const char*, const Char*>(RegisterNativeLibraryPtr, moduleName, modulePath);
}

bool InitHostfxr();
void ShutdownHostfxr();

MAssembly* GetAssembly(void* assemblyHandle);
MClass* GetClass(MType* typeHandle);
MClass* GetOrCreateClass(MType* typeHandle);
MType* GetObjectType(MObject* obj);

void* GetCustomAttribute(const Array<MObject*>& attributes, const MClass* attributeClass)
{
    for (MObject* attr : attributes)
    {
        MClass* attrClass = MCore::Object::GetClass(attr);
        if (attrClass == attributeClass)
            return attr;
    }
    return nullptr;
}

void GetCustomAttributes(Array<MObject*>& result, void* handle, void* getAttributesFunc)
{
    MObject** attributes;
    int numAttributes;
    CallStaticMethod<void, void*, MObject***, int*>(getAttributesFunc, handle, &attributes, &numAttributes);
    result.Set(attributes, numAttributes);
    MCore::GC::FreeMemory(attributes);
}

// Structures used to pass information from runtime, must match with the structures in managed side
struct NativeClassDefinitions
{
    void* typeHandle;
    MClass* nativePointer;
    const char* name;
    const char* fullname;
    const char* namespace_;
    MTypeAttributes typeAttributes;
};

struct NativeMethodDefinitions
{
    const char* name;
    int numParameters;
    void* handle;
    MMethodAttributes methodAttributes;
};

struct NativeFieldDefinitions
{
    const char* name;
    void* fieldHandle;
    void* fieldType;
    int fieldOffset;
    MFieldAttributes fieldAttributes;
};

struct NativePropertyDefinitions
{
    const char* name;
    void* propertyHandle;
    void* getterHandle;
    void* setterHandle;
    MMethodAttributes getterAttributes;
    MMethodAttributes setterAttributes;
};

MDomain* MCore::CreateDomain(const StringAnsi& domainName)
{
    return nullptr;
}

void MCore::UnloadDomain(const StringAnsi& domainName)
{
}

bool MCore::LoadEngine()
{
    PROFILE_CPU();

    // Initialize hostfxr
    if (InitHostfxr())
        return true;

    // Prepare managed side
    CallStaticMethod<void>(GetStaticMethodPointer(TEXT("Init")));
#ifdef MCORE_MAIN_MODULE_NAME
    // MCORE_MAIN_MODULE_NAME define is injected by Scripting.Build.cs on platforms that use separate shared library for engine symbols
    ::String flaxLibraryPath(Platform::GetMainDirectory() / TEXT(MACRO_TO_STR(MCORE_MAIN_MODULE_NAME)));
#else
    ::String flaxLibraryPath(Platform::GetExecutableFilePath());
#endif
#if PLATFORM_MAC
    // On some platforms all native binaries are side-by-side with the app in a different folder
    if (!FileSystem::FileExists(flaxLibraryPath))
    {
        flaxLibraryPath = ::String(StringUtils::GetDirectoryName(Platform::GetExecutableFilePath())) / StringUtils::GetFileName(flaxLibraryPath);
    }
#endif
#if !PLATFORM_SWITCH
    if (!FileSystem::FileExists(flaxLibraryPath))
    {
        LOG(Error, "Flax Engine native library file is missing ({0})", flaxLibraryPath);
    }
#endif
    RegisterNativeLibrary("FlaxEngine", flaxLibraryPath.Get());

    MRootDomain = New<MDomain>("Root");
    MDomains.Add(MRootDomain);

    char* buildInfo = CallStaticMethod<char*>(GetStaticMethodPointer(TEXT("GetRuntimeInformation")));
    LOG(Info, ".NET runtime version: {0}", ::String(buildInfo));
    MCore::GC::FreeMemory(buildInfo);

    return false;
}

void MCore::UnloadEngine()
{
    if (!MRootDomain)
        return;
    PROFILE_CPU();
    CallStaticMethod<void>(GetStaticMethodPointer(TEXT("Exit")));
    MDomains.ClearDelete();
    MRootDomain = nullptr;
    ShutdownHostfxr();
}

#if USE_EDITOR

void MCore::ReloadScriptingAssemblyLoadContext()
{
    // Clear any cached class attributes (see https://github.com/FlaxEngine/FlaxEngine/issues/1108)
    for (auto e : CachedClassHandles)
    {
        e.Value->_hasCachedAttributes = false;
        e.Value->_attributes.Clear();
    }
    for (auto e : CachedAssemblyHandles)
    {
        MAssembly* a = e.Value;
        if (!a->IsLoaded() || !a->_hasCachedClasses)
            continue;
        for (auto q : a->GetClasses())
        {
            MClass* c = q.Value;
            c->_hasCachedAttributes = false;
            c->_attributes.Clear();
            if (c->_hasCachedMethods)
            {
                for (MMethod* m : c->GetMethods())
                {
                    m->_hasCachedAttributes = false;
                    m->_attributes.Clear();
                }
            }
            if (c->_hasCachedFields)
            {
                for (MField* f : c->GetFields())
                {
                    f->_hasCachedAttributes = false;
                    f->_attributes.Clear();
                }
            }
            if (c->_hasCachedProperties)
            {
                for (MProperty* p : c->GetProperties())
                {
                    p->_hasCachedAttributes = false;
                    p->_attributes.Clear();
                }
            }
        }
    }

    static void* ReloadScriptingAssemblyLoadContextPtr = GetStaticMethodPointer(TEXT("ReloadScriptingAssemblyLoadContext"));
    CallStaticMethod<void>(ReloadScriptingAssemblyLoadContextPtr);
}

#endif

MObject* MCore::Object::Box(void* value, const MClass* klass)
{
    static void* BoxValuePtr = GetStaticMethodPointer(TEXT("BoxValue"));
    return (MObject*)CallStaticMethod<void*, void*, void*>(BoxValuePtr, klass->_handle, value);
}

void* MCore::Object::Unbox(MObject* obj)
{
    static void* UnboxValuePtr = GetStaticMethodPointer(TEXT("UnboxValue"));
    return CallStaticMethod<void*, void*>(UnboxValuePtr, obj);
}

MObject* MCore::Object::New(const MClass* klass)
{
    static void* NewObjectPtr = GetStaticMethodPointer(TEXT("NewObject"));
    return (MObject*)CallStaticMethod<void*, void*>(NewObjectPtr, klass->_handle);
}

void MCore::Object::Init(MObject* obj)
{
    static void* ObjectInitPtr = GetStaticMethodPointer(TEXT("ObjectInit"));
    CallStaticMethod<void, void*>(ObjectInitPtr, obj);
}

MClass* MCore::Object::GetClass(MObject* obj)
{
    ASSERT(obj);
    static void* GetObjectClassPtr = GetStaticMethodPointer(TEXT("GetObjectClass"));
    return (MClass*)CallStaticMethod<MClass*, void*>(GetObjectClassPtr, obj);
}

MString* MCore::Object::ToString(MObject* obj)
{
    static void* GetObjectStringPtr = GetStaticMethodPointer(TEXT("GetObjectString"));
    return (MString*)CallStaticMethod<void*, void*>(GetObjectStringPtr, obj);
}

int32 MCore::Object::GetHashCode(MObject* obj)
{
    static void* GetObjectStringPtr = GetStaticMethodPointer(TEXT("GetObjectHashCode"));
    return CallStaticMethod<int32, void*>(GetObjectStringPtr, obj);
}

MString* MCore::String::GetEmpty(MDomain* domain)
{
    static void* GetStringEmptyPtr = GetStaticMethodPointer(TEXT("GetStringEmpty"));
    return (MString*)CallStaticMethod<void*>(GetStringEmptyPtr);
}

MString* MCore::String::New(const char* str, int32 length, MDomain* domain)
{
    static void* NewStringUTF8Ptr = GetStaticMethodPointer(TEXT("NewStringUTF8"));
    return (MString*)CallStaticMethod<void*, const char*, int>(NewStringUTF8Ptr, str, length);
}

MString* MCore::String::New(const Char* str, int32 length, MDomain* domain)
{
    static void* NewStringUTF16Ptr = GetStaticMethodPointer(TEXT("NewStringUTF16"));
    return (MString*)CallStaticMethod<void*, const Char*, int>(NewStringUTF16Ptr, str, length);
}

StringView MCore::String::GetChars(MString* obj)
{
    int32 length = 0;
    static void* GetStringPointerPtr = GetStaticMethodPointer(TEXT("GetStringPointer"));
    const Char* chars = CallStaticMethod<const Char*, void*, int*>(GetStringPointerPtr, obj, &length);
    return StringView(chars, length);
}

MArray* MCore::Array::New(const MClass* elementKlass, int32 length)
{
    static void* NewArrayPtr = GetStaticMethodPointer(TEXT("NewArray"));
    return (MArray*)CallStaticMethod<void*, void*, long long>(NewArrayPtr, elementKlass->_handle, length);
}

MClass* MCore::Array::GetClass(MClass* elementKlass)
{
    static void* GetArrayTypeFromElementTypePtr = GetStaticMethodPointer(TEXT("GetArrayTypeFromElementType"));
    MType* typeHandle = (MType*)CallStaticMethod<void*, void*>(GetArrayTypeFromElementTypePtr, elementKlass->_handle);
    return GetOrCreateClass(typeHandle);
}

MClass* MCore::Array::GetArrayClass(const MArray* obj)
{
    static void* GetArrayTypeFromWrappedArrayPtr = GetStaticMethodPointer(TEXT("GetArrayTypeFromWrappedArray"));
    MType* typeHandle = (MType*)CallStaticMethod<void*, void*>(GetArrayTypeFromWrappedArrayPtr, (void*)obj);
    return GetOrCreateClass(typeHandle);
}

int32 MCore::Array::GetLength(const MArray* obj)
{
    static void* GetArrayLengthPtr = GetStaticMethodPointer(TEXT("GetArrayLength"));
    return CallStaticMethod<int, void*>(GetArrayLengthPtr, (void*)obj);
}

void* MCore::Array::GetAddress(const MArray* obj)
{
    static void* GetArrayPointerPtr = GetStaticMethodPointer(TEXT("GetArrayPointer"));
    return CallStaticMethod<void*, void*>(GetArrayPointerPtr, (void*)obj);
}

MArray* MCore::Array::Unbox(MObject* obj)
{
    static void* GetArrayPtr = GetStaticMethodPointer(TEXT("GetArray"));
    return (MArray*)CallStaticMethod<void*, void*>(GetArrayPtr, (void*)obj);
}

MGCHandle MCore::GCHandle::New(MObject* obj, bool pinned)
{
    ASSERT(obj);
    static void* NewGCHandlePtr = GetStaticMethodPointer(TEXT("NewGCHandle"));
    return (MGCHandle)CallStaticMethod<void*, void*, bool>(NewGCHandlePtr, obj, pinned);
}

MGCHandle MCore::GCHandle::NewWeak(MObject* obj, bool trackResurrection)
{
    ASSERT(obj);
    static void* NewGCHandleWeakPtr = GetStaticMethodPointer(TEXT("NewGCHandleWeak"));
    return (MGCHandle)CallStaticMethod<void*, void*, bool>(NewGCHandleWeakPtr, obj, trackResurrection);
}

MObject* MCore::GCHandle::GetTarget(const MGCHandle& handle)
{
    return (MObject*)(void*)handle;
}

void MCore::GCHandle::Free(const MGCHandle& handle)
{
    static void* FreeGCHandlePtr = GetStaticMethodPointer(TEXT("FreeGCHandle"));
    CallStaticMethod<void, void*>(FreeGCHandlePtr, (void*)handle);
}

void MCore::GC::Collect()
{
    PROFILE_CPU();
    static void* GCCollectPtr = GetStaticMethodPointer(TEXT("GCCollect"));
    CallStaticMethod<void, int, int, bool, bool>(GCCollectPtr, MaxGeneration(), (int)MGCCollectionMode::Default, true, false);
}

void MCore::GC::Collect(int32 generation)
{
    PROFILE_CPU();
    static void* GCCollectPtr = GetStaticMethodPointer(TEXT("GCCollect"));
    CallStaticMethod<void, int, int, bool, bool>(GCCollectPtr, generation, (int)MGCCollectionMode::Default, true, false);
}

void MCore::GC::Collect(int32 generation, MGCCollectionMode collectionMode, bool blocking, bool compacting)
{
    PROFILE_CPU();
    static void* GCCollectPtr = GetStaticMethodPointer(TEXT("GCCollect"));
    CallStaticMethod<void, int, int, bool, bool>(GCCollectPtr, generation, (int)collectionMode, blocking, compacting);
}

int32 MCore::GC::MaxGeneration()
{
    static int32 maxGeneration = CallStaticMethod<int32>(GetStaticMethodPointer(TEXT("GCMaxGeneration")));
    return maxGeneration;
}

void MCore::GC::WaitForPendingFinalizers()
{
    PROFILE_CPU();
    static void* GCWaitForPendingFinalizersPtr = GetStaticMethodPointer(TEXT("GCWaitForPendingFinalizers"));
    CallStaticMethod<void>(GCWaitForPendingFinalizersPtr);
}

void MCore::GC::WriteRef(void* ptr, MObject* ref)
{
    *(void**)ptr = ref;
}

void MCore::GC::WriteValue(void* dst, void* src, int32 count, const MClass* klass)
{
    const int32 size = klass->GetInstanceSize();
    memcpy(dst, src, count * size);
}

void MCore::GC::WriteArrayRef(MArray* dst, MObject* ref, int32 index)
{
    static void* WriteArrayReferencePtr = GetStaticMethodPointer(TEXT("WriteArrayReference"));
    CallStaticMethod<void, void*, void*, int32>(WriteArrayReferencePtr, dst, ref, index);
}

void MCore::GC::WriteArrayRef(MArray* dst, Span<MObject*> refs)
{
    static void* WriteArrayReferencesPtr = GetStaticMethodPointer(TEXT("WriteArrayReferences"));
    CallStaticMethod<void, void*, void*, int32>(WriteArrayReferencesPtr, dst, refs.Get(), refs.Length());
}

void* MCore::GC::AllocateMemory(int32 size, bool coTaskMem)
{
    static void* AllocMemoryPtr = GetStaticMethodPointer(TEXT("AllocMemory"));
    return CallStaticMethod<void*, int, bool>(AllocMemoryPtr, size, coTaskMem);
}

void MCore::GC::FreeMemory(void* ptr, bool coTaskMem)
{
    if (!ptr)
        return;
    static void* FreeMemoryPtr = GetStaticMethodPointer(TEXT("FreeMemory"));
    CallStaticMethod<void, void*, bool>(FreeMemoryPtr, ptr, coTaskMem);
}

void MCore::Thread::Attach()
{
#if DOTNET_HOST_MONO
    if (!IsInMainThread() && !mono_domain_get())
    {
        mono_thread_attach(MonoDomainHandle);
    }
#endif
}

void MCore::Thread::Exit()
{
}

bool MCore::Thread::IsAttached()
{
    return true;
}

void MCore::Exception::Throw(MObject* exception)
{
    static void* RaiseExceptionPtr = GetStaticMethodPointer(TEXT("RaiseException"));
    CallStaticMethod<void*, void*>(RaiseExceptionPtr, exception);
}

MObject* MCore::Exception::GetNullReference()
{
    static void* GetNullReferenceExceptionPtr = GetStaticMethodPointer(TEXT("GetNullReferenceException"));
    return (MObject*)CallStaticMethod<void*>(GetNullReferenceExceptionPtr);
}

MObject* MCore::Exception::Get(const char* msg)
{
    static void* GetExceptionPtr = GetStaticMethodPointer(TEXT("GetException"));
    return (MObject*)CallStaticMethod<void*, const char*>(GetExceptionPtr, msg);
}

MObject* MCore::Exception::GetArgument(const char* arg, const char* msg)
{
    static void* GetArgumentExceptionPtr = GetStaticMethodPointer(TEXT("GetArgumentException"));
    return (MObject*)CallStaticMethod<void*>(GetArgumentExceptionPtr);
}

MObject* MCore::Exception::GetArgumentNull(const char* arg)
{
    static void* GetArgumentNullExceptionPtr = GetStaticMethodPointer(TEXT("GetArgumentNullException"));
    return (MObject*)CallStaticMethod<void*>(GetArgumentNullExceptionPtr);
}

MObject* MCore::Exception::GetArgumentOutOfRange(const char* arg)
{
    static void* GetArgumentOutOfRangeExceptionPtr = GetStaticMethodPointer(TEXT("GetArgumentOutOfRangeException"));
    return (MObject*)CallStaticMethod<void*>(GetArgumentOutOfRangeExceptionPtr);
}

MObject* MCore::Exception::GetNotSupported(const char* msg)
{
    static void* GetNotSupportedExceptionPtr = GetStaticMethodPointer(TEXT("GetNotSupportedException"));
    return (MObject*)CallStaticMethod<void*>(GetNotSupportedExceptionPtr);
}

::String MCore::Type::ToString(MType* type)
{
    MClass* klass = GetOrCreateClass(type);
    return ::String(klass->GetFullName());
}

MClass* MCore::Type::GetClass(MType* type)
{
    static void* GetTypeClassPtr = GetStaticMethodPointer(TEXT("GetTypeClass"));
    return CallStaticMethod<MClass*, void*>(GetTypeClassPtr, type);
}

MType* MCore::Type::GetElementType(MType* type)
{
    static void* GetElementClassPtr = GetStaticMethodPointer(TEXT("GetElementClass"));
    return (MType*)CallStaticMethod<void*, void*>(GetElementClassPtr, type);
}

int32 MCore::Type::GetSize(MType* type)
{
    return GetOrCreateClass(type)->GetInstanceSize();
}

MTypes MCore::Type::GetType(MType* type)
{
    MClass* klass = GetOrCreateClass(type);
    if (klass->_types == 0)
    {
        static void* GetTypeMTypesEnumPtr = GetStaticMethodPointer(TEXT("GetTypeMTypesEnum"));
        klass->_types = CallStaticMethod<uint32, void*>(GetTypeMTypesEnumPtr, klass->_handle);
    }
    return (MTypes)klass->_types;
}

bool MCore::Type::IsPointer(MType* type)
{
    static void* GetTypeIsPointerPtr = GetStaticMethodPointer(TEXT("GetTypeIsPointer"));
    return CallStaticMethod<bool, void*>(GetTypeIsPointerPtr, type);
}

bool MCore::Type::IsReference(MType* type)
{
    static void* GetTypeIsReferencePtr = GetStaticMethodPointer(TEXT("GetTypeIsReference"));
    return CallStaticMethod<bool, void*>(GetTypeIsReferencePtr, type);
}

void MCore::ScriptingObject::SetInternalValues(MClass* klass, MObject* object, void* unmanagedPtr, const Guid* id)
{
#if PLATFORM_DESKTOP && !USE_MONO_AOT
    static void* ScriptingObjectSetInternalValuesPtr = GetStaticMethodPointer(TEXT("ScriptingObjectSetInternalValues"));
    CallStaticMethod<void, MObject*, void*, const Guid*>(ScriptingObjectSetInternalValuesPtr, object, unmanagedPtr, id);
#else
    const MField* monoUnmanagedPtrField = klass->GetField("__unmanagedPtr");
    if (monoUnmanagedPtrField)
        monoUnmanagedPtrField->SetValue(object, &unmanagedPtr);
    const MField* monoIdField = klass->GetField("__internalId");
    if (id != nullptr && monoIdField)
        monoIdField->SetValue(object, (void*)id);
#endif
}

MObject* MCore::ScriptingObject::CreateScriptingObject(MClass* klass, void* unmanagedPtr, const Guid* id)
{
#if PLATFORM_DESKTOP && !USE_MONO_AOT
    static void* ScriptingObjectSetInternalValuesPtr = GetStaticMethodPointer(TEXT("ScriptingObjectCreate"));
    return CallStaticMethod<MObject*, void*, void*, const Guid*>(ScriptingObjectSetInternalValuesPtr, klass->_handle, unmanagedPtr, id);
#else
    MObject* object = MCore::Object::New(klass);
    if (object)
    {
        MCore::ScriptingObject::SetInternalValues(klass, object, unmanagedPtr, id);
        MCore::Object::Init(object);
    }
    return object;
#endif
}

const MAssembly::ClassesDictionary& MAssembly::GetClasses() const
{
    if (_hasCachedClasses || !IsLoaded())
        return _classes;
    PROFILE_CPU();
    Stopwatch stopwatch;

#if TRACY_ENABLE
    ZoneText(*_name, _name.Length());
#endif
    ScopeLock lock(BinaryModule::Locker);
    if (_hasCachedClasses)
        return _classes;
    ASSERT(_classes.IsEmpty());

    NativeClassDefinitions* managedClasses;
    int classCount;
    static void* GetManagedClassesPtr = GetStaticMethodPointer(TEXT("GetManagedClasses"));
    CallStaticMethod<void, void*, NativeClassDefinitions**, int*>(GetManagedClassesPtr, _handle, &managedClasses, &classCount);
    _classes.EnsureCapacity(classCount);
    for (int32 i = 0; i < classCount; i++)
    {
        NativeClassDefinitions& managedClass = managedClasses[i];

        // Create class object
        MClass* klass = New<MClass>(this, managedClass.typeHandle, managedClass.name, managedClass.fullname, managedClass.namespace_, managedClass.typeAttributes);
        _classes.Add(klass->GetFullName(), klass);

        managedClass.nativePointer = klass;

        MCore::GC::FreeMemory((void*)managedClasses[i].name);
        MCore::GC::FreeMemory((void*)managedClasses[i].fullname);
        MCore::GC::FreeMemory((void*)managedClasses[i].namespace_);
    }

    static void* RegisterManagedClassNativePointersPtr = GetStaticMethodPointer(TEXT("RegisterManagedClassNativePointers"));
    CallStaticMethod<void, NativeClassDefinitions**, int>(RegisterManagedClassNativePointersPtr, &managedClasses, classCount);

    MCore::GC::FreeMemory(managedClasses);

    stopwatch.Stop();
    LOG(Info, "Caching classes for assembly {0} took {1}ms", String(_name), stopwatch.GetMilliseconds());

#if 0
    for (auto i = _classes.Begin(); i.IsNotEnd(); ++i)
        LOG(Info, "Class: {0}", String(i->Value->GetFullName()));
#endif

    _hasCachedClasses = true;
    return _classes;
}

void GetAssemblyName(void* assemblyHandle, StringAnsi& name, StringAnsi& fullname)
{
    static void* GetAssemblyNamePtr = GetStaticMethodPointer(TEXT("GetAssemblyName"));
    const char* name_;
    const char* fullname_;
    CallStaticMethod<void, void*, const char**, const char**>(GetAssemblyNamePtr, assemblyHandle, &name_, &fullname_);
    name = name_;
    fullname = fullname_;
    MCore::GC::FreeMemory((void*)name_);
    MCore::GC::FreeMemory((void*)fullname_);
}

DEFINE_INTERNAL_CALL(void) NativeInterop_CreateClass(NativeClassDefinitions* managedClass, void* assemblyHandle)
{
    ScopeLock lock(BinaryModule::Locker);
    MAssembly* assembly = GetAssembly(assemblyHandle);
    if (assembly == nullptr)
    {
        StringAnsi assemblyName;
        StringAnsi assemblyFullName;
        GetAssemblyName(assemblyHandle, assemblyName, assemblyFullName);
        assembly = New<MAssembly>(nullptr, assemblyName, assemblyFullName, assemblyHandle);
        CachedAssemblyHandles.Add(assemblyHandle, assembly);
    }

    MClass* klass = New<MClass>(assembly, managedClass->typeHandle, managedClass->name, managedClass->fullname, managedClass->namespace_, managedClass->typeAttributes);
    if (assembly != nullptr)
    {
        auto& classes = const_cast<MAssembly::ClassesDictionary&>(assembly->GetClasses());
        MClass* oldKlass;
        if (classes.TryGet(klass->GetFullName(), oldKlass))
        {
            LOG(Warning, "Class '{0}' was already added to assembly '{1}'", String(klass->GetFullName()), String(assembly->GetName()));
            Delete(klass);
            klass = oldKlass;
        }
        else
        {
            classes.Add(klass->GetFullName(), klass);
        }
    }
    managedClass->nativePointer = klass;
}

bool MAssembly::LoadCorlib()
{
    if (IsLoaded())
        return false;
    PROFILE_CPU();
#if TRACY_ENABLE
    const StringAnsiView name("Corlib");
    ZoneText(*name, name.Length());
#endif

    // Ensure to be unloaded
    Unload();

    // Start
    Stopwatch stopwatch;
    OnLoading();

    // Load
    {
        static void* GetAssemblyByNamePtr = GetStaticMethodPointer(TEXT("GetAssemblyByName"));
        _handle = CallStaticMethod<void*, const char*>(GetAssemblyByNamePtr, "System.Private.CoreLib");
        GetAssemblyName(_handle, _name, _fullname);
    }
    if (_handle == nullptr)
    {
        OnLoadFailed();
        return true;
    }
    _hasCachedClasses = false;
    CachedAssemblyHandles.Add(_handle, this);

    // End
    OnLoaded(stopwatch);
    return false;
}

bool MAssembly::LoadImage(const String& assemblyPath, const StringView& nativePath)
{
    // TODO: Use new hostfxr delegate load_assembly_bytes? (.NET 8+)
    // Open .Net assembly
    static void* LoadAssemblyImagePtr = GetStaticMethodPointer(TEXT("LoadAssemblyImage"));
    _handle = CallStaticMethod<void*, const Char*>(LoadAssemblyImagePtr, assemblyPath.Get());
    if (_handle == nullptr)
    {
        Log::CLRInnerException(TEXT(".NET assembly image is invalid at ") + assemblyPath);
        return true;
    }
    GetAssemblyName(_handle, _name, _fullname);
    CachedAssemblyHandles.Add(_handle, this);

    // Provide new path of hot-reloaded native library path for managed DllImport
    if (nativePath.HasChars())
    {
        StringAnsi nativeName = _name.EndsWith(".CSharp") ? StringAnsi(_name.Get(), _name.Length() - 7) : StringAnsi(_name);
        RegisterNativeLibrary(nativeName.Get(), nativePath.Get());
    }
#if USE_EDITOR
    // Register the editor module location for Assembly resolver
    else
    {
        RegisterNativeLibrary(_name.Get(), assemblyPath.Get());
    }
#endif

    _hasCachedClasses = false;
    _assemblyPath = assemblyPath;
    return false;
}

bool MAssembly::UnloadImage(bool isReloading)
{
    if (_handle && isReloading)
    {
        LOG(Info, "Unloading managed assembly \'{0}\' (is reloading)", String(_name));

        static void* CloseAssemblyPtr = GetStaticMethodPointer(TEXT("CloseAssembly"));
        CallStaticMethod<void, const void*>(CloseAssemblyPtr, _handle);

        CachedAssemblyHandles.Remove(_handle);
        _handle = nullptr;
    }
    return false;
}

MClass::MClass(const MAssembly* parentAssembly, void* handle, const char* name, const char* fullname, const char* namespace_, MTypeAttributes attributes)
    : _handle(handle)
    , _name(name)
    , _namespace(namespace_)
    , _assembly(parentAssembly)
    , _fullname(fullname)
    , _hasCachedProperties(false)
    , _hasCachedFields(false)
    , _hasCachedMethods(false)
    , _hasCachedAttributes(false)
    , _hasCachedEvents(false)
    , _hasCachedInterfaces(false)
{
    ASSERT(handle != nullptr);
    switch (attributes & MTypeAttributes::VisibilityMask)
    {
    case MTypeAttributes::NotPublic:
    case MTypeAttributes::NestedPrivate:
        _visibility = MVisibility::Private;
        break;
    case MTypeAttributes::Public:
    case MTypeAttributes::NestedPublic:
        _visibility = MVisibility::Public;
        break;
    case MTypeAttributes::NestedFamily:
    case MTypeAttributes::NestedAssembly:
        _visibility = MVisibility::Internal;
        break;
    case MTypeAttributes::NestedFamORAssem:
        _visibility = MVisibility::ProtectedInternal;
        break;
    case MTypeAttributes::NestedFamANDAssem:
        _visibility = MVisibility::PrivateProtected;
        break;
    default:
        CRASH;
    }

    const MTypeAttributes staticClassFlags = MTypeAttributes::Abstract | MTypeAttributes::Sealed;
    _isStatic = (attributes & staticClassFlags) == staticClassFlags;
    _isSealed = !_isStatic && (attributes & MTypeAttributes::Sealed) == MTypeAttributes::Sealed;
    _isAbstract = !_isStatic && (attributes & MTypeAttributes::Abstract) == MTypeAttributes::Abstract;
    _isInterface = (attributes & MTypeAttributes::ClassSemanticsMask) == MTypeAttributes::Interface;

    // TODO: pass type info from C# side at once (pack into flags with attributes)

    static void* TypeIsValueTypePtr = GetStaticMethodPointer(TEXT("TypeIsValueType"));
    _isValueType = CallStaticMethod<bool, void*>(TypeIsValueTypePtr, handle);

    static void* TypeIsEnumPtr = GetStaticMethodPointer(TEXT("TypeIsEnum"));
    _isEnum = CallStaticMethod<bool, void*>(TypeIsEnumPtr, handle);

    CachedClassHandles[handle] = this;
}

bool MAssembly::ResolveMissingFile(String& assemblyPath) const
{
#if DOTNET_HOST_MONO
    // Fallback to AOT-ed assembly location
    assemblyPath = Globals::BinariesFolder / TEXT("Dotnet") / StringUtils::GetFileName(assemblyPath);
    return !FileSystem::FileExists(assemblyPath);
#endif
    return true;
}

MClass::~MClass()
{
    _methods.ClearDelete();
    _fields.ClearDelete();
    _properties.ClearDelete();
    _events.ClearDelete();

    CachedClassHandles.Remove(_handle);
}

StringAnsiView MClass::GetName() const
{
    return _name;
}

StringAnsiView MClass::GetNamespace() const
{
    return _namespace;
}

MType* MClass::GetType() const
{
    return (MType*)_handle;
}

MClass* MClass::GetBaseClass() const
{
    static void* GetClassParentPtr = GetStaticMethodPointer(TEXT("GetClassParent"));
    return CallStaticMethod<MClass*, void*>(GetClassParentPtr, _handle);
}

bool MClass::IsSubClassOf(const MClass* klass, bool checkInterfaces) const
{
    static void* TypeIsSubclassOfPtr = GetStaticMethodPointer(TEXT("TypeIsSubclassOf"));
    return klass && CallStaticMethod<bool, void*, void*, bool>(TypeIsSubclassOfPtr, _handle, klass->_handle, checkInterfaces);
}

bool MClass::HasInterface(const MClass* klass) const
{
    static void* TypeIsAssignableFrom = GetStaticMethodPointer(TEXT("TypeIsAssignableFrom"));
    return klass && CallStaticMethod<bool, void*, void*>(TypeIsAssignableFrom, klass->_handle, _handle);
}

bool MClass::IsInstanceOfType(MObject* object) const
{
    if (object == nullptr)
        return false;
    MClass* objectClass = MCore::Object::GetClass(object);
    return IsSubClassOf(objectClass, false);
}

uint32 MClass::GetInstanceSize() const
{
    if (_size != 0)
        return _size;
    static void* NativeSizeOfPtr = GetStaticMethodPointer(TEXT("NativeSizeOf"));
    _size = CallStaticMethod<int, void*>(NativeSizeOfPtr, _handle);
    return _size;
}

MClass* MClass::GetElementClass() const
{
    static void* GetElementClassPtr = GetStaticMethodPointer(TEXT("GetElementClass"));
    return CallStaticMethod<MClass*, void*>(GetElementClassPtr, _handle);
}

MMethod* MClass::GetMethod(const char* name, int32 numParams) const
{
    GetMethods();
    for (int32 i = 0; i < _methods.Count(); i++)
    {
        if (_methods[i]->GetParametersCount() == numParams && _methods[i]->GetName() == name)
            return _methods[i];
    }
    return nullptr;
}

const Array<MMethod*>& MClass::GetMethods() const
{
    if (_hasCachedMethods)
        return _methods;
    ScopeLock lock(BinaryModule::Locker);
    if (_hasCachedMethods)
        return _methods;

    NativeMethodDefinitions* methods;
    int methodsCount;
    static void* GetClassMethodsPtr = GetStaticMethodPointer(TEXT("GetClassMethods"));
    CallStaticMethod<void, void*, NativeMethodDefinitions**, int*>(GetClassMethodsPtr, _handle, &methods, &methodsCount);
    _methods.Resize(methodsCount);
    for (int32 i = 0; i < methodsCount; i++)
    {
        NativeMethodDefinitions& definition = methods[i];
        MMethod* method = New<MMethod>(const_cast<MClass*>(this), StringAnsi(definition.name), definition.handle, definition.numParameters, definition.methodAttributes);
        _methods[i] = method;
        MCore::GC::FreeMemory((void*)definition.name);
    }
    MCore::GC::FreeMemory(methods);

    _hasCachedMethods = true;
    return _methods;
}

MField* MClass::GetField(const char* name) const
{
    GetFields();
    for (int32 i = 0; i < _fields.Count(); i++)
    {
        if (_fields[i]->GetName() == name)
            return _fields[i];
    }
    return nullptr;
}

const Array<MField*>& MClass::GetFields() const
{
    if (_hasCachedFields)
        return _fields;
    ScopeLock lock(BinaryModule::Locker);
    if (_hasCachedFields)
        return _fields;

    NativeFieldDefinitions* fields;
    int numFields;
    static void* GetClassFieldsPtr = GetStaticMethodPointer(TEXT("GetClassFields"));
    CallStaticMethod<void, void*, NativeFieldDefinitions**, int*>(GetClassFieldsPtr, _handle, &fields, &numFields);
    _fields.Resize(numFields);
    for (int32 i = 0; i < numFields; i++)
    {
        NativeFieldDefinitions& definition = fields[i];
        MField* field = New<MField>(const_cast<MClass*>(this), definition.fieldHandle, definition.name, definition.fieldType, definition.fieldOffset, definition.fieldAttributes);
        _fields[i] = field;
        MCore::GC::FreeMemory((void*)definition.name);
    }
    MCore::GC::FreeMemory(fields);

    _hasCachedFields = true;
    return _fields;
}

const Array<MEvent*>& MClass::GetEvents() const
{
    if (_hasCachedEvents)
        return _events;

    // TODO: implement MEvent in .NET

    _hasCachedEvents = true;
    return _events;
}

MProperty* MClass::GetProperty(const char* name) const
{
    GetProperties();
    for (int32 i = 0; i < _properties.Count(); i++)
    {
        if (_properties[i]->GetName() == name)
            return _properties[i];
    }
    return nullptr;
}

const Array<MProperty*>& MClass::GetProperties() const
{
    if (_hasCachedProperties)
        return _properties;
    ScopeLock lock(BinaryModule::Locker);
    if (_hasCachedProperties)
        return _properties;

    NativePropertyDefinitions* foundProperties;
    int numProperties;
    static void* GetClassPropertiesPtr = GetStaticMethodPointer(TEXT("GetClassProperties"));
    CallStaticMethod<void, void*, NativePropertyDefinitions**, int*>(GetClassPropertiesPtr, _handle, &foundProperties, &numProperties);
    _properties.Resize(numProperties);
    for (int i = 0; i < numProperties; i++)
    {
        const NativePropertyDefinitions& definition = foundProperties[i];
        MProperty* property = New<MProperty>(const_cast<MClass*>(this), definition.name, definition.propertyHandle, definition.getterHandle, definition.setterHandle, definition.getterAttributes, definition.setterAttributes);
        _properties[i] = property;
        MCore::GC::FreeMemory((void*)definition.name);
    }
    MCore::GC::FreeMemory(foundProperties);

    _hasCachedProperties = true;
    return _properties;
}

const Array<MClass*>& MClass::GetInterfaces() const
{
    if (_hasCachedInterfaces)
        return _interfaces;
    ScopeLock lock(BinaryModule::Locker);
    if (_hasCachedInterfaces)
        return _interfaces;

    MType** foundInterfaceTypes;
    int numInterfaces;
    static void* GetClassInterfacesPtr = GetStaticMethodPointer(TEXT("GetClassInterfaces"));
    CallStaticMethod<void, void*, MType***, int*>(GetClassInterfacesPtr, _handle, &foundInterfaceTypes, &numInterfaces);
    _interfaces.Resize(numInterfaces);
    for (int32 i = 0; i < numInterfaces; i++)
    {
        MClass* interfaceClass = GetOrCreateClass(foundInterfaceTypes[i]);
        _interfaces[i] = interfaceClass;
    }
    MCore::GC::FreeMemory(foundInterfaceTypes);

    _hasCachedInterfaces = true;
    return _interfaces;
}

bool MClass::HasAttribute(const MClass* klass) const
{
    return GetCustomAttribute(GetAttributes(), klass) != nullptr;
}

bool MClass::HasAttribute() const
{
    return !GetAttributes().IsEmpty();
}

MObject* MClass::GetAttribute(const MClass* klass) const
{
    return (MObject*)GetCustomAttribute(GetAttributes(), klass);
}

const Array<MObject*>& MClass::GetAttributes() const
{
    if (_hasCachedAttributes)
        return _attributes;
    ScopeLock lock(BinaryModule::Locker);
    if (_hasCachedAttributes)
        return _attributes;
    static void* GetClassAttributesPtr = GetStaticMethodPointer(TEXT("GetClassAttributes"));
    GetCustomAttributes(_attributes, _handle, GetClassAttributesPtr);
    _hasCachedAttributes = true;
    return _attributes;
}

bool MDomain::SetCurrentDomain(bool force)
{
    MActiveDomain = this;
    return true;
}

void MDomain::Dispatch() const
{
}

MEvent::MEvent(MClass* parentClass, void* handle, const char* name)
    : _handle(handle)
    , _addMethod(nullptr)
    , _removeMethod(nullptr)
    , _parentClass(parentClass)
    , _name(name)
    , _hasCachedAttributes(false)
    , _hasAddMonoMethod(true)
    , _hasRemoveMonoMethod(true)
{
}

MMethod* MEvent::GetAddMethod() const
{
    return nullptr; // TODO: implement MEvent in .NET
}

MMethod* MEvent::GetRemoveMethod() const
{
    return nullptr; // TODO: implement MEvent in .NET
}

bool MEvent::HasAttribute(const MClass* klass) const
{
    return GetCustomAttribute(GetAttributes(), klass) != nullptr;
}

bool MEvent::HasAttribute() const
{
    return !GetAttributes().IsEmpty();
}

MObject* MEvent::GetAttribute(const MClass* klass) const
{
    return (MObject*)GetCustomAttribute(GetAttributes(), klass);
}

const Array<MObject*>& MEvent::GetAttributes() const
{
    if (_hasCachedAttributes)
        return _attributes;
    _hasCachedAttributes = true;

    // TODO: implement MEvent in .NET
    return _attributes;
}

MException::MException(MObject* exception)
    : InnerException(nullptr)
{
    ASSERT(exception);
    MClass* exceptionClass = MCore::Object::GetClass(exception);

    MProperty* exceptionMsgProp = exceptionClass->GetProperty("Message");
    MMethod* exceptionMsgGetter = exceptionMsgProp->GetGetMethod();
    MString* exceptionMsg = (MString*)exceptionMsgGetter->Invoke(exception, nullptr, nullptr);
    Message = MUtils::ToString(exceptionMsg);

    MProperty* exceptionStackProp = exceptionClass->GetProperty("StackTrace");
    MMethod* exceptionStackGetter = exceptionStackProp->GetGetMethod();
    MString* exceptionStackTrace = (MString*)exceptionStackGetter->Invoke(exception, nullptr, nullptr);
    StackTrace = MUtils::ToString(exceptionStackTrace);

    MProperty* innerExceptionProp = exceptionClass->GetProperty("InnerException");
    MMethod* innerExceptionGetter = innerExceptionProp->GetGetMethod();
    MObject* innerException = (MObject*)innerExceptionGetter->Invoke(exception, nullptr, nullptr);
    if (innerException)
        InnerException = New<MException>(innerException);
}

MException::~MException()
{
    if (InnerException)
        Delete(InnerException);
}

MField::MField(MClass* parentClass, void* handle, const char* name, void* type, int fieldOffset, MFieldAttributes attributes)
    : _handle(handle)
    , _type(type)
    , _fieldOffset(fieldOffset)
    , _parentClass(parentClass)
    , _name(name)
    , _hasCachedAttributes(false)
{
    switch (attributes & MFieldAttributes::FieldAccessMask)
    {
    case MFieldAttributes::Private:
        _visibility = MVisibility::Private;
        break;
    case MFieldAttributes::FamANDAssem:
        _visibility = MVisibility::PrivateProtected;
        break;
    case MFieldAttributes::Assembly:
        _visibility = MVisibility::Internal;
        break;
    case MFieldAttributes::Family:
        _visibility = MVisibility::Protected;
        break;
    case MFieldAttributes::FamORAssem:
        _visibility = MVisibility::ProtectedInternal;
        break;
    case MFieldAttributes::Public:
        _visibility = MVisibility::Public;
        break;
    default:
        CRASH;
    }
    _isStatic = (attributes & MFieldAttributes::Static) == MFieldAttributes::Static;
}

MType* MField::GetType() const
{
    return (MType*)_type;
}

int32 MField::GetOffset() const
{
    return _fieldOffset;
}

void MField::GetValue(MObject* instance, void* result) const
{
    static void* FieldGetValuePtr = GetStaticMethodPointer(TEXT("FieldGetValue"));
    CallStaticMethod<void, void*, void*, void*>(FieldGetValuePtr, instance, _handle, result);
}

void MField::GetValueReference(MObject* instance, void* result) const
{
    static void* FieldGetValueReferencePtr = GetStaticMethodPointer(TEXT("FieldGetValueReference"));
    CallStaticMethod<void, void*, void*, int, void*>(FieldGetValueReferencePtr, instance, _handle, _fieldOffset, result);
}

MObject* MField::GetValueBoxed(MObject* instance) const
{
    static void* FieldGetValueBoxedPtr = GetStaticMethodPointer(TEXT("FieldGetValueBoxed"));
    return CallStaticMethod<MObject*, void*, void*>(FieldGetValueBoxedPtr, instance, _handle);
}

void MField::SetValue(MObject* instance, void* value) const
{
    static void* FieldSetValuePtr = GetStaticMethodPointer(TEXT("FieldSetValue"));
    CallStaticMethod<void, void*, void*, void*>(FieldSetValuePtr, instance, _handle, value);
}

bool MField::HasAttribute(const MClass* klass) const
{
    return GetCustomAttribute(GetAttributes(), klass) != nullptr;
}

bool MField::HasAttribute() const
{
    return !GetAttributes().IsEmpty();
}

MObject* MField::GetAttribute(const MClass* klass) const
{
    return (MObject*)GetCustomAttribute(GetAttributes(), klass);
}

const Array<MObject*>& MField::GetAttributes() const
{
    if (_hasCachedAttributes)
        return _attributes;
    ScopeLock lock(BinaryModule::Locker);
    if (_hasCachedAttributes)
        return _attributes;
    static void* GetFieldAttributesPtr = GetStaticMethodPointer(TEXT("GetFieldAttributes"));
    GetCustomAttributes(_attributes, _handle, GetFieldAttributesPtr);
    _hasCachedAttributes = true;
    return _attributes;
}

MMethod::MMethod(MClass* parentClass, StringAnsi&& name, void* handle, int32 paramsCount, MMethodAttributes attributes)
    : _handle(handle)
    , _paramsCount(paramsCount)
    , _parentClass(parentClass)
    , _name(MoveTemp(name))
    , _hasCachedAttributes(false)
    , _hasCachedSignature(false)
{
    switch (attributes & MMethodAttributes::MemberAccessMask)
    {
    case MMethodAttributes::Private:
        _visibility = MVisibility::Private;
        break;
    case MMethodAttributes::FamANDAssem:
        _visibility = MVisibility::PrivateProtected;
        break;
    case MMethodAttributes::Assembly:
        _visibility = MVisibility::Internal;
        break;
    case MMethodAttributes::Family:
        _visibility = MVisibility::Protected;
        break;
    case MMethodAttributes::FamORAssem:
        _visibility = MVisibility::ProtectedInternal;
        break;
    case MMethodAttributes::Public:
        _visibility = MVisibility::Public;
        break;
    default:
        CRASH;
    }
    _isStatic = (attributes & MMethodAttributes::Static) == MMethodAttributes::Static;

#if COMPILE_WITH_PROFILER
    const StringAnsi& className = parentClass->GetFullName();
    ProfilerName.Resize(className.Length() + 2 + _name.Length());
    Platform::MemoryCopy(ProfilerName.Get(), className.Get(), className.Length());
    ProfilerName.Get()[className.Length()] = ':';
    ProfilerName.Get()[className.Length() + 1] = ':';
    Platform::MemoryCopy(ProfilerName.Get() + className.Length() + 2, _name.Get(), _name.Length());
    ProfilerData.name = ProfilerName.Get();
    ProfilerData.function = _name.Get();
    ProfilerData.file = nullptr;
    ProfilerData.line = 0;
    ProfilerData.color = 0;
#endif
}

void MMethod::CacheSignature() const
{
    ScopeLock lock(BinaryModule::Locker);
    if (_hasCachedSignature)
        return;

    static void* GetMethodReturnTypePtr = GetStaticMethodPointer(TEXT("GetMethodReturnType"));
    static void* GetMethodParameterTypesPtr = GetStaticMethodPointer(TEXT("GetMethodParameterTypes"));
    _returnType = CallStaticMethod<void*, void*>(GetMethodReturnTypePtr, _handle);
    if (_paramsCount != 0)
    {
        void** parameterTypeHandles;
        CallStaticMethod<void, void*, void***>(GetMethodParameterTypesPtr, _handle, &parameterTypeHandles);
        _parameterTypes.Set(parameterTypeHandles, _paramsCount);
        MCore::GC::FreeMemory(parameterTypeHandles);
    }

    _hasCachedSignature = true;
}

MObject* MMethod::Invoke(void* instance, void** params, MObject** exception) const
{
    PROFILE_CPU_SRC_LOC(ProfilerData);
    static void* InvokeMethodPtr = GetStaticMethodPointer(TEXT("InvokeMethod"));
    return (MObject*)CallStaticMethod<void*, void*, void*, void*, void*>(InvokeMethodPtr, instance, _handle, params, exception);
}

MObject* MMethod::InvokeVirtual(MObject* instance, void** params, MObject** exception) const
{
    return Invoke(instance, params, exception);
}

#if !USE_MONO_AOT

void* MMethod::GetThunk()
{
    if (!_cachedThunk)
    {
        static void* GetThunkPtr = GetStaticMethodPointer(TEXT("GetThunk"));
        _cachedThunk = CallStaticMethod<void*, void*>(GetThunkPtr, _handle);
#if !BUILD_RELEASE
        if (!_cachedThunk)
            LOG(Error, "Failed to get C# method thunk for {0}::{1}", String(_parentClass->GetFullName()), String(_name));
#endif
    }
    return _cachedThunk;
}

#endif

MMethod* MMethod::InflateGeneric() const
{
    // This seams to be unused on .NET (Mono required inflating generic class of the script)
    return const_cast<MMethod*>(this);
}

MType* MMethod::GetReturnType() const
{
    if (!_hasCachedSignature)
        CacheSignature();
    return (MType*)_returnType;
}

int32 MMethod::GetParametersCount() const
{
    return _paramsCount;
}

MType* MMethod::GetParameterType(int32 paramIdx) const
{
    if (!_hasCachedSignature)
        CacheSignature();
    ASSERT_LOW_LAYER(paramIdx >= 0 && paramIdx < _paramsCount);
    return (MType*)_parameterTypes.Get()[paramIdx];
}

bool MMethod::GetParameterIsOut(int32 paramIdx) const
{
    if (!_hasCachedSignature)
        CacheSignature();
    ASSERT_LOW_LAYER(paramIdx >= 0 && paramIdx < _paramsCount);
    // TODO: cache GetParameterIsOut maybe?
    static void* GetMethodParameterIsOutPtr = GetStaticMethodPointer(TEXT("GetMethodParameterIsOut"));
    return CallStaticMethod<bool, void*, int>(GetMethodParameterIsOutPtr, _handle, paramIdx);
}

bool MMethod::HasAttribute(const MClass* klass) const
{
    return GetCustomAttribute(GetAttributes(), klass) != nullptr;
}

bool MMethod::HasAttribute() const
{
    return !GetAttributes().IsEmpty();
}

MObject* MMethod::GetAttribute(const MClass* klass) const
{
    return (MObject*)GetCustomAttribute(GetAttributes(), klass);
}

const Array<MObject*>& MMethod::GetAttributes() const
{
    if (_hasCachedAttributes)
        return _attributes;
    ScopeLock lock(BinaryModule::Locker);
    if (_hasCachedAttributes)
        return _attributes;
    static void* GetMethodAttributesPtr = GetStaticMethodPointer(TEXT("GetMethodAttributes"));
    GetCustomAttributes(_attributes, _handle, GetMethodAttributesPtr);
    _hasCachedAttributes = true;
    return _attributes;
}

MProperty::MProperty(MClass* parentClass, const char* name, void* handle, void* getterHandle, void* setterHandle, MMethodAttributes getterAttributes, MMethodAttributes setterAttributes)
    : _parentClass(parentClass)
    , _name(name)
    , _handle(handle)
    , _hasCachedAttributes(false)
{
    _hasGetMethod = getterHandle != nullptr;
    if (_hasGetMethod)
        _getMethod = New<MMethod>(parentClass, StringAnsi("get_" + _name), getterHandle, 0, getterAttributes);
    else
        _getMethod = nullptr;
    _hasSetMethod = setterHandle != nullptr;
    if (_hasSetMethod)
        _setMethod = New<MMethod>(parentClass, StringAnsi("set_" + _name), setterHandle, 1, setterAttributes);
    else
        _setMethod = nullptr;
}

MProperty::~MProperty()
{
    if (_getMethod)
        Delete(_getMethod);
    if (_setMethod)
        Delete(_setMethod);
}

MMethod* MProperty::GetGetMethod() const
{
    return _getMethod;
}

MMethod* MProperty::GetSetMethod() const
{
    return _setMethod;
}

MObject* MProperty::GetValue(MObject* instance, MObject** exception) const
{
    CHECK_RETURN(_getMethod, nullptr);
    return _getMethod->Invoke(instance, nullptr, exception);
}

void MProperty::SetValue(MObject* instance, void* value, MObject** exception) const
{
    CHECK(_setMethod);
    void* params[1];
    params[0] = value;
    _setMethod->Invoke(instance, params, exception);
}

bool MProperty::HasAttribute(const MClass* klass) const
{
    return GetCustomAttribute(GetAttributes(), klass) != nullptr;
}

bool MProperty::HasAttribute() const
{
    return !GetAttributes().IsEmpty();
}

MObject* MProperty::GetAttribute(const MClass* klass) const
{
    return (MObject*)GetCustomAttribute(GetAttributes(), klass);
}

const Array<MObject*>& MProperty::GetAttributes() const
{
    if (_hasCachedAttributes)
        return _attributes;
    ScopeLock lock(BinaryModule::Locker);
    if (_hasCachedAttributes)
        return _attributes;
    static void* GetPropertyAttributesPtr = GetStaticMethodPointer(TEXT("GetPropertyAttributes"));
    GetCustomAttributes(_attributes, _handle, GetPropertyAttributesPtr);
    _hasCachedAttributes = true;
    return _attributes;
}

MAssembly* GetAssembly(void* assemblyHandle)
{
    ScopeLock lock(BinaryModule::Locker);
    MAssembly* assembly;
    if (CachedAssemblyHandles.TryGet(assemblyHandle, assembly))
        return assembly;
    return nullptr;
}

MClass* GetClass(MType* typeHandle)
{
    ScopeLock lock(BinaryModule::Locker);
    MClass* klass = nullptr;
    CachedClassHandles.TryGet(typeHandle, klass);
    return nullptr;
}

MClass* GetOrCreateClass(MType* typeHandle)
{
    if (!typeHandle)
        return nullptr;
    ScopeLock lock(BinaryModule::Locker);
    MClass* klass;
    if (!CachedClassHandles.TryGet(typeHandle, klass))
    {
        NativeClassDefinitions classInfo;
        void* assemblyHandle;
        static void* GetManagedClassFromTypePtr = GetStaticMethodPointer(TEXT("GetManagedClassFromType"));
        CallStaticMethod<void, void*, void*>(GetManagedClassFromTypePtr, typeHandle, &classInfo, &assemblyHandle);
        MAssembly* assembly = GetAssembly(assemblyHandle);
        klass = New<MClass>(assembly, classInfo.typeHandle, classInfo.name, classInfo.fullname, classInfo.namespace_, classInfo.typeAttributes);
        if (assembly != nullptr)
        {
            auto& classes = const_cast<MAssembly::ClassesDictionary&>(assembly->GetClasses());
            if (classes.ContainsKey(klass->GetFullName()))
            {
                LOG(Warning, "Class '{0}' was already added to assembly '{1}'", String(klass->GetFullName()), String(assembly->GetName()));
            }
            classes[klass->GetFullName()] = klass;
        }

        if (typeHandle != classInfo.typeHandle)
            CallStaticMethod<void, void*, void*>(GetManagedClassFromTypePtr, typeHandle, &classInfo);

        MCore::GC::FreeMemory((void*)classInfo.name);
        MCore::GC::FreeMemory((void*)classInfo.fullname);
        MCore::GC::FreeMemory((void*)classInfo.namespace_);
    }
    ASSERT(klass != nullptr);
    return klass;
}

MType* GetObjectType(MObject* obj)
{
    static void* GetObjectTypePtr = GetStaticMethodPointer(TEXT("GetObjectType"));
    void* typeHandle = CallStaticMethod<void*, void*>(GetObjectTypePtr, obj);
    return (MType*)typeHandle;
}

#if DOTNET_HOST_CORECLR

const char_t* NativeInteropTypeName = FLAX_CORECLR_TEXT("FlaxEngine.Interop.NativeInterop, FlaxEngine.CSharp");
hostfxr_initialize_for_runtime_config_fn hostfxr_initialize_for_runtime_config;
hostfxr_initialize_for_dotnet_command_line_fn hostfxr_initialize_for_dotnet_command_line;
hostfxr_get_runtime_delegate_fn hostfxr_get_runtime_delegate;
hostfxr_close_fn hostfxr_close;
load_assembly_and_get_function_pointer_fn load_assembly_and_get_function_pointer;
get_function_pointer_fn get_function_pointer;
hostfxr_set_error_writer_fn hostfxr_set_error_writer;
hostfxr_get_dotnet_environment_info_result_fn hostfxr_get_dotnet_environment_info_result;
hostfxr_run_app_fn hostfxr_run_app;

bool InitHostfxr()
{
    const ::String csharpLibraryPath = Globals::BinariesFolder / TEXT("FlaxEngine.CSharp.dll");
    const ::String csharpRuntimeConfigPath = Globals::BinariesFolder / TEXT("FlaxEngine.CSharp.runtimeconfig.json");
    if (!FileSystem::FileExists(csharpLibraryPath))
        LOG(Fatal, "Failed to initialize .NET runtime, missing file: {0}", csharpLibraryPath);
    if (!FileSystem::FileExists(csharpRuntimeConfigPath))
        LOG(Fatal, "Failed to initialize .NET runtime, missing file: {0}", csharpRuntimeConfigPath);
    const FLAX_CORECLR_STRING& libraryPath = FLAX_CORECLR_STRING(csharpLibraryPath);

    // Get path to hostfxr library
    get_hostfxr_parameters get_hostfxr_params;
    get_hostfxr_params.size = sizeof(get_hostfxr_parameters);
    get_hostfxr_params.assembly_path = libraryPath.Get();
#if PLATFORM_MAC
    ::String macOSDotnetRoot = TEXT("/usr/local/share/dotnet");
#if defined(__x86_64) || defined(__x86_64__) || defined(__amd64__) || defined(_M_X64)
    // When emulating x64 on arm
    const ::String dotnetRootEmulated = macOSDotnetRoot / TEXT("x64");
    if (FileSystem::FileExists(dotnetRootEmulated / TEXT("dotnet"))) {
        macOSDotnetRoot = dotnetRootEmulated;
    }
#endif
    const FLAX_CORECLR_STRING& finalDotnetRootPath = FLAX_CORECLR_STRING(macOSDotnetRoot);
    get_hostfxr_params.dotnet_root = finalDotnetRootPath.Get();
#else
    get_hostfxr_params.dotnet_root = nullptr;
#endif
    FLAX_CORECLR_STRING dotnetRoot;
    String dotnetRootEnvVar;
    if (!Platform::GetEnvironmentVariable(TEXT("DOTNET_ROOT"), dotnetRootEnvVar) && FileSystem::DirectoryExists(dotnetRootEnvVar))
    {
        dotnetRoot = FLAX_CORECLR_STRING(dotnetRootEnvVar);
        get_hostfxr_params.dotnet_root = dotnetRoot.Get();
    }
#if !USE_EDITOR
    const String bundledDotnetPath = Globals::ProjectFolder / TEXT("Dotnet");
    if (FileSystem::DirectoryExists(bundledDotnetPath))
    {
        dotnetRoot = FLAX_CORECLR_STRING(bundledDotnetPath);
#if PLATFORM_WINDOWS_FAMILY
        dotnetRoot.Replace('/', '\\');
#endif
        get_hostfxr_params.dotnet_root = dotnetRoot.Get();
    }
#endif
    char_t hostfxrPath[1024];
    size_t hostfxrPathSize = sizeof(hostfxrPath) / sizeof(char_t);
    int rc = get_hostfxr_path(hostfxrPath, &hostfxrPathSize, &get_hostfxr_params);
    if (rc != 0)
    {
        LOG(Error, "Failed to find hostfxr: {0:x} ({1})", (unsigned int)rc, String(get_hostfxr_params.dotnet_root));

        // Warn user about missing .Net
#if PLATFORM_DESKTOP
        Platform::OpenUrl(TEXT("https://dotnet.microsoft.com/en-us/download/dotnet"));
#endif
#if USE_EDITOR
        LOG(Fatal, "Missing .NET 8 or later SDK installation required to run Flax Editor.");
#else
        LOG(Fatal, "Missing .NET 8 or later Runtime installation required to run this application.");
#endif
        return true;
    }
    String path(hostfxrPath);
    LOG(Info, "Found hostfxr in {0}", path);

    // Get API from hostfxr library
    void* hostfxr = Platform::LoadLibrary(path.Get());
    if (hostfxr == nullptr)
    {
        if (FileSystem::FileExists(path))
            LOG(Fatal, "Failed to load hostfxr library, possible platform/architecture mismatch with the library. See log for more information. ({0})", path);
        else
            LOG(Fatal, "Failed to load hostfxr library ({0})", path);
        return true;
    }
    hostfxr_initialize_for_runtime_config = (hostfxr_initialize_for_runtime_config_fn)Platform::GetProcAddress(hostfxr, "hostfxr_initialize_for_runtime_config");
    hostfxr_initialize_for_dotnet_command_line = (hostfxr_initialize_for_dotnet_command_line_fn)Platform::GetProcAddress(hostfxr, "hostfxr_initialize_for_dotnet_command_line");
    hostfxr_get_runtime_delegate = (hostfxr_get_runtime_delegate_fn)Platform::GetProcAddress(hostfxr, "hostfxr_get_runtime_delegate");
    hostfxr_close = (hostfxr_close_fn)Platform::GetProcAddress(hostfxr, "hostfxr_close");
    hostfxr_set_error_writer = (hostfxr_set_error_writer_fn)Platform::GetProcAddress(hostfxr, "hostfxr_set_error_writer");
    hostfxr_get_dotnet_environment_info_result = (hostfxr_get_dotnet_environment_info_result_fn)Platform::GetProcAddress(hostfxr, "hostfxr_get_dotnet_environment_info_result");
    hostfxr_run_app = (hostfxr_run_app_fn)Platform::GetProcAddress(hostfxr, "hostfxr_run_app");
    if (!hostfxr_get_runtime_delegate || !hostfxr_run_app)
    {
        LOG(Fatal, "Failed to setup hostfxr API ({0})", path);
        return true;
    }

    // TODO: Implement support for picking RC/beta updates of .NET runtime
    // Uncomment for enabling support for upcoming .NET major release candidates
#if 0
    String dotnetRollForwardPr;
    if (Platform::GetEnvironmentVariable(TEXT("DOTNET_ROLL_FORWARD_TO_PRERELEASE"), dotnetRollForwardPr))
        Platform::SetEnvironmentVariable(TEXT("DOTNET_ROLL_FORWARD_TO_PRERELEASE"), TEXT("1"));
#endif

    // Initialize hosting component
    const char_t* argv[1] = { libraryPath.Get() };
    hostfxr_initialize_parameters init_params;
    init_params.size = sizeof(hostfxr_initialize_parameters);
    init_params.host_path = libraryPath.Get();
    path = String(StringUtils::GetDirectoryName(path)) / TEXT("/../../../");
    StringUtils::PathRemoveRelativeParts(path);
    dotnetRoot = FLAX_CORECLR_STRING(path);
    init_params.dotnet_root = dotnetRoot.Get();
    hostfxr_handle handle = nullptr;
    rc = hostfxr_initialize_for_dotnet_command_line(ARRAY_COUNT(argv), argv, &init_params, &handle);
    if (rc != 0 || handle == nullptr)
    {
        hostfxr_close(handle);
        if (rc == 0x80008096) // FrameworkMissingFailure
        {
            String platformStr;
            switch (PLATFORM_TYPE)
            {
            case PlatformType::Windows:
            case PlatformType::UWP:
                if (PLATFORM_ARCH == ArchitectureType::x64)
                    platformStr = "Windows x64";
                else if (PLATFORM_ARCH == ArchitectureType::ARM64)
                    platformStr = "Windows ARM64";
                else
                    platformStr = "Windows x86";
                break;
            case PlatformType::Linux:
                platformStr = PLATFORM_ARCH_ARM64 ? "Linux ARM64" : PLATFORM_ARCH_ARM ? "Linux Arm32" : PLATFORM_64BITS ? "Linux x64" : "Linux x86";
                break;
            case PlatformType::Mac:
                platformStr = PLATFORM_ARCH_ARM || PLATFORM_ARCH_ARM64 ? "macOS ARM64" : PLATFORM_64BITS ? "macOS x64" : "macOS x86";
                break;
            default:;
                platformStr = "";
            }
            LOG(Fatal, "Failed to resolve compatible .NET runtime version in '{0}'. Make sure the correct platform version for runtime is installed ({1})", platformStr, String(init_params.dotnet_root));
        }
        else
            LOG(Fatal, "Failed to initialize hostfxr: {0:x} ({1})", (unsigned int)rc, String(init_params.dotnet_root));
        return true;
    }

    void* pget_function_pointer = nullptr;
    rc = hostfxr_get_runtime_delegate(handle, hdt_get_function_pointer, &pget_function_pointer);
    if (rc != 0 || pget_function_pointer == nullptr)
    {
        hostfxr_close(handle);
        LOG(Fatal, "Failed to get runtime delegate hdt_get_function_pointer: 0x{0:x}", (unsigned int)rc);
        return true;
    }

    hostfxr_close(handle);
    get_function_pointer = (get_function_pointer_fn)pget_function_pointer;
    return false;
}

void ShutdownHostfxr()
{
}

void* GetStaticMethodPointer(const String& methodName)
{
    void* fun;
    if (CachedFunctions.TryGet(methodName, fun))
        return fun;
    PROFILE_CPU();
    const int rc = get_function_pointer(NativeInteropTypeName, FLAX_CORECLR_STRING(methodName).Get(), UNMANAGEDCALLERSONLY_METHOD, nullptr, nullptr, &fun);
    if (rc != 0)
        LOG(Fatal, "Failed to get unmanaged function pointer for method '{0}': 0x{1:x}", methodName, (unsigned int)rc);
    CachedFunctions.Add(methodName, fun);
    return fun;
}

#elif DOTNET_HOST_MONO

void OnLogCallback(const char* logDomain, const char* logLevel, const char* message, mono_bool fatal, void* userData)
{
    String msg(message);
    msg.Replace('\n', ' ');

    static const char* monoErrorLevels[] =
    {
        nullptr,
        "error",
        "critical",
        "warning",
        "message",
        "info",
        "debug"
    };

    uint32 errorLevel = 0;
    if (logLevel != nullptr)
    {
        for (uint32 i = 1; i < 7; i++)
        {
            if (strcmp(monoErrorLevels[i], logLevel) == 0)
            {
                errorLevel = i;
                break;
            }
        }
    }

#if 0
	// Print C# stack trace (crash may be caused by the managed code)
	if (mono_domain_get() && Assemblies::FlaxEngine.Assembly->IsLoaded())
	{
		const auto managedStackTrace = DebugLog::GetStackTrace();
		if (managedStackTrace.HasChars())
		{
			LOG(Warning, "Managed stack trace:");
			LOG_STR(Warning, managedStackTrace);
		}
	}
#endif

    if (errorLevel <= 2)
    {
        Log::CLRInnerException(String::Format(TEXT("[Mono] {0}"), msg)).SetLevel(LogType::Error);
    }
    else if (errorLevel <= 3)
    {
        LOG(Warning, "[Mono] {0}", msg);
    }
    else
    {
        LOG(Info, "[Mono] {0}", msg);
    }
#if DOTNET_HOST_MONO && !BUILD_RELEASE
    if (errorLevel <= 2)
    {
        // Mono backend ends with fatal assertions so capture crash info (eg. stack trace)
        CRASH;
    }
#endif
}

void OnPrintCallback(const char* string, mono_bool isStdout)
{
    LOG_STR(Warning, String(string));
}

void OnPrintErrorCallback(const char* string, mono_bool isStdout)
{
    // HACK: ignore this message
    if (string && Platform::MemoryCompare(string, "debugger-agent: Unable to listen on ", 36) == 0)
        return;

    LOG_STR(Error, String(string));
}

static MonoAssembly* OnMonoAssemblyLoad(const char* aname)
{
    // Find assembly file
    const String name(aname);
#if DOTNET_HOST_MONO_DEBUG
    LOG(Info, "Loading C# assembly {0}", name);
#endif
    String fileName = name;
    if (!name.EndsWith(TEXT(".dll")) && !name.EndsWith(TEXT(".exe")))
        fileName += TEXT(".dll");
    String path = fileName;
    if (!FileSystem::FileExists(path))
    {
        path = Globals::ProjectFolder / String(TEXT("/Dotnet/shared/Microsoft.NETCore.App/")) / fileName;
        if (!FileSystem::FileExists(path))
        {
            path = Globals::ProjectFolder / String(TEXT("/Dotnet/")) / fileName;
        }
    }

    // Load assembly
#if DOTNET_HOST_MONO_DEBUG
    LOG(Info, "Loading C# assembly from path = {0}, exist = {1}", path, FileSystem::FileExists(path));
#endif
    MonoAssembly* assembly = nullptr;
    if (FileSystem::FileExists(path))
    {
        StringAnsi pathAnsi(path);
        assembly = mono_assembly_open(pathAnsi.Get(), nullptr);
    }
    if (!assembly)
    {
        LOG(Error, "Failed to load C# assembly {0}", path);
    }
    return assembly;
}

static MonoAssembly* OnMonoAssemblyPreloadHook(MonoAssemblyName* aname, char** assemblies_path, void* user_data)
{
    return OnMonoAssemblyLoad(mono_assembly_name_get_name(aname));
}

#if 0

static unsigned char* OnMonoLoadAOT(MonoAssembly* assembly, int size, void* user_data, void** out_handle)
{
    MonoAssemblyName* assemblyName = mono_assembly_get_name(assembly);
    const char* assemblyNameStr = mono_assembly_name_get_name(assemblyName);
#if DOTNET_HOST_MONO_DEBUG
    LOG(Info, "Loading AOT data for C# assembly {0}", String(assemblyNameStr));
#endif
    return nullptr;
}

static void OnMonoFreeAOT(MonoAssembly* assembly, int size, void* user_data, void* handle)
{
#if DOTNET_HOST_MONO_DEBUG
    MonoAssemblyName* assemblyName = mono_assembly_get_name(assembly);
    const char* assemblyNameStr = mono_assembly_name_get_name(assemblyName);
    LOG(Info, "Free AOT data for C# assembly {0}", String(assemblyNameStr));
#endif
}

#endif

#if PLATFORM_IOS

#include "Engine/Engine/Globals.h"
#include <mono/utils/mono-dl-fallback.h>
#include <dlfcn.h>

static void* OnMonoDlFallbackLoad(const char* name, int flags, char** err, void* user_data)
{
    const String fileName = StringUtils::GetFileName(String(name));
#if DOTNET_HOST_MONO_DEBUG
    LOG(Info, "Loading dynamic library {0}", fileName);
#endif
    int dlFlags = 0;
    if (flags & MONO_DL_GLOBAL && !(flags & MONO_DL_LOCAL))
		dlFlags |= RTLD_GLOBAL;
	else
		dlFlags |= RTLD_LOCAL;
	if (flags & MONO_DL_LAZY)
		dlFlags |= RTLD_LAZY;
	else
		dlFlags |= RTLD_NOW;
    void* result = dlopen(name, dlFlags);
    if (!result)
    {
        // Try Frameworks location on iOS
        String path = Globals::ProjectFolder / TEXT("Frameworks") / fileName;
        if (!path.EndsWith(TEXT(".dylib")))
            path += TEXT(".dylib");
        result = dlopen(StringAsANSI<>(*path).Get(), dlFlags);
        if (!result)
        {
            LOG(Error, "Failed to load dynamic libary {0}", String(name));
        }
    }
    return result;
}

static void* OnMonoDlFallbackSymbol(void* handle, const char* name, char** err, void* user_data)
{
	return dlsym(handle, name);
}

static void* OnMonoDlFallbackClose(void* handle, void* user_data)
{
	dlclose(handle);
    return 0;
}

#endif

bool InitHostfxr()
{
#if DOTNET_HOST_MONO_DEBUG
    // Enable detailed Mono logging
    Platform::SetEnvironmentVariable(TEXT("MONO_LOG_LEVEL"), TEXT("debug"));
    Platform::SetEnvironmentVariable(TEXT("MONO_LOG_MASK"), TEXT("all"));
    //Platform::SetEnvironmentVariable(TEXT("MONO_GC_DEBUG"), TEXT("6:gc-log.txt,check-remset-consistency,nursery-canaries"));
#endif

    // Adjust GC threads suspending mode to not block attached native threads (eg. Job System)
    Platform::SetEnvironmentVariable(TEXT("MONO_THREADS_SUSPEND"), TEXT("preemptive"));

#if defined(USE_MONO_AOT_MODE)
    // Enable AOT mode (per-platform)
    mono_jit_set_aot_mode(USE_MONO_AOT_MODE);
#endif
    
    // Platform-specific setup
#if PLATFORM_IOS || PLATFORM_SWITCH
    setenv("MONO_AOT_MODE", "aot", 1);
    setenv("DOTNET_SYSTEM_GLOBALIZATION_INVARIANT", "1", 1);
#endif

#ifdef USE_MONO_AOT_MODULE
    // Load AOT module
    Stopwatch aotModuleLoadStopwatch;
    LOG(Info, "Loading Mono AOT module...");
    void* libAotModule = Platform::LoadLibrary(TEXT(USE_MONO_AOT_MODULE));
    if (libAotModule == nullptr)
    {
        LOG(Error, "Failed to laod Mono AOT module (" TEXT(USE_MONO_AOT_MODULE) ")");
        return true;
    }
    MonoAotModuleHandle = libAotModule;
    void* getModulesPtr = Platform::GetProcAddress(libAotModule, "GetMonoModules");
    if (getModulesPtr == nullptr)
    {
        LOG(Error, "Failed to get Mono AOT modules getter.");
        return true;
    }
    typedef int (*GetMonoModulesFunc)(void** buffer, int bufferSize);
    const auto getModules = (GetMonoModulesFunc)getModulesPtr;
    const int32 moduelsCount = getModules(nullptr, 0);
    void** modules = (void**)Allocator::Allocate(moduelsCount * sizeof(void*));
    getModules(modules, moduelsCount);
    for (int32 i = 0; i < moduelsCount; i++)
    {
        mono_aot_register_module((void**)modules[i]);
    }
    Allocator::Free(modules);
    aotModuleLoadStopwatch.Stop();
    LOG(Info, "Mono AOT module loaded in {0}ms", aotModuleLoadStopwatch.GetMilliseconds());
#endif

    // Setup debugger
    {
        int32 debuggerLogLevel = 0;
        if (CommandLine::Options.MonoLog.IsTrue() || DOTNET_HOST_MONO_DEBUG)
        {
            LOG(Info, "Using detailed Mono logging");
            mono_trace_set_level_string("debug");
            debuggerLogLevel = 10;
        }
        else
        {
            mono_trace_set_level_string("warning");
        }

#if MONO_DEBUG_ENABLE && !PLATFORM_SWITCH
        StringAnsi debuggerIp = "127.0.0.1";
        uint16 debuggerPort = 41000 + Platform::GetCurrentProcessId() % 1000;
        if (CommandLine::Options.DebuggerAddress.HasValue())
        {
            const auto& address = CommandLine::Options.DebuggerAddress.GetValue();
            const int32 splitIndex = address.Find(':');
            if (splitIndex == INVALID_INDEX)
            {
                debuggerIp = address.ToStringAnsi();
            }
            else
            {
                debuggerIp = address.Left(splitIndex).ToStringAnsi();
                StringUtils::Parse(address.Right(address.Length() - splitIndex - 1).Get(), &debuggerPort);
            }
        }

        char buffer[150];
        sprintf(buffer, "--debugger-agent=transport=dt_socket,address=%s:%d,embedding=1,server=y,suspend=%s,loglevel=%d", debuggerIp.Get(), debuggerPort, CommandLine::Options.WaitForDebugger ? "y,timeout=5000" : "n", debuggerLogLevel);

        const char* options[] = {
            "--soft-breakpoints",
            //"--optimize=float32",
            buffer
        };
        mono_jit_parse_options(ARRAY_COUNT(options), (char**)options);

        mono_debug_init(MONO_DEBUG_FORMAT_MONO, 0);
        LOG(Info, "Mono debugger server at {0}:{1}", ::String(debuggerIp), debuggerPort);
#endif
    }

    // Connect to mono engine callback system
    mono_trace_set_log_handler(OnLogCallback, nullptr);
    mono_trace_set_print_handler(OnPrintCallback);
    mono_trace_set_printerr_handler(OnPrintErrorCallback);

    // Initialize Mono VM
    StringAnsi baseDirectory(Globals::ProjectFolder);
    const char* appctxKeys[] =
    {
        "RUNTIME_IDENTIFIER",
        "APP_CONTEXT_BASE_DIRECTORY",
    };
    const char* appctxValues[] =
    {
        MACRO_TO_STR(DOTNET_HOST_RUNTIME_IDENTIFIER),
        baseDirectory.Get(),
    };
    static_assert(ARRAY_COUNT(appctxKeys) == ARRAY_COUNT(appctxValues), "Invalid appctx setup");
    monovm_initialize(ARRAY_COUNT(appctxKeys), appctxKeys, appctxValues);
    mono_install_assembly_preload_hook(OnMonoAssemblyPreloadHook, nullptr);
#if 0
    mono_install_load_aot_data_hook(OnMonoLoadAOT, OnMonoFreeAOT, nullptr);
#endif
#if PLATFORM_IOS
    mono_dl_fallback_register(OnMonoDlFallbackLoad, OnMonoDlFallbackSymbol, OnMonoDlFallbackClose, nullptr);
#endif

    // Init managed runtime
#if PLATFORM_ANDROID || PLATFORM_IOS
    const char* monoVersion = "mobile";
#else
    const char* monoVersion = ""; // ignored
#endif
    MonoDomainHandle = mono_jit_init_version("Flax", monoVersion);
    if (!MonoDomainHandle)
    {
        LOG(Fatal, "Failed to initialize Mono.");
        return true;
    }
    mono_gc_init_finalizer_thread();

    // Log info
    char* buildInfo = mono_get_runtime_build_info();
    LOG(Info, "Mono runtime version: {0}", String(buildInfo));
    mono_free(buildInfo);

    return false;
}

void ShutdownHostfxr()
{
    mono_jit_cleanup(MonoDomainHandle);
    MonoDomainHandle = nullptr;

#ifdef USE_MONO_AOT_MODULE
    Platform::FreeLibrary(MonoAotModuleHandle);
#endif
}

void* GetStaticMethodPointer(const String& methodName)
{
    void* fun;
    if (CachedFunctions.TryGet(methodName, fun))
        return fun;
    PROFILE_CPU();

    static MonoClass* nativeInteropClass = nullptr;
    if (!nativeInteropClass)
    {
        const char* assemblyName = "FlaxEngine.CSharp";
        const char* className = "FlaxEngine.Interop.NativeInterop";
        MonoAssembly* flaxEngineAssembly = OnMonoAssemblyLoad(assemblyName);
        ASSERT(flaxEngineAssembly);
        MonoType* interopTyp = mono_reflection_type_from_name((char*)className, mono_assembly_get_image(flaxEngineAssembly));
        ASSERT(interopTyp);
        nativeInteropClass = mono_class_from_mono_type(interopTyp);
        ASSERT(nativeInteropClass);
    }
    const StringAsUTF8<40> methodNameAnsi(methodName.Get(), methodName.Length());
    MonoMethod* method = mono_class_get_method_from_name(nativeInteropClass, methodNameAnsi.Get(), -1);
    ASSERT(method);

    MonoError error;
    mono_error_init(&error);
    fun = mono_method_get_unmanaged_callers_only_ftnptr(method, &error);
    if (fun == nullptr)
    {
        const unsigned short errorCode = mono_error_get_error_code(&error);
        const char* errorMessage = mono_error_get_message(&error);
        LOG(Fatal, "Failed to get unmanaged function pointer for method '{0}': 0x{1:x}, {2}", methodName, errorCode, String(errorMessage));
    }
    mono_error_cleanup(&error);

    CachedFunctions.Add(methodName, fun);
    return fun;
}

#endif

#endif
