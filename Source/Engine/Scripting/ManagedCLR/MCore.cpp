// Copyright (c) Wojciech Figat. All rights reserved.

#include "MCore.h"
#include "MAssembly.h"
#include "MClass.h"
#include "MEvent.h"
#include "MDomain.h"
#include "MException.h"
#include "MMethod.h"
#include "MProperty.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Core/Types/DateTime.h"
#include "Engine/Core/Types/Stopwatch.h"
#include "Engine/Core/Types/TimeSpan.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Debug/Exceptions/FileNotFoundException.h"
#include "Engine/Debug/Exceptions/InvalidOperationException.h"

MDomain* MRootDomain = nullptr;
MDomain* MActiveDomain = nullptr;
Array<MDomain*, FixedAllocation<4>> MDomains;

MClass* MCore::TypeCache::Void = nullptr;
MClass* MCore::TypeCache::Object = nullptr;
MClass* MCore::TypeCache::Byte = nullptr;
MClass* MCore::TypeCache::Boolean = nullptr;
MClass* MCore::TypeCache::SByte = nullptr;
MClass* MCore::TypeCache::Char = nullptr;
MClass* MCore::TypeCache::Int16 = nullptr;
MClass* MCore::TypeCache::UInt16 = nullptr;
MClass* MCore::TypeCache::Int32 = nullptr;
MClass* MCore::TypeCache::UInt32 = nullptr;
MClass* MCore::TypeCache::Int64 = nullptr;
MClass* MCore::TypeCache::UInt64 = nullptr;
MClass* MCore::TypeCache::IntPtr = nullptr;
MClass* MCore::TypeCache::UIntPtr = nullptr;
MClass* MCore::TypeCache::Single = nullptr;
MClass* MCore::TypeCache::Double = nullptr;
MClass* MCore::TypeCache::String = nullptr;

MAssembly::MAssembly(MDomain* domain, const StringAnsiView& name)
    : _domain(domain)
    , _isLoaded(false)
    , _isLoading(false)
    , _hasCachedClasses(false)
    , _reloadCount(0)
    , _name(name)
{
}

#if USE_NETCORE

MAssembly::MAssembly(MDomain* domain, const StringAnsiView& name, const StringAnsiView& fullname, void* handle)
    : _handle(handle)
    , _fullname(fullname)
    , _domain(domain)
    , _isLoaded(false)
    , _isLoading(false)
    , _hasCachedClasses(false)
    , _reloadCount(0)
    , _name(name)
{
}

#endif

MAssembly::~MAssembly()
{
    Unload();
}

String MAssembly::ToString() const
{
    return _name.ToString();
}

bool MAssembly::Load(const String& assemblyPath, const StringView& nativePath)
{
    if (IsLoaded())
        return false;
    PROFILE_CPU();
    ZoneText(*assemblyPath, assemblyPath.Length());
    Stopwatch stopwatch;

    const String* pathPtr = &assemblyPath;
    String path;
    if (!FileSystem::FileExists(assemblyPath))
    {
        path = assemblyPath;
        pathPtr = &path;
        if (ResolveMissingFile(path))
        {
            Log::FileNotFoundException ex(assemblyPath);
            return true;
        }
    }

    OnLoading();

    if (LoadImage(*pathPtr, nativePath))
    {
        OnLoadFailed();
        return true;
    }

    OnLoaded(stopwatch);
    return false;
}

void MAssembly::Unload(bool isReloading)
{
    if (!IsLoaded())
        return;
    PROFILE_CPU();

    Unloading(this);

    // Close runtime
    UnloadImage(isReloading);

    // Cleanup
    _debugData.Resize(0);
    _assemblyPath.Clear();
    _isLoading = false;
    _isLoaded = false;
    _hasCachedClasses = false;
    _classes.ClearDelete();

    Unloaded(this);
}

