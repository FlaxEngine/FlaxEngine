// Copyright (c) Wojciech Figat. All rights reserved.

#include "BinaryModule.h"
#include "Scripting.h"
#include "ScriptingType.h"
#include "FlaxEngine.Gen.h"
#include "Engine/Threading/Threading.h"
#include "Engine/Threading/ThreadLocal.h"
#include "Engine/Threading/IRunnable.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Platform/File.h"
#include "Engine/Debug/DebugLog.h"
#if USE_EDITOR
#include "Engine/Level/Level.h"
#include "Editor/Scripting/ScriptsBuilder.h"
#endif
#include "ManagedCLR/MAssembly.h"
#include "ManagedCLR/MClass.h"
#include "ManagedCLR/MMethod.h"
#include "ManagedCLR/MDomain.h"
#include "ManagedCLR/MCore.h"
#include "ManagedCLR/MException.h"
#include "Internal/StdTypesContainer.h"
#include "Engine/Core/LogContext.h"
#include "Engine/Core/ObjectsRemovalService.h"
#include "Engine/Core/Types/TimeSpan.h"
#include "Engine/Core/Types/Stopwatch.h"
#include "Engine/Content/Asset.h"
#include "Engine/Content/Content.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Engine/Globals.h"
#include "Engine/Engine/Time.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Serialization/JsonTools.h"
#include "Engine/Profiler/ProfilerCPU.h"

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
    void LateFixedUpdate() override;
    void Draw() override;
    void BeforeExit() override;
    void Dispose() override;
};

namespace
{
    MDomain* _rootDomain = nullptr;
    MDomain* _scriptsDomain = nullptr;
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
    MMethod* _method_LateFixedUpdate = nullptr;
    MMethod* _method_Draw = nullptr;
    MMethod* _method_Exit = nullptr;
    Array<Function<void()>> UpdateActions;
    Dictionary<StringAnsi, BinaryModule*, InlinedAllocation<64>> _nonNativeModules;
#if USE_EDITOR
    bool LastBinariesLoadTriggeredCompilation = false;
#endif

    void ReleaseObjects(bool gameOnly)
    {
        // Flush objects already enqueued objects to delete
        ObjectsRemovalService::Flush();

        // Give GC a try to cleanup old user objects and the other mess
        MCore::GC::Collect();
        MCore::GC::WaitForPendingFinalizers();

        // Destroy objects from game assemblies (eg. not released objects that might crash if persist in memory after reload)
        const auto flaxModule = GetBinaryModuleFlaxEngine();
        _objectsLocker.Lock();
        for (auto i = _objectsDictionary.Begin(); i.IsNotEnd(); ++i)
        {
            auto obj = i->Value;
            if (gameOnly && obj->GetTypeHandle().Module == flaxModule)
                continue;

#if USE_OBJECTS_DISPOSE_CRASHES_DEBUGGING
            LOG(Info, "[OnScriptingDispose] obj = 0x{0:x}, {1}", (uint64)obj.Ptr, String(obj.TypeName));
#endif
            obj->OnScriptingDispose();
        }
        _objectsLocker.Unlock();

        // Release assets sourced from game assemblies
        Array<Asset*> assets = Content::GetAssets();
        for (auto asset : assets)
        {
            if (asset->GetTypeHandle().Module == flaxModule)
            {
                continue;
            }

            asset->DeleteObject();
        }
        ObjectsRemovalService::Flush();
    }
}

Delegate<BinaryModule*> Scripting::BinaryModuleLoaded;
Action Scripting::ScriptsLoaded;
Action Scripting::ScriptsUnload;
Action Scripting::ScriptsReloading;
Action Scripting::ScriptsReloaded;
Action Scripting::Update;
Action Scripting::LateUpdate;
Action Scripting::FixedUpdate;
Action Scripting::LateFixedUpdate;
Action Scripting::Draw;
Action Scripting::Exit;
ThreadLocal<Scripting::IdsMappingTable*, PLATFORM_THREADS_LIMIT> Scripting::ObjectsLookupIdMapping;
ScriptingService ScriptingServiceInstance;

bool initFlaxEngine();

// Assembly events
void onEngineLoaded(MAssembly* assembly);
void onEngineUnloading(MAssembly* assembly);

