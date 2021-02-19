// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "MAssembly.h"

#if USE_MONO

#include "MClass.h"
#include "MDomain.h"
#include "MUtils.h"
#include "Engine/Core/Log.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Debug/Exceptions/InvalidOperationException.h"
#include "Engine/Debug/Exceptions/FileNotFoundException.h"
#include "Engine/Debug/Exceptions/CLRInnerException.h"
#include "Engine/Scripting/ManagedCLR/MCore.h"
#include "Engine/Scripting/Scripting.h"
#include "Engine/Platform/StringUtils.h"
#include "Engine/Platform/File.h"
#include <ThirdParty/mono-2.0/mono/metadata/mono-debug.h>
#include <ThirdParty/mono-2.0/mono/metadata/assembly.h>
#include <ThirdParty/mono-2.0/mono/metadata/tokentype.h>

MAssembly::MAssembly(MDomain* domain, const StringAnsiView& name, const MAssemblyOptions& options)
    : _monoAssembly(nullptr)
    , _monoImage(nullptr)
    , _domain(domain)
    , _isLoaded(false)
    , _isLoading(false)
    , _isDependency(false)
    , _isFileLocked(false)
    , _hasCachedClasses(false)
    , _reloadCount(0)
    , _name(name)
    , _options(options)
{
}

MAssembly::~MAssembly()
{
    Unload();
}

String MAssembly::ToString() const
{
    return _name.ToString();
}

bool MAssembly::Load(const String& assemblyPath)
{
    // Skip if already loaded
    if (IsLoaded())
        return false;

    // Check file path
    if (!FileSystem::FileExists(assemblyPath))
    {
        Log::FileNotFoundException ex(assemblyPath);
        return true;
    }

    // Start
    const auto startTime = DateTime::NowUTC();
    OnLoading();

    // Load
    bool failed;
    if (_options.KeepManagedFileLocked)
        failed = LoadDefault(assemblyPath);
    else
        failed = LoadWithImage(assemblyPath);
    if (failed)
    {
        OnLoadFailed();
        return true;
    }

    // End
    OnLoaded(startTime);
    return false;
}

bool MAssembly::Load(MonoImage* monoImage)
{
    // Skip if already loaded
    if (IsLoaded())
        return false;

    // Ensure to be unloaded
    Unload();

    // Start
    const auto startTime = DateTime::NowUTC();
    OnLoading();

    // Load
    _monoAssembly = mono_image_get_assembly(monoImage);
    if (_monoAssembly == nullptr)
    {
        OnLoadFailed();
        return true;
    }
    _monoImage = monoImage;
    _isDependency = true;
    _hasCachedClasses = false;

    // End
    OnLoaded(startTime);
    return false;
}

