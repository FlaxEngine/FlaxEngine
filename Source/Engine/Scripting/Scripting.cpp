// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "BinaryModule.h"
#include "Scripting.h"
#include "StdTypesContainer.h"
#include "ScriptingType.h"
#include "FlaxEngine.Gen.h"
#include "Engine/Threading/Threading.h"
#include "Engine/Threading/IRunnable.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Platform/File.h"
#include "Engine/Debug/DebugLog.h"
#if USE_EDITOR
#include "Editor/Scripting/ScriptsBuilder.h"
#endif
#include "ManagedCLR/MAssembly.h"
#include "ManagedCLR/MClass.h"
#include "ManagedCLR/MMethod.h"
#include "ManagedCLR/MDomain.h"
#include "ManagedCLR/MCore.h"
#include "MException.h"
#include "Engine/Level/Level.h"
#include "Engine/Core/ObjectsRemovalService.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Content/Asset.h"
#include "Engine/Content/Content.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Serialization/JsonTools.h"
#include "Engine/Utilities/StringConverter.h"
#include <ThirdParty/mono-2.0/mono/metadata/mono-debug.h>
#include <ThirdParty/mono-2.0/mono/metadata/object.h>

extern void registerFlaxEngineInternalCalls();

class ScriptingService : public EngineService
{
public:

    ScriptingService()
        : EngineService(TEXT("Scripting"), -20)
    {
    }

    bool Init() override;
    void Update() override;
    void LateUpdate() override;
    void FixedUpdate() override;
    void Draw() override;
    void BeforeExit() override;
    void Dispose() override;
};

namespace
{
    MDomain* _monoRootDomain = nullptr;
    MDomain* _monoScriptsDomain = nullptr;
    CriticalSection _objectsLocker;
#define USE_OBJECTS_DISPOSE_CRASHES_DEBUGGING 0
#if USE_OBJECTS_DISPOSE_CRASHES_DEBUGGING
    struct ScriptingObjectData
    {
        ScriptingObject* Ptr;
        StringAnsi TypeName;

        ScriptingObjectData()
        {
            Ptr = nullptr;
        }

        ScriptingObjectData(ScriptingObject* ptr)
        {
            Ptr = ptr;
            if (ptr && ptr->GetTypeHandle() && ptr->GetTypeHandle().TypeIndex < ptr->GetTypeHandle().Module->Types.Count())
                TypeName = ptr->GetType().Fullname;
        }

        ScriptingObject* operator->() const
        {
            return Ptr;
        }

        explicit operator ScriptingObject*()
        {
            return Ptr;
        }

        operator ScriptingObject*() const
        {
            return Ptr;
        }
    };

    Dictionary<Guid, ScriptingObjectData> _objectsDictionary(1024 * 16);
#else
    Dictionary<Guid, ScriptingObject*> _objectsDictionary(1024 * 16);
#endif
    bool _isEngineAssemblyLoaded = false;
    bool _hasGameModulesLoaded = false;
    MMethod* _method_Update = nullptr;
    MMethod* _method_LateUpdate = nullptr;
    MMethod* _method_FixedUpdate = nullptr;
    MMethod* _method_Draw = nullptr;
    MMethod* _method_Exit = nullptr;
    Array<BinaryModule*, InlinedAllocation<64>> _nonNativeModules;
}

Delegate<BinaryModule*> Scripting::BinaryModuleLoaded;
Action Scripting::ScriptsLoaded;
Action Scripting::ScriptsUnload;
Action Scripting::ScriptsReloading;
Action Scripting::ScriptsReloaded;
ThreadLocal<Scripting::IdsMappingTable*, 32> Scripting::ObjectsLookupIdMapping;
ScriptingService ScriptingServiceInstance;

bool initFlaxEngine();

// Assembly events
void onEngineLoaded(MAssembly* assembly);
void onEngineUnloading(MAssembly* assembly);