bool ScriptingService::Init()
{
    Stopwatch stopwatch;

    // Initialize managed runtime
    if (MCore::LoadEngine())
    {
        LOG(Fatal, "C# runtime initialization failed.");
        return true;
    }

    MCore::CreateScriptingAssemblyLoadContext();

    // Cache root domain
    _rootDomain = MCore::GetRootDomain();

#if USE_SCRIPTING_SINGLE_DOMAIN
    // Use single root domain
    auto domain = _rootDomain;
#else
    // Create Mono domain for scripts
    auto domain = MCore::CreateDomain("Scripts Domain");
#endif
    domain->SetCurrentDomain(true);
    _scriptsDomain = domain;

    // Add internal calls
    registerFlaxEngineInternalCalls();

    // Load assemblies
    if (Scripting::Load())
    {
        LOG(Fatal, "Scripting Engine initialization failed.");
        return true;
    }

    stopwatch.Stop();
    LOG(Info, "Scripting Engine initializated! (time: {0}ms)", stopwatch.GetMilliseconds());
    return false;
}

#if COMPILE_WITHOUT_CSHARP
#define INVOKE_EVENT(name) Scripting::name();
#else
#define INVOKE_EVENT(name) Scripting::name(); \
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
	MObject* exception = nullptr; \
	_method_##name->Invoke(nullptr, nullptr, &exception); \
	DebugLog::LogException(exception)
#endif

void ScriptingService::Update()
{
    PROFILE_CPU_NAMED("Scripting::Update");
    INVOKE_EVENT(Update);

    // Flush update actions
    _objectsLocker.Lock();
    int32 count = UpdateActions.Count();
    for (int32 i = 0; i < count; i++)
    {
        UpdateActions[i]();
    }
    int32 newlyAdded = UpdateActions.Count() - count;
    if (newlyAdded == 0)
        UpdateActions.Clear();
    else
    {
        // Someone added another action within current callback
        Array<Function<void()>> tmp;
        for (int32 i = newlyAdded; i < UpdateActions.Count(); i++)
            tmp.Add(UpdateActions[i]);
        UpdateActions.Clear();
        UpdateActions.Add(tmp);
    }
    _objectsLocker.Unlock();

#ifdef USE_NETCORE
    // Force GC to run in background periodically to avoid large blocking collections causing hitches
    if (Time::Update.TicksCount % 60 == 0)
    {
        MCore::GC::Collect(MCore::GC::MaxGeneration(), MGCCollectionMode::Forced, false, false);
    }
#endif
}

void ScriptingService::LateUpdate()
{
    PROFILE_CPU_NAMED("Scripting::LateUpdate");
    INVOKE_EVENT(LateUpdate);
}

void ScriptingService::FixedUpdate()
{
    PROFILE_CPU_NAMED("Scripting::FixedUpdate");
    INVOKE_EVENT(FixedUpdate);
}

void ScriptingService::LateFixedUpdate()
{
    PROFILE_CPU_NAMED("Scripting::LateFixedUpdate");
    INVOKE_EVENT(LateFixedUpdate);
}

void ScriptingService::Draw()
{
    PROFILE_CPU_NAMED("Scripting::Draw");
    INVOKE_EVENT(Draw);
}

void ScriptingService::BeforeExit()
{
    PROFILE_CPU_NAMED("Scripting::BeforeExit");
    INVOKE_EVENT(Exit);
}

MDomain* Scripting::GetRootDomain()
{
    return _rootDomain;
}