MClass* MAssembly::GetClass(const StringAnsiView& fullname) const
{
    // Check state
    if (!IsLoaded())
    {
        Log::InvalidOperationException(TEXT("MAssembly was not yet loaded or loading was in progress"));
        return nullptr;
    }

    StringAnsiView key(fullname);

    // Special case for reference
    if (fullname[fullname.Length() - 1] == '&')
        key = StringAnsiView(key.Get(), key.Length() - 1);

    // Find class by name
    const auto& classes = GetClasses();
    MClass* result = nullptr;
    classes.TryGet(key, result);

#if 0
    if (!result)
    {
        LOG(Warning, "Failed to find class {0} in assembly {1}. Classes:", String(fullname), ToString());
        for (auto i = classes.Begin(); i.IsNotEnd(); ++i)
        {
            LOG(Warning, " - {0}", String(i->Key));
        }
    }
#endif
    return result;
}

void MAssembly::OnLoading()
{
    Loading(this);

    _isLoading = true;

    // Pick a domain
    if (_domain == nullptr)
        _domain = MCore::GetActiveDomain();
}

void MAssembly::OnLoaded(Stopwatch& stopwatch)
{
    // Register in domain
    _domain->_assemblies[_name] = this;

    _isLoaded = true;
    _isLoading = false;

    stopwatch.Stop();
    LOG(Info, "Assembly {0} loaded in {1}ms", String(_name), stopwatch.GetMilliseconds());

    // Pre-cache classes
    GetClasses();

    Loaded(this);
}

void MAssembly::OnLoadFailed()
{
    _isLoading = false;

    LoadFailed(this);
}

MEvent* MClass::GetEvent(const char* name) const
{
    GetEvents();
    for (int32 i = 0; i < _events.Count(); i++)
    {
        if (_events[i]->GetName() == name)
            return _events[i];
    }
    return nullptr;
}

MObject* MClass::CreateInstance() const
{
    MObject* obj = MCore::Object::New(this);
    if (!IsValueType())
        MCore::Object::Init(obj);
    return obj;
}

MType* MEvent::GetType() const
{
    if (GetAddMethod() != nullptr)
        return GetAddMethod()->GetReturnType();
    if (GetRemoveMethod() != nullptr)
        return GetRemoveMethod()->GetReturnType();
    return nullptr;
}

void MException::Log(const LogType type, const Char* target)
{
    // Log inner exceptions chain
    MException* inner = InnerException;
    while (inner)
    {
        const Char* stackTrace = inner->StackTrace.HasChars() ? *inner->StackTrace : TEXT("<empty>");
        Log::Logger::Write(LogType::Warning, String::Format(TEXT("Inner exception. {0}\nStack strace:\n{1}\n"), inner->Message, stackTrace));
        inner = inner->InnerException;
    }

    // Send stack trace only to log file
    const Char* stackTrace = StackTrace.HasChars() ? *StackTrace : TEXT("<empty>");
    const String info = target && *target ? String::Format(TEXT("Exception has been thrown during {0}."), target) : TEXT("Exception has been thrown.");
    Log::Logger::Write(LogType::Warning, String::Format(TEXT("{0} {1}\nStack strace:\n{2}"), info, Message, stackTrace));
    Log::Logger::Write(type, String::Format(TEXT("{0}\n{1}"), info, Message));
}

MType* MProperty::GetType() const
{
    if (GetGetMethod() != nullptr)
        return GetGetMethod()->GetReturnType();
    return GetSetMethod()->GetReturnType();
}

MVisibility MProperty::GetVisibility() const
{
    if (GetGetMethod() && GetSetMethod())
    {
        return static_cast<MVisibility>(
            Math::Max(
                static_cast<int>(GetGetMethod()->GetVisibility()),
                static_cast<int>(GetSetMethod()->GetVisibility())
            ));
    }
    if (GetGetMethod())
    {
        return GetGetMethod()->GetVisibility();
    }
    return GetSetMethod()->GetVisibility();
}

bool MProperty::IsStatic() const
{
    if (GetGetMethod())
    {
        return GetGetMethod()->IsStatic();
    }
    if (GetSetMethod())
    {
        return GetSetMethod()->IsStatic();
    }
    return false;
}

MDomain* MCore::GetRootDomain()
{
    return MRootDomain;
}

MDomain* MCore::GetActiveDomain()
{
    return MActiveDomain;
}