bool ScriptingService::Init()
{
    const auto startTime = DateTime::NowUTC();

    // Link for assemblies events
    auto engineAssembly = GetBinaryModuleFlaxEngine()->Assembly;
    engineAssembly->Loaded.Bind(onEngineLoaded);
    engineAssembly->Unloading.Bind(onEngineUnloading);

    // Initialize managed runtime
    if (MCore::Instance()->LoadEngine())
    {
        LOG(Fatal, "Mono initialization failed.");
        return true;
    }

    // Cache root domain
    _monoRootDomain = MCore::Instance()->GetRootDomain();

#if USE_SINGLE_DOMAIN
    // Use single root domain
    auto domain = _monoRootDomain;
#else
	// Create Mono domain for scripts
	auto domain = MCore::Instance()->CreateDomain(TEXT("Scripts Domain"));
#endif
    domain->SetCurrentDomain(true);
    _monoScriptsDomain = domain;

    // Add internal calls
    registerFlaxEngineInternalCalls();

    // Load assemblies
    if (Scripting::Load())
    {
        LOG(Fatal, "Scripting Engine initialization failed.");
        return true;
    }

    auto endTime = DateTime::NowUTC();
    LOG(Info, "Scripting Engine initializated! (time: {0}ms)", (int32)((endTime - startTime).GetTotalMilliseconds()));

    return false;
}