MDomain* Scripting::GetScriptsDomain()
{
    return _scriptsDomain;
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

void Scripting::InvokeOnUpdate(const Function<void()>& action)
{
    _objectsLocker.Lock();
    UpdateActions.Add(action);
    _objectsLocker.Unlock();
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
    {
        PROFILE_CPU_NAMED("Json.Parse");
        document.Parse((char*)fileData.Get(), fileData.Count());
    }
    if (document.HasParseError())
    {
        LOG(Error, "Failed to file contents.");
        return true;
    }

    // TODO: validate Name, Platform, Architecture, Configuration from file

    // Load all references
    auto referencesMember = document.FindMember("References");
    if (referencesMember != document.MemberEnd() && referencesMember->value.IsArray())
    {
        auto& referencesArray = referencesMember->value;
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
    if (binaryModulesMember != document.MemberEnd() && binaryModulesMember->value.IsArray())
    {
        auto& binaryModulesArray = binaryModulesMember->value;
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
                _nonNativeModules.TryGet(nameAnsi, module);
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
                        Stopwatch stopwatch;
#if PLATFORM_ANDROID || PLATFORM_MAC
                        // On some platforms all native binaries are side-by-side with the app in a different folder
                        if (!FileSystem::FileExists(nativePath))
                        {
                            nativePath = String(StringUtils::GetDirectoryName(Platform::GetExecutableFilePath())) / StringUtils::GetFileName(nativePath);
                        }
#elif PLATFORM_IOS
                        // iOS uses Frameworks folder with native binaries
                        if (!FileSystem::FileExists(nativePath))
                        {
                            nativePath = Globals::ProjectFolder / TEXT("Frameworks") / StringUtils::GetFileName(nativePath);
                        }
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
                        stopwatch.Stop();
                        LOG(Info, "Module {0} loaded in {1}ms", name, stopwatch.GetMilliseconds());

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
                    module = New<ManagedBinaryModule>(nameAnsi);
                    _nonNativeModules.Add(nameAnsi, module);
                }
            }

#if !COMPILE_WITHOUT_CSHARP
            // C#
            if (managedPath.HasChars() && !((ManagedBinaryModule*)module)->Assembly->IsLoaded())
            {
                if (((ManagedBinaryModule*)module)->Assembly->Load(managedPath, nativePath))
                {
                    LOG(Error, "Failed to load C# assembly '{0}' for binary module {1}.", managedPath, name);
                    return true;
                }
            }
#endif

            BinaryModuleLoaded(module);
        }
    }

    return false;
}