void MAssembly::Unload(bool isReloading)
{
    if (!IsLoaded())
        return;

    Unloading(this);

    // Close runtime
    if (_monoImage)
    {
        if (isReloading)
        {
            LOG(Info, "Unloading managed assembly \'{0}\' (is reloading)", String(_name));

            mono_assembly_close(_monoAssembly);
            mono_image_close(_monoImage);
        }
        else
        {
            // NOTE: do not try to close all the opened images
            //       that will cause the domain unload to crash because
            //       the images have already been closed (double free)
        }

        _monoAssembly = nullptr;
        _monoImage = nullptr;
    }

    // Cleanup
    _debugData.Resize(0);
    _assemblyPath.Clear();
    _isFileLocked = false;
    _isDependency = false;
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

MClass* MAssembly::GetClass(MonoClass* monoClass) const
{
    if (monoClass == nullptr || !IsLoaded() || mono_class_get_image(monoClass) != _monoImage)
        return nullptr;

    // Find class by native pointer
    const auto& classes = GetClasses();
    const auto typeToken = mono_class_get_type_token(monoClass);
    for (auto i = classes.Begin(); i.IsNotEnd(); ++i)
    {
        MonoClass* e = i->Value->GetNative();
        if (e == monoClass || mono_class_get_type_token(e) == typeToken)
        {
            return i->Value;
        }
    }

#if 0
    {
        LOG(Warning, "Failed to find class {0}.{1} in assembly {2}. Classes:", String(mono_class_get_namespace(monoClass)), String(mono_class_get_name(monoClass)), ToString());
        for (auto i = classes.Begin(); i.IsNotEnd(); ++i)
        {
            LOG(Warning, " - {0}", String(i->Key));
        }
    }
#endif
    return nullptr;
}

MonoReflectionAssembly* MAssembly::GetNative() const
{
    if (!_monoAssembly)
        return nullptr;
    return mono_assembly_get_object(mono_domain_get(), _monoAssembly);
}

const MAssembly::ClassesDictionary& MAssembly::GetClasses() const
{
    if (_hasCachedClasses || !IsLoaded())
        return _classes;
    ScopeLock lock(_locker);
    if (_hasCachedClasses)
        return _classes;
    ASSERT(_classes.IsEmpty());

    const auto startTime = DateTime::NowUTC();

    const int32 numRows = mono_image_get_table_rows(_monoImage, MONO_TABLE_TYPEDEF);
    _classes.EnsureCapacity(numRows * 4);
    for (int32 i = 1; i < numRows; i++) // Skip <Module> class
    {
        MonoClass* klass = mono_class_get(_monoImage, (i + 1) | MONO_TOKEN_TYPE_DEF);

        // Peek the typename
        MString fullname;
        MUtils::GetClassFullname(klass, fullname);

        // Create class object
        auto mclass = New<MClass>(this, klass, fullname);
        _classes.Add(fullname, mclass);
    }

    const auto endTime = DateTime::NowUTC();
    LOG(Info, "Caching classes for assembly {0} took {1}ms", String(_name), (int32)(endTime - startTime).GetTotalMilliseconds());

#if 0
    for (auto i = _classes.Begin(); i.IsNotEnd(); ++i)
        LOG(Info, "Class: {0}", String(i->Value->GetFullName()));
#endif

    _hasCachedClasses = true;
    return _classes;
}

bool MAssembly::LoadDefault(const String& assemblyPath)
{
    // With this method of loading we need to make sure, we won't try to load assembly again if its loaded somewhere.
    auto assembly = mono_domain_assembly_open(_domain->GetNative(), assemblyPath.ToStringAnsi().Get());
    if (!assembly)
    {
        return true;
    }
    auto assemblyImage = mono_assembly_get_image(assembly);
    if (!assembly)
    {
        mono_assembly_close(assembly);
        return true;
    }

    // Register in domain
    _domain->_assemblies[_name] = this;

    // Set state
    _monoAssembly = assembly;
    _monoImage = assemblyImage;
    _isDependency = false;
    _hasCachedClasses = false;
    _isFileLocked = true;
    _assemblyPath = assemblyPath;

    return false;
}

bool MAssembly::LoadWithImage(const String& assemblyPath)
{
    // Lock file only for loading operation
    _isFileLocked = true;

    // Load assembly file data
    Array<byte> data;
    File::ReadAllBytes(assemblyPath, data);

    // Init Mono image
    MonoImageOpenStatus status;
    const auto name = assemblyPath.ToStringAnsi();
    const auto assemblyImage = mono_image_open_from_data_with_name(reinterpret_cast<char*>(data.Get()), data.Count(), true, &status, false, name.Get());
    if (status != MONO_IMAGE_OK || assemblyImage == nullptr)
    {
        Log::CLRInnerException(TEXT("Mono assembly image is invalid at ") + assemblyPath);
        return true;
    }

    // Setup assembly
    const auto assembly = mono_assembly_load_from_full(assemblyImage, name.Substring(0, name.Length() - 3).Get(), &status, false);
    if (status != MONO_IMAGE_OK || assembly == nullptr)
    {
        // Close image if error occurred
        mono_image_close(assemblyImage);

        Log::CLRInnerException(TEXT("Mono assembly image is corrupted at ") + assemblyPath);
        return true;
    }

#if MONO_DEBUG_ENABLE
    // Try to load debug symbols (use portable PDB format)
    const auto pdbPath = StringUtils::GetPathWithoutExtension(assemblyPath) + TEXT(".pdb");
    if (FileSystem::FileExists(pdbPath))
    {
        // Load .pdb file
        File::ReadAllBytes(pdbPath, _debugData);

        // Attach debugging symbols to image
        if (_debugData.HasItems())
        {
            mono_debug_open_image_from_memory(assemblyImage, _debugData.Get(), _debugData.Count());
        }
    }
#endif

    // Set state
    _monoAssembly = assembly;
    _monoImage = assemblyImage;
    _isDependency = false;
    _hasCachedClasses = false;
    _isFileLocked = false;
    _assemblyPath = assemblyPath;

    return false;
}

void MAssembly::OnLoading()
{
    Loading(this);

    _isLoading = true;
    if (_classes.Capacity() == 0)
        _classes.EnsureCapacity(_options.DictionaryInitialSize);

    // Pick a domain
    if (_domain == nullptr)
        _domain = MCore::Instance()->GetActiveDomain();
}

void MAssembly::OnLoaded(const DateTime& startTime)
{
    // Register in domain
    _domain->_assemblies[_name] = this;

    _isLoaded = true;
    _isLoading = false;

    if (_options.PreCacheOnLoad)
        GetClasses();

    const auto endTime = DateTime::NowUTC();
    LOG(Info, "Assembly {0} loaded in {1}ms", String(_name), (int32)(endTime - startTime).GetTotalMilliseconds());

    Loaded(this);
}

void MAssembly::OnLoadFailed()
{
    _isLoading = false;

    LoadFailed(this);
}

#endif