#define INVOKE_EVENT(name) \
    if (!_isEngineAssemblyLoaded) return; \
	if (_method_##name == nullptr) \
	{ \
        MClass* mclass = Scripting::GetStaticClass(); \
		if (mclass == nullptr) \
		{ \
			LOG(Fatal, "Missing Scripting class."); \
			return; \
		} \
		_method_##name = mclass->GetMethod("Internal_" #name, 0); \
		if (_method_##name == nullptr) \
		{ \
			LOG(Fatal, "Missing Scripting method " #name "."); \
			return; \
		} \
	} \
	MonoObject* exception = nullptr; \
	_method_##name->Invoke(nullptr, nullptr, &exception); \
	DebugLog::LogException(exception)

void ScriptingService::Update()
{
    PROFILE_CPU_NAMED("Scripting.Update");

    INVOKE_EVENT(Update);
}

void ScriptingService::LateUpdate()
{
    PROFILE_CPU_NAMED("Scripting.LateUpdate");

    INVOKE_EVENT(LateUpdate);
}

void ScriptingService::FixedUpdate()
{
    PROFILE_CPU_NAMED("Scripting.FixedUpdate");

    INVOKE_EVENT(FixedUpdate);
}

void ScriptingService::Draw()
{
    PROFILE_CPU_NAMED("Scripting.Draw");

    INVOKE_EVENT(Draw);
}

void ScriptingService::BeforeExit()
{
    PROFILE_CPU_NAMED("Scripting.BeforeExit");

    INVOKE_EVENT(Exit);
}

MDomain* Scripting::GetRootDomain()
{
    return _monoRootDomain;
}

MonoDomain* Scripting::GetMonoScriptsDomain()
{
    ASSERT(_monoScriptsDomain != nullptr);
    return _monoScriptsDomain->GetNative();
}

MDomain* Scripting::GetScriptsDomain()
{
    return _monoScriptsDomain;
}

void Scripting::ProcessBuildInfoPath(String& path, const String& projectFolderPath)
{
    if (path.IsEmpty())
        return;
    if (path.StartsWith(TEXT("$(EnginePath)")))
        path = Globals::StartupFolder / path.Substring(14);
    else if (path.StartsWith(TEXT("$(ProjectPath)")))
        path = projectFolderPath / path.Substring(14);
    else if (FileSystem::IsRelative(path))
        path = projectFolderPath / path;
}

bool Scripting::LoadBinaryModules(const String& path, const String& projectFolderPath)
{
    PROFILE_CPU_NAMED("LoadBinaryModules");
    LOG(Info, "Loading binary modules from build info file {0}", path);

    // Read file contents
    Array<byte> fileData;
    if (File::ReadAllBytes(path, fileData))
    {
        LOG(Error, "Failed to read file contents.");
        return true;
    }

    // Parse Json data
    rapidjson_flax::Document document;
    document.Parse((char*)fileData.Get(), fileData.Count());
    if (document.HasParseError())
    {
        LOG(Error, "Failed to file contents.");
        return true;
    }

    // TODO: validate Name, Platform, Architecture, Configuration from file

    // Load all references
    auto referencesMember = document.FindMember("References");
    if (referencesMember != document.MemberEnd())
    {
        auto& referencesArray = referencesMember->value;
        ASSERT(referencesArray.IsArray());
        for (rapidjson::SizeType i = 0; i < referencesArray.Size(); i++)
        {
            auto& reference = referencesArray[i];
            String referenceProjectPath = JsonTools::GetString(reference, "ProjectPath", String::Empty);
            if (referenceProjectPath == TEXT("$(EnginePath)/Flax.flaxproj"))
                continue; // Skip reference to engine
            String referencePath = JsonTools::GetString(reference, "Path", String::Empty);
            if (referenceProjectPath.IsEmpty() || referencePath.IsEmpty())
            {
                LOG(Error, "Empty reference.");
                return true;
            }

            ProcessBuildInfoPath(referenceProjectPath, projectFolderPath);
            ProcessBuildInfoPath(referencePath, projectFolderPath);

            String referenceProjectFolderPath = StringUtils::GetDirectoryName(referenceProjectPath);

            if (LoadBinaryModules(referencePath, referenceProjectFolderPath))
            {
                LOG(Error, "Failed to load reference.");
                return true;
            }
        }
    }

    // Load all binary modules
    auto binaryModulesMember = document.FindMember("BinaryModules");
    if (binaryModulesMember != document.MemberEnd())
    {
        auto& binaryModulesArray = binaryModulesMember->value;
        ASSERT(binaryModulesArray.IsArray());
        for (rapidjson::SizeType i = 0; i < binaryModulesArray.Size(); i++)
        {
            auto& binaryModule = binaryModulesArray[i];
            const auto nameMember = binaryModule.FindMember("Name");
            if (nameMember == binaryModule.MemberEnd())
            {
                LOG(Error, "Failed to process file.");
                return true;
            }
            String name = nameMember->value.GetText();
            StringAnsi nameAnsi(nameMember->value.GetString(), nameMember->value.GetStringLength());
            String nativePath = JsonTools::GetString(binaryModule, "NativePath", String::Empty);
            String managedPath = JsonTools::GetString(binaryModule, "ManagedPath", String::Empty);
            ProcessBuildInfoPath(nativePath, projectFolderPath);
            ProcessBuildInfoPath(managedPath, projectFolderPath);
            LOG(Info, "Loading binary module {0}", name);

            // Check if that module has been already registered
            BinaryModule* module = BinaryModule::GetModule(nameAnsi);
            if (!module)
            {
                // C++
                if (nativePath.HasChars())
                {
                    // Check if this module has been statically linked
                    auto& staticallyLinkedBinaryModules = StaticallyLinkedBinaryModuleInitializer::GetStaticallyLinkedBinaryModules();
                    for (auto getter : staticallyLinkedBinaryModules)
                    {
                        module = getter();
                        if (module && module->GetName() == nameAnsi)
                            break;
                        module = nullptr;
                    }

                    if (!module)
                    {
                        // Load library
                        const auto startTime = DateTime::NowUTC();
#if PLATFORM_ANDROID
                        // On Android all native binaries are side-by-side with the app
                        nativePath = StringUtils::GetDirectoryName(Platform::GetExecutableFilePath()) / StringUtils::GetFileName(nativePath);
#endif
                        auto library = Platform::LoadLibrary(nativePath.Get());
                        if (!library)
                        {
                            LOG(Error, "Failed to load library '{0}' for binary module {1}.", nativePath, name);
                            return true;
                        }
                        char getBinaryFuncName[512];
                        StringAnsiView getBinaryFuncNamePrefix("GetBinaryModule");
                        ASSERT(getBinaryFuncNamePrefix.Length() + nameAnsi.Length() < ARRAY_COUNT(getBinaryFuncName));
                        Platform::MemoryCopy(getBinaryFuncName, getBinaryFuncNamePrefix.Get(), getBinaryFuncNamePrefix.Length());
                        Platform::MemoryCopy(getBinaryFuncName + getBinaryFuncNamePrefix.Length(), nameAnsi.Get(), nameAnsi.Length());
                        *(getBinaryFuncName + getBinaryFuncNamePrefix.Length() + nameAnsi.Length()) = '\0';
                        const auto getBinaryFunc = (GetBinaryModuleFunc)Platform::GetProcAddress(library, getBinaryFuncName);
                        if (!getBinaryFunc)
                        {
                            Platform::FreeLibrary(library);
                            LOG(Error, "Failed to setup library '{0}' for binary module {1}.", nativePath, name);
                            return true;
                        }
                        const auto endTime = DateTime::NowUTC();
                        LOG(Info, "Module {0} loaded in {1}ms", name, (int32)(endTime - startTime).GetTotalMilliseconds());

                        // Get the binary module
                        module = getBinaryFunc();
                        if (!module)
                        {
                            Platform::FreeLibrary(library);
                            LOG(Error, "Failed to get binary module {0}.", name);
                            return true;
                        }
                        ((NativeBinaryModule*)module)->Library = library;
                    }
                }
                else
                {
                    // Create module if native library is not used
                    module = New<ManagedBinaryModule>(nameAnsi, MAssemblyOptions());
                    _nonNativeModules.Add(module);
                }
            }

            // C#
            if (managedPath.HasChars() && !((ManagedBinaryModule*)module)->Assembly->IsLoaded())
            {
                if (((ManagedBinaryModule*)module)->Assembly->Load(managedPath))
                {
                    LOG(Error, "Failed to load C# assembly '{0}' for binary module {1}.", managedPath, name);
                    return true;
                }
            }

            BinaryModuleLoaded(module);
        }
    }

    return false;
}

bool Scripting::Load()
{
    // Note: this action can be called from main thread (due to Mono problems with assemblies actions from other threads)
    ASSERT(IsInMainThread());

    // Load C# core assembly
    if (GetBinaryModuleCorlib()->Assembly->Load(mono_get_corlib()))
    {
        LOG(Error, "Failed to load corlib C# assembly.");
        return true;
    }

    // Load FlaxEngine
    const String flaxEnginePath = Globals::BinariesFolder / TEXT("FlaxEngine.CSharp.dll");
    if (GetBinaryModuleFlaxEngine()->Assembly->Load(flaxEnginePath))
    {
        LOG(Error, "Failed to load FlaxEngine C# assembly.");
        return true;
    }

#if USE_EDITOR
    // Skip loading game modules in Editor on startup - Editor loads them later during splash screen (eg. after first compilation)
    static bool SkipFirstLoad = true;
    if (SkipFirstLoad)
    {
        SkipFirstLoad = false;
        return false;
    }

    // Flax.Build outputs the <target>.Build.json with binary modules to use for game scripting
    const Char *target, *platform, *architecture, *configuration;
    ScriptsBuilder::GetBinariesConfiguration(target, platform, architecture, configuration);
    if (target == nullptr)
    {
        LOG(Info, "Missing EditorTarget in project. Not using game script modules.");
        return false;
    }
    const String targetBuildInfo = Globals::ProjectFolder / TEXT("Binaries") / target / platform / architecture / configuration / target + TEXT(".Build.json");

    // Call compilation if game target build info is missing
    if (!FileSystem::FileExists(targetBuildInfo))
    {
        LOG(Info, "Missing target build info ({0}). Calling compilation.", targetBuildInfo);
        ScriptsBuilder::Compile();
        return false;
    }
#else
    const String targetBuildInfo = Globals::BinariesFolder / TEXT("Game.Build.json");
#endif

    // Load game binary modules
    if (LoadBinaryModules(targetBuildInfo, Globals::ProjectFolder))
    {
        LOG(Error, "Failed to load Game assemblies.");
        return true;
    }
    _hasGameModulesLoaded = true;

    // End
    ScriptsLoaded();
    return false;
}

void Scripting::Release()
{
    // Note: this action can be called from main thread (due to Mono problems with assemblies actions from other threads)
    ASSERT(IsInMainThread());

    // Fire event
    ScriptsUnload();

    // Cleanup
    ObjectsRemovalService::Flush();

    // Cleanup some managed objects
    MCore::GC::Collect();
    MCore::GC::WaitForPendingFinalizers();

    // Release managed objects instances for persistent objects (assets etc.)
    _objectsLocker.Lock();
    {
        for (auto i = _objectsDictionary.Begin(); i.IsNotEnd(); ++i)
        {
            auto obj = i->Value;
#if USE_OBJECTS_DISPOSE_CRASHES_DEBUGGING
            LOG(Info, "[OnScriptingDispose] obj = 0x{0:x}, {1}", (uint64)obj.Ptr, String(obj.TypeName));
#endif
            obj->OnScriptingDispose();
        }
    }
    _objectsLocker.Unlock();

    // Unload assemblies (from back to front)
    {
        LOG(Info, "Unloading binary modules");
        auto modules = BinaryModule::GetModules();
        for (int32 i = modules.Count() - 1; i >= 0; i--)
        {
            auto module = modules[i];
            if (module == GetBinaryModuleCorlib() || module == GetBinaryModuleFlaxEngine())
            {
                // Just C# assembly unload for in-build modules
                ((ManagedBinaryModule*)module)->Assembly->Unload();
                continue;
            }

            module->Destroy(false);
        }
        _nonNativeModules.Clear();
        _hasGameModulesLoaded = false;
    }

    // Cleanup
    MCore::GC::Collect();
    MCore::GC::WaitForPendingFinalizers();

    // Flush objects
    ObjectsRemovalService::Flush();

    // Switch domain
    auto rootDomain = MCore::Instance()->GetRootDomain();
    if (rootDomain)
    {
        if (!rootDomain->SetCurrentDomain(false))
        {
            LOG(Error, "Failed to set current domain to root");
        }
    }

#if !USE_SINGLE_DOMAIN
    MCore::Instance()->UnloadDomain("Scripts Domain");
#endif
}

#if USE_EDITOR

void Scripting::Reload(bool canTriggerSceneReload)
{
    // By default we allow to call it only from the main thread and when no scene is loaded.
    // Otherwise call scene manager to perform clear scripts reload.
    // It will call this method back on main thread without scenes loaded, see SceneActionType::ReloadScripts.
    if (!IsInMainThread() || Level::IsAnySceneLoaded())
    {
        if (canTriggerSceneReload)
        {
            // Call scene to reload scripts
            Level::ReloadScriptsAsync();
        }
        else
        {
            LOG(Warning, "Cannot reload scene on scripting reload. Flag is not set.");
        }
        return;
    }

    PROFILE_CPU();

    // Ideally we would call Release and Load but this would also reload Editor objects which we want avoid
    // Editor is also referencing assets and other managed objects so we should force reload everything.
    // However Reload is called when no scene is loaded.

    // Faster path - if no game assembly loaded yet
    if (!_hasGameModulesLoaded)
    {
        // Just load missing assemblies
        Load();
        return;
    }

    LOG(Info, "Start user scripts reload");
    ScriptsReloading();

    // Flush cache (some objects may be deleted after reload start event)
    ObjectsRemovalService::Flush();

    // Give GC a try to cleanup old user objects and the other mess
    MCore::GC::Collect();
    MCore::GC::WaitForPendingFinalizers();

    // Destroy objects from game assemblies (eg. not released objects that might crash if persist in memory after reload)
    _objectsLocker.Lock();
    {
        const auto flaxModule = GetBinaryModuleFlaxEngine();
        for (auto i = _objectsDictionary.Begin(); i.IsNotEnd(); ++i)
        {
            auto obj = i->Value;
            if (obj->GetTypeHandle().Module == flaxModule)
                continue;

#if USE_OBJECTS_DISPOSE_CRASHES_DEBUGGING
            LOG(Info, "[OnScriptingDispose] obj = 0x{0:x}, {1}", (uint64)obj.Ptr, String(obj.TypeName));
#endif
            obj->OnScriptingDispose();
        }
    }
    _objectsLocker.Unlock();

    // Unload all game modules
    LOG(Info, "Unloading game binary modules");
    auto modules = BinaryModule::GetModules();
    for (int32 i = modules.Count() - 1; i >= 0; i--)
    {
        BinaryModule* module = modules[i];
        if (module == GetBinaryModuleCorlib() || module == GetBinaryModuleFlaxEngine())
            continue;

        module->Destroy(true);
    }
    modules.Clear();
    _nonNativeModules.Clear();
    _hasGameModulesLoaded = false;

    // Give GC a try to cleanup old user objects and the other mess
    MCore::GC::Collect();
    MCore::GC::WaitForPendingFinalizers();

    // Load all game modules
    if (Load())
    {
        LOG(Error, "User assemblies reload failed.");
    }

    ScriptsReloaded();
    LOG(Info, "End user scripts reload");
}

#endif

MClass* Scripting::FindClass(MonoClass* monoClass)
{
    if (monoClass == nullptr)
        return nullptr;

    PROFILE_CPU_NAMED("FindClass");

    auto& modules = BinaryModule::GetModules();
    for (auto module : modules)
    {
        auto managedModule = dynamic_cast<ManagedBinaryModule*>(module);
        if (managedModule && managedModule->Assembly->IsLoaded())
        {
            MClass* result = managedModule->Assembly->GetClass(monoClass);
            if (result != nullptr)
                return result;
        }
    }

    return nullptr;
}

MClass* Scripting::FindClass(const StringAnsiView& fullname)
{
    if (fullname.IsEmpty())
        return nullptr;

    PROFILE_CPU_NAMED("FindClass");

    auto& modules = BinaryModule::GetModules();
    for (auto module : modules)
    {
        auto managedModule = dynamic_cast<ManagedBinaryModule*>(module);
        if (managedModule && managedModule->Assembly->IsLoaded())
        {
            MClass* result = managedModule->Assembly->GetClass(fullname);
            if (result != nullptr)
                return result;
        }
    }

    return nullptr;
}

MonoClass* Scripting::FindClassNative(const StringAnsiView& fullname)
{
    if (fullname.IsEmpty())
        return nullptr;

    PROFILE_CPU_NAMED("FindClassNative");

    auto& modules = BinaryModule::GetModules();
    for (auto module : modules)
    {
        auto managedModule = dynamic_cast<ManagedBinaryModule*>(module);
        if (managedModule && managedModule->Assembly->IsLoaded())
        {
            MClass* result = managedModule->Assembly->GetClass(fullname);
            if (result != nullptr)
                return result->GetNative();
        }
    }

    return nullptr;
}

ScriptingTypeHandle Scripting::FindScriptingType(const StringAnsiView& fullname)
{
    if (fullname.IsEmpty())
        return ScriptingTypeHandle();

    PROFILE_CPU_NAMED("FindScriptingType");

    auto& modules = BinaryModule::GetModules();
    for (auto module : modules)
    {
        int32 typeIndex;
        if (module->FindScriptingType(fullname, typeIndex))
        {
            return ScriptingTypeHandle(module, typeIndex);
        }
    }

    return ScriptingTypeHandle();
}

FLAXENGINE_API ScriptingObject* FindObject(const Guid& id, MClass* type)
{
    return Scripting::FindObject(id, type);
}

ScriptingObject* Scripting::FindObject(Guid id, MClass* type)
{
    if (!id.IsValid())
        return nullptr;

    PROFILE_CPU_NAMED("FindObject");

    // Try to map object id
    const auto idsMapping = ObjectsLookupIdMapping.Get();
    if (idsMapping)
    {
        idsMapping->TryGet(id, id);
    }

    // Try to find it
#if USE_OBJECTS_DISPOSE_CRASHES_DEBUGGING
    ScriptingObjectData data;
    _objectsLocker.Lock();
    _objectsDictionary.TryGet(id, data);
    _objectsLocker.Unlock();
    auto result = data.Ptr;
#else
    ScriptingObject* result = nullptr;
    _objectsLocker.Lock();
    _objectsDictionary.TryGet(id, result);
    _objectsLocker.Unlock();
#endif
    if (result)
    {
        // Check type
        if (result->Is(type))
            return result;
        LOG(Warning, "Found scripting object with ID={0} of type {1} that doesn't match type {2}.", id, String(result->GetType().Fullname), String(type->GetFullName()));
        return nullptr;
    }

    // Check if object can be an asset and try to load it
    if (type == ScriptingObject::GetStaticClass() || type->IsSubClassOf(Asset::GetStaticClass()))
    {
        Asset* asset = Content::LoadAsync(id, type);
        if (asset)
            return asset;
    }

    LOG(Warning, "Unable to find scripting object with ID={0}. Required type {1}.", id, String(type->GetFullName()));
    return nullptr;
}

ScriptingObject* Scripting::TryFindObject(Guid id, MClass* type)
{
    // Try to map object id
    const auto idsMapping = ObjectsLookupIdMapping.Get();
    if (idsMapping)
    {
        idsMapping->TryGet(id, id);
    }

    // Try to find it
#if USE_OBJECTS_DISPOSE_CRASHES_DEBUGGING
    ScriptingObjectData data;
    _objectsLocker.Lock();
    _objectsDictionary.TryGet(id, data);
    _objectsLocker.Unlock();
    auto result = data.Ptr;
#else
    ScriptingObject* result = nullptr;
    _objectsLocker.Lock();
    _objectsDictionary.TryGet(id, result);
    _objectsLocker.Unlock();
#endif

    // Check type
    if (result && !result->Is(type))
    {
        result = nullptr;
    }

    return result;
}

ScriptingObject* Scripting::FindObject(const MonoObject* managedInstance)
{
    if (managedInstance == nullptr)
        return nullptr;

    PROFILE_CPU_NAMED("FindObject");

    // TODO: optimize it by reading the unmanagedPtr or _internalId from managed Object property

    ScopeLock lock(_objectsLocker);

    for (auto i = _objectsDictionary.Begin(); i.IsNotEnd(); ++i)
    {
        const auto obj = i->Value;
        if (obj->GetManagedInstance() == managedInstance)
            return obj;
    }
    return nullptr;
}

void Scripting::OnManagedInstanceDeleted(ScriptingObject* obj)
{
    PROFILE_CPU_NAMED("OnManagedInstanceDeleted");
    ASSERT(obj);

    // Validate if object still exists
    _objectsLocker.Lock();
    if (_objectsDictionary.ContainsValue(obj))
    {
#if USE_OBJECTS_DISPOSE_CRASHES_DEBUGGING
        LOG(Info, "[OnManagedInstanceDeleted] obj = 0x{0:x}, {1}", (uint64)obj, String(ScriptingObjectData(obj).TypeName));
#endif
        obj->OnManagedInstanceDeleted();
    }
    else
    {
        //LOG(Warning, "Object finalization called for already removed object (address={0:x})", (uint64)obj);
    }
    _objectsLocker.Unlock();
}

bool Scripting::HasGameModulesLoaded()
{
    return _hasGameModulesLoaded;
}

bool Scripting::IsEveryAssemblyLoaded()
{
    auto& modules = BinaryModule::GetModules();
    for (BinaryModule* module : modules)
    {
        if (!module->IsLoaded())
            return false;
    }
    return true;
}

bool Scripting::IsTypeFromGameScripts(MClass* type)
{
    const auto binaryModule = ManagedBinaryModule::GetModule(type->GetAssembly());
    return binaryModule && binaryModule != GetBinaryModuleCorlib() && binaryModule != GetBinaryModuleFlaxEngine();
}

void Scripting::RegisterObject(ScriptingObject* obj)
{
    ScopeLock lock(_objectsLocker);

    //ASSERT(!_objectsDictionary.ContainsValue(obj));
#if ENABLE_ASSERTION
#if USE_OBJECTS_DISPOSE_CRASHES_DEBUGGING
    ScriptingObjectData other;
    if (_objectsDictionary.TryGet(obj->GetID(), other))
#else
    ScriptingObject* other;
    if (_objectsDictionary.TryGet(obj->GetID(), other))
#endif
    {
        // Something went wrong...
        LOG(Error, "Objects registry already contains object with ID={0} (type '{3}')! Trying to register object {1} (type '{2}').", obj->GetID(), obj->ToString(), String(obj->GetClass()->GetFullName()), String(other->GetClass()->GetFullName()));
        _objectsDictionary.Remove(obj->GetID());
    }
#else
	ASSERT(!_objectsDictionary.ContainsKey(obj->_id));
#endif

#if USE_OBJECTS_DISPOSE_CRASHES_DEBUGGING
    LOG(Info, "[RegisterObject] obj = 0x{0:x}, {1}", (uint64)obj, String(ScriptingObjectData(obj).TypeName));
#endif
    _objectsDictionary.Add(obj->GetID(), obj);
}

void Scripting::UnregisterObject(ScriptingObject* obj)
{
    ScopeLock lock(_objectsLocker);

    //ASSERT(!obj->_id.IsValid() || _objectsDictionary.ContainsValue(obj));

#if USE_OBJECTS_DISPOSE_CRASHES_DEBUGGING
    LOG(Info, "[UnregisterObject] obj = 0x{0:x}, {1}", (uint64)obj, String(ScriptingObjectData(obj).TypeName));
#endif
    _objectsDictionary.Remove(obj->GetID());
}

void Scripting::OnObjectIdChanged(ScriptingObject* obj, const Guid& oldId)
{
    ScopeLock lock(_objectsLocker);

    ASSERT(obj && oldId.IsValid());
    ASSERT(obj->GetID() != oldId);
    ASSERT(_objectsDictionary.ContainsKey(oldId));
    //ASSERT(_objectsDictionary.ContainsValue(obj));
    ASSERT(!_objectsDictionary.ContainsKey(obj->GetID()));

    _objectsDictionary.Remove(oldId);
    _objectsDictionary.Add(obj->GetID(), obj);
}

bool initFlaxEngine()
{
    // Cache common types
    if (StdTypesContainer::Instance()->Gather())
        return true;

    // Init C# class library
    {
        auto scriptingClass = Scripting::GetStaticClass();
        ASSERT(scriptingClass);
        const auto initMethod = scriptingClass->GetMethod("Init");
        ASSERT(initMethod);
        MonoObject* exception = nullptr;
        initMethod->Invoke(nullptr, nullptr, &exception);
        if (exception)
        {
            MException ex(exception);
            ex.Log(LogType::Fatal, TEXT("FlaxEngine.ClassLibraryInitializer.Init"));
            return true;
        }

        // TODO: move it somewhere to game instance class or similar
        MainRenderTask::Instance = New<MainRenderTask>();
    }

    return false;
}

void onEngineLoaded(MAssembly* assembly)
{
    if (initFlaxEngine())
    {
        LOG(Fatal, "Failed to initialize Flax Engine runtime.");
    }

    // Set flag
    _isEngineAssemblyLoaded = true;
}

void onEngineUnloading(MAssembly* assembly)
{
    // Clear flag
    _isEngineAssemblyLoaded = false;

    // Clear cached methods
    _method_Update = nullptr;
    _method_LateUpdate = nullptr;
    _method_FixedUpdate = nullptr;
    _method_Exit = nullptr;

    StdTypesContainer::Instance()->Clear();
}

void ScriptingService::Dispose()
{
    Scripting::Release();

    MCore::Instance()->UnloadEngine();
}