bool Scripting::Load()
{
    PROFILE_CPU();
    // Note: this action can be called from main thread (due to Mono problems with assemblies actions from other threads)
    ASSERT(IsInMainThread());
    ScopeLock lock(BinaryModule::Locker);

#if USE_CSHARP
    // Load C# core assembly
    ManagedBinaryModule* corlib = GetBinaryModuleCorlib();
    if (corlib->Assembly->LoadCorlib())
    {
        LOG(Error, "Failed to load corlib C# assembly.");
        return true;
    }

    // Initialize C# corelib types
    {
        const auto& corlibClasses = corlib->Assembly->GetClasses();
        bool gotAll = true;
#define CACHE_CORLIB_CLASS(var, name) gotAll &= corlibClasses.TryGet(StringAnsiView(name), MCore::TypeCache::var)
        CACHE_CORLIB_CLASS(Void, "System.Void");
        CACHE_CORLIB_CLASS(Object, "System.Object");
        CACHE_CORLIB_CLASS(Byte, "System.Byte");
        CACHE_CORLIB_CLASS(Boolean, "System.Boolean");
        CACHE_CORLIB_CLASS(SByte, "System.SByte");
        CACHE_CORLIB_CLASS(Char, "System.Char");
        CACHE_CORLIB_CLASS(Int16, "System.Int16");
        CACHE_CORLIB_CLASS(UInt16, "System.UInt16");
        CACHE_CORLIB_CLASS(Int32, "System.Int32");
        CACHE_CORLIB_CLASS(UInt32, "System.UInt32");
        CACHE_CORLIB_CLASS(Int64, "System.Int64");
        CACHE_CORLIB_CLASS(UInt64, "System.UInt64");
        CACHE_CORLIB_CLASS(IntPtr, "System.IntPtr");
        CACHE_CORLIB_CLASS(UIntPtr, "System.UIntPtr");
        CACHE_CORLIB_CLASS(Single, "System.Single");
        CACHE_CORLIB_CLASS(Double, "System.Double");
        CACHE_CORLIB_CLASS(String, "System.String");
#undef CACHE_CORLIB_CLASS
        if (!gotAll)
        {
            LOG(Error, "Failed to load corlib C# assembly.");
            for (const auto& e : corlibClasses)
                LOG(Info, "Class: {0}", String(e.Value->GetFullName()));
            return true;
        }
    }
#endif

    // Load FlaxEngine
    const String flaxEnginePath = Globals::BinariesFolder / TEXT("FlaxEngine.CSharp.dll");
    auto* flaxEngineModule = (NativeBinaryModule*)GetBinaryModuleFlaxEngine();
    if (!flaxEngineModule->Assembly->IsLoaded())
    {
        if (flaxEngineModule->Assembly->Load(flaxEnginePath))
        {
            LOG(Error, "Failed to load FlaxEngine C# assembly.");
            return true;
        }
        onEngineLoaded(flaxEngineModule->Assembly);

        // Insert type aliases for vector types that don't exist in C++ but are just typedef (properly redirect them to actual types)
        // TODO: add support for automatic typedef aliases setup for scripting module to properly lookup type from the alias typename
#if USE_LARGE_WORLDS
        flaxEngineModule->TypeNameToTypeIndex["FlaxEngine.Vector2"] = flaxEngineModule->TypeNameToTypeIndex["FlaxEngine.Double2"];
        flaxEngineModule->TypeNameToTypeIndex["FlaxEngine.Vector3"] = flaxEngineModule->TypeNameToTypeIndex["FlaxEngine.Double3"];
        flaxEngineModule->TypeNameToTypeIndex["FlaxEngine.Vector4"] = flaxEngineModule->TypeNameToTypeIndex["FlaxEngine.Double4"];
#else
        flaxEngineModule->TypeNameToTypeIndex["FlaxEngine.Vector2"] = flaxEngineModule->TypeNameToTypeIndex["FlaxEngine.Float2"];
        flaxEngineModule->TypeNameToTypeIndex["FlaxEngine.Vector3"] = flaxEngineModule->TypeNameToTypeIndex["FlaxEngine.Float3"];
        flaxEngineModule->TypeNameToTypeIndex["FlaxEngine.Vector4"] = flaxEngineModule->TypeNameToTypeIndex["FlaxEngine.Float4"];
#endif
#if USE_CSHARP
        flaxEngineModule->ClassToTypeIndex[flaxEngineModule->Assembly->GetClass("FlaxEngine.Vector2")] = flaxEngineModule->TypeNameToTypeIndex["FlaxEngine.Vector2"];
        flaxEngineModule->ClassToTypeIndex[flaxEngineModule->Assembly->GetClass("FlaxEngine.Vector3")] = flaxEngineModule->TypeNameToTypeIndex["FlaxEngine.Vector3"];
        flaxEngineModule->ClassToTypeIndex[flaxEngineModule->Assembly->GetClass("FlaxEngine.Vector4")] = flaxEngineModule->TypeNameToTypeIndex["FlaxEngine.Vector4"];
#endif
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
    if (StringUtils::Length(target) == 0)
    {
        LOG(Info, "Missing EditorTarget in project. Not using game script modules.");
        _hasGameModulesLoaded = true;
        return false;
    }
    const String targetBuildInfo = Globals::ProjectFolder / TEXT("Binaries") / target / platform / architecture / configuration / target + TEXT(".Build.json");

    // Call compilation if game target build info is missing
    if (!FileSystem::FileExists(targetBuildInfo))
    {
        LOG(Info, "Missing target build info ({0})", targetBuildInfo);
        if (LastBinariesLoadTriggeredCompilation)
            return false;
        LastBinariesLoadTriggeredCompilation = true;
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
    PROFILE_CPU();
    // Note: this action can be called from main thread (due to Mono problems with assemblies actions from other threads)
    ASSERT(IsInMainThread());

    // Fire event
    ScriptsUnload();

    // Release managed objects instances for persistent objects (assets etc.)
    ReleaseObjects(false);

    auto* flaxEngineModule = (NativeBinaryModule*)GetBinaryModuleFlaxEngine();
    onEngineUnloading(flaxEngineModule->Assembly);

    // Unload assemblies (from back to front)
    {
        LOG(Info, "Unloading binary modules");
        auto modules = BinaryModule::GetModules();
        for (int32 i = modules.Count() - 1; i >= 0; i--)
        {
            auto module = modules[i];
            module->Destroy(false);
        }
        _nonNativeModules.ClearDelete();
        _hasGameModulesLoaded = false;
    }

    // Cleanup
    MCore::GC::Collect();
    MCore::GC::WaitForPendingFinalizers();

    // Flush objects
    ObjectsRemovalService::Flush();

    // Switch domain
    auto rootDomain = MCore::GetRootDomain();
    if (rootDomain)
    {
        if (!rootDomain->SetCurrentDomain(false))
        {
            LOG(Error, "Failed to set current domain to root");
        }
    }

#if !USE_SCRIPTING_SINGLE_DOMAIN
    MCore::UnloadDomain("Scripts Domain");
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

    // Destroy objects from game assemblies (eg. not released objects that might crash if persist in memory after reload)
    ReleaseObjects(true);

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
    _nonNativeModules.ClearDelete();
    _hasGameModulesLoaded = false;

    // Release and create a new assembly load context for user assemblies
    MCore::UnloadScriptingAssemblyLoadContext();
    MCore::CreateScriptingAssemblyLoadContext();

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

Array<ScriptingObject*, HeapAllocation> Scripting::GetObjects()
{
    Array<ScriptingObject*> objects;
    _objectsLocker.Lock();
#if USE_OBJECTS_DISPOSE_CRASHES_DEBUGGING
    for (const auto& e : _objectsDictionary)
        objects.Add(e.Value.Ptr);
#else
    _objectsDictionary.GetValues(objects);
#endif
    _objectsLocker.Unlock();
    return objects;
}

MClass* Scripting::FindClass(const StringAnsiView& fullname)
{
    if (fullname.IsEmpty())
        return nullptr;
    PROFILE_CPU();
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

ScriptingTypeHandle Scripting::FindScriptingType(const StringAnsiView& fullname)
{
    if (fullname.IsEmpty())
        return ScriptingTypeHandle();
    PROFILE_CPU();
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

ScriptingObject* Scripting::NewObject(const ScriptingTypeHandle& type)
{
    if (!type)
    {
        LOG(Error, "Invalid type.");
        return nullptr;
    }
    const ScriptingType& scriptingType = type.GetType();

    // Create unmanaged object
    const ScriptingObjectSpawnParams params(Guid::New(), type);
    ScriptingObject* obj = scriptingType.Script.Spawn(params);
    if (obj == nullptr)
        LOG(Error, "Failed to spawn object of type \'{0}\'.", scriptingType.ToString());
    return obj;
}

ScriptingObject* Scripting::NewObject(const MClass* type)
{
    if (type == nullptr)
    {
        LOG(Error, "Invalid type.");
        return nullptr;
    }
#if USE_CSHARP
    // Get the assembly with that class
    auto module = ManagedBinaryModule::FindModule(type);
    if (module == nullptr)
    {
        LOG(Error, "Cannot find scripting assembly for type \'{0}\'.", String(type->GetFullName()));
        return nullptr;
    }

    // Try to find the scripting type for this class
    int32 typeIndex;
    if (!module->ClassToTypeIndex.TryGet(type, typeIndex))
    {
        LOG(Error, "Cannot spawn objects of type \'{0}\'.", String(type->GetFullName()));
        return nullptr;
    }
    const ScriptingType& scriptingType = module->Types[typeIndex];

    // Create unmanaged object
    const ScriptingObjectSpawnParams params(Guid::New(), ScriptingTypeHandle(module, typeIndex));
    ScriptingObject* obj = scriptingType.Script.Spawn(params);
    if (obj == nullptr)
        LOG(Error, "Failed to spawn object of type \'{0}\'.", scriptingType.ToString());
    return obj;
#else
    LOG(Error, "Not supported object creation from Managed class.");
    return nullptr;
#endif
}

FLAXENGINE_API ScriptingObject* FindObject(const Guid& id, MClass* type)
{
    return Scripting::FindObject(id, type);
}

void ScriptingObjectReferenceBase::OnSet(ScriptingObject* object)
{
    auto e = _object;
    if (e != object)
    {
        if (e)
            e->Deleted.Unbind<ScriptingObjectReferenceBase, &ScriptingObjectReferenceBase::OnDeleted>(this);
        _object = e = object;
        if (e)
            e->Deleted.Bind<ScriptingObjectReferenceBase, &ScriptingObjectReferenceBase::OnDeleted>(this);
        Changed();
    }
}

void ScriptingObjectReferenceBase::OnDeleted(ScriptingObject* obj)
{
    if (_object == obj)
    {
        _object->Deleted.Unbind<ScriptingObjectReferenceBase, &ScriptingObjectReferenceBase::OnDeleted>(this);
        _object = nullptr;
        Changed();
    }
}

ScriptingObject* Scripting::FindObject(Guid id, const MClass* type)
{
    if (!id.IsValid())
        return nullptr;
    PROFILE_CPU();

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
        if (!type || result->Is(type))
            return result;
        LOG(Warning, "Found scripting object with ID={0} of type {1} that doesn't match type {2}", id, String(result->GetType().Fullname), String(type->GetFullName()));
        LogContext::Print(LogType::Warning);
        return nullptr;
    }

    // Check if object can be an asset and try to load it
    if (!type)
    {
        result = Content::LoadAsync<Asset>(id);
        if (!result)
            LOG(Warning, "Unable to find scripting object with ID={0}", id);
        return result;
    }
    if (type == ScriptingObject::GetStaticClass() || type->IsSubClassOf(Asset::GetStaticClass()))
    {
        Asset* asset = Content::LoadAsync(id, type);
        if (asset)
            return asset;
    }

    LOG(Warning, "Unable to find scripting object with ID={0}. Required type {1}", id, String(type->GetFullName()));
    LogContext::Print(LogType::Warning);
    return nullptr;
}

ScriptingObject* Scripting::TryFindObject(Guid id, const MClass* type)
{
    if (!id.IsValid())
        return nullptr;
    PROFILE_CPU();

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
    if (result && type && !result->Is(type))
    {
        result = nullptr;
    }

    return result;
}

ScriptingObject* Scripting::TryFindObject(const MClass* type)
{
    if (type == nullptr)
        return nullptr;
    ScopeLock lock(_objectsLocker);
    for (auto i = _objectsDictionary.Begin(); i.IsNotEnd(); ++i)
    {
        const auto obj = i->Value;
        if (obj->GetClass() == type)
            return obj;
    }
    return nullptr;
}

ScriptingObject* Scripting::FindObject(const MObject* managedInstance)
{
    if (managedInstance == nullptr)
        return nullptr;
    PROFILE_CPU();

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
    PROFILE_CPU();
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

bool Scripting::IsTypeFromGameScripts(const MClass* type)
{
    const auto binaryModule = ManagedBinaryModule::GetModule(type ? type->GetAssembly() : nullptr);
    return binaryModule && binaryModule != GetBinaryModuleCorlib() && binaryModule != GetBinaryModuleFlaxEngine();
}

void Scripting::RegisterObject(ScriptingObject* obj)
{
    const Guid id = obj->GetID();
    ScopeLock lock(_objectsLocker);

    //ASSERT(!_objectsDictionary.ContainsValue(obj));
#if ENABLE_ASSERTION
#if USE_OBJECTS_DISPOSE_CRASHES_DEBUGGING
    ScriptingObjectData other;
    if (_objectsDictionary.TryGet(id, other))
#else
    ScriptingObject* other;
    if (_objectsDictionary.TryGet(id, other))
#endif
    {
        // Something went wrong...
        LOG(Error, "Objects registry already contains object with ID={0} (type '{3}')! Trying to register object {1} (type '{2}').", id, obj->ToString(), String(obj->GetClass()->GetFullName()), String(other->GetClass()->GetFullName()));
    }
#endif

#if USE_OBJECTS_DISPOSE_CRASHES_DEBUGGING
    LOG(Info, "[RegisterObject] obj = 0x{0:x}, {1}", (uint64)obj, String(ScriptingObjectData(obj).TypeName));
#endif
    _objectsDictionary[id] = obj;
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

#if !COMPILE_WITHOUT_CSHARP
    // Init C# class library
    {
        auto scriptingClass = Scripting::GetStaticClass();
        ASSERT(scriptingClass);
        const auto initMethod = scriptingClass->GetMethod("Init");
        ASSERT(initMethod);
        MObject* exception = nullptr;
        initMethod->Invoke(nullptr, nullptr, &exception);
        if (exception)
        {
            MException ex(exception);
            ex.Log(LogType::Fatal, TEXT("FlaxEngine.Scripting.Init"));
            return true;
        }
    }
#endif

    // TODO: move it somewhere to game instance class or similar
    MainRenderTask::Instance = New<MainRenderTask>();

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

    MCore::UnloadEngine();
}
