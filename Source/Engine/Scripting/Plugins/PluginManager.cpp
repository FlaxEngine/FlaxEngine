// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#include "PluginManager.h"
#include "FlaxEngine.Gen.h"
#include "GamePlugin.h"
#include "Engine/Scripting/Scripting.h"
#include "Engine/Scripting/ScriptingType.h"
#include "Engine/Scripting/BinaryModule.h"
#include "Engine/Scripting/ManagedCLR/MAssembly.h"
#include "Engine/Scripting/ManagedCLR/MClass.h"
#include "Engine/Scripting/ManagedCLR/MUtils.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Core/Log.h"

Plugin::Plugin(const SpawnParams& params)
    : ScriptingObject(params)
{
    _description.Name = String(GetType().GetName());
    _description.Category = TEXT("Other");
    _description.Version = Version(1, 0);
}

GamePlugin::GamePlugin(const SpawnParams& params)
    : Plugin(params)
{
}

#if USE_EDITOR

#include "EditorPlugin.h"
#include "Engine/Scripting/MException.h"
#include "Engine/Scripting/ManagedCLR/MMethod.h"

EditorPlugin::EditorPlugin(const SpawnParams& params)
    : Plugin(params)
{
}

void EditorPlugin::Initialize()
{
    Plugin::Initialize();

    MObject* exception = nullptr;
    TypeInitializer.GetType().ManagedClass->GetMethod("Initialize_Internal")->Invoke(GetOrCreateManagedInstance(), nullptr, &exception);
    if (exception)
    {
        MException ex(exception);
        ex.Log(LogType::Error,TEXT("EditorPlugin"));
    }
}

void EditorPlugin::Deinitialize()
{
    MObject* exception = nullptr;
    TypeInitializer.GetType().ManagedClass->GetMethod("Deinitialize_Internal")->Invoke(GetOrCreateManagedInstance(), nullptr, &exception);
    if (exception)
    {
        MException ex(exception);
        ex.Log(LogType::Error,TEXT("EditorPlugin"));
    }

    Plugin::Deinitialize();
}

#endif

Delegate<Plugin*> PluginManager::PluginLoading;
Delegate<Plugin*> PluginManager::PluginLoaded;
Delegate<Plugin*> PluginManager::PluginUnloading;
Delegate<Plugin*> PluginManager::PluginUnloaded;
Action PluginManager::PluginsChanged;

namespace PluginManagerImpl
{
    Array<GamePlugin*> GamePlugins;
    Array<Plugin*> EditorPlugins;

    void LoadPlugin(MClass* klass, bool isEditor);
    void OnAssemblyLoaded(MAssembly* assembly);
    void OnAssemblyUnloading(MAssembly* assembly);
    void OnBinaryModuleLoaded(BinaryModule* module);
    void OnScriptsReloading();
}

using namespace PluginManagerImpl;

class PluginManagerService : public EngineService
{
public:
    PluginManagerService()
        : EngineService(TEXT("Plugin Manager"), 130)
    {
    }

    bool Init() override;
    void Dispose() override;

    static void InvokeInitialize(Plugin* plugin);
    static void InvokeDeinitialize(Plugin* plugin);
};

PluginManagerService PluginManagerServiceInstance;

void PluginManagerService::InvokeInitialize(Plugin* plugin)
{
    if (plugin->_initialized)
        return;
    StringAnsiView typeName = plugin->GetType().GetName();
    PROFILE_CPU();
    ZoneName(typeName.Get(), typeName.Length());

    LOG(Info, "Loading plugin {}", plugin->ToString());

    PluginManager::PluginLoading(plugin);

    plugin->Initialize();
    plugin->_initialized = true;

    PluginManager::PluginLoaded(plugin);
}

void PluginManagerService::InvokeDeinitialize(Plugin* plugin)
{
    if (!plugin->_initialized)
        return;
    StringAnsiView typeName = plugin->GetType().GetName();
    PROFILE_CPU();
    ZoneName(typeName.Get(), typeName.Length());

    LOG(Info, "Unloading plugin {}", plugin->ToString());

    PluginManager::PluginUnloading(plugin);

    plugin->Deinitialize();
    plugin->_initialized = false;

    PluginManager::PluginUnloaded(plugin);
}

void PluginManagerImpl::LoadPlugin(MClass* klass, bool isEditor)
{
    // Create and check if use it
    auto plugin = (Plugin*)Scripting::NewObject(klass);
    if (!plugin)
        return;

    if (!isEditor)
    {
        GamePlugins.Add((GamePlugin*)plugin);
#if !USE_EDITOR
        PluginManagerService::InvokeInitialize(plugin);
#endif
    }
#if USE_EDITOR
    else
    {
        EditorPlugins.Add(plugin);
        PluginManagerService::InvokeInitialize(plugin);
    }
#endif
    PluginManager::PluginsChanged();
}

void PluginManagerImpl::OnAssemblyLoaded(MAssembly* assembly)
{
    PROFILE_CPU_NAMED("Load Assembly Plugins");

    const auto gamePluginClass = GamePlugin::GetStaticClass();
    if (gamePluginClass == nullptr)
    {
        LOG(Warning, "Missing GamePlugin class.");
        return;
    }
#if USE_EDITOR
    const auto editorPluginClass = ((ManagedBinaryModule*)GetBinaryModuleFlaxEngine())->Assembly->GetClass("FlaxEditor.EditorPlugin");
    if (editorPluginClass == nullptr)
    {
        LOG(Warning, "Missing EditorPlugin class.");
        return;
    }
#endif

    // Process all classes to find plugins
    auto& classes = assembly->GetClasses();
    for (auto i = classes.Begin(); i.IsNotEnd(); ++i)
    {
        auto mclass = i->Value;

        // Skip invalid classes
        if (mclass->IsGeneric() || mclass->IsStatic() || mclass->IsAbstract())
            continue;

        if (mclass->IsSubClassOf(gamePluginClass))
        {
            LoadPlugin(mclass, false);
        }

#if USE_EDITOR
        if (mclass->IsSubClassOf(editorPluginClass))
        {
            LoadPlugin(mclass, true);
        }
#endif
    }
}

void PluginManagerImpl::OnAssemblyUnloading(MAssembly* assembly)
{
    bool changed = false;
    for (int32 i = EditorPlugins.Count() - 1; i >= 0 && EditorPlugins.Count() > 0; i--)
    {
        auto plugin = EditorPlugins[i];
        if (plugin->GetType().ManagedClass->GetAssembly() == assembly)
        {
            PluginManagerService::InvokeDeinitialize(plugin);
            EditorPlugins.RemoveAtKeepOrder(i);
            changed = true;
        }
    }
    for (int32 i = GamePlugins.Count() - 1; i >= 0 && GamePlugins.Count() > 0; i--)
    {
        auto plugin = GamePlugins[i];
        if (plugin->GetType().ManagedClass->GetAssembly() == assembly)
        {
            PluginManagerService::InvokeDeinitialize(plugin);
            GamePlugins.RemoveAtKeepOrder(i);
            changed = true;
        }
    }
    if (changed)
        PluginManager::PluginsChanged();
}

void PluginManagerImpl::OnBinaryModuleLoaded(BinaryModule* module)
{
    // Skip special modules
    if (module == GetBinaryModuleFlaxEngine() ||
        module == GetBinaryModuleCorlib())
        return;

    // Skip non-managed modules
    // TODO: search native-only modules too
    auto managedModule = dynamic_cast<ManagedBinaryModule*>(module);
    if (!managedModule)
        return;

    // Process already loaded C# assembly
    if (managedModule->Assembly->IsLoaded())
    {
        OnAssemblyLoaded(managedModule->Assembly);
    }

    // Track C# assembly changes
    managedModule->Assembly->Loaded.Bind(OnAssemblyLoaded);
    managedModule->Assembly->Unloading.Bind(OnAssemblyUnloading);
}

void PluginManagerImpl::OnScriptsReloading()
{
    // When scripting is reloading (eg. for hot-reload in Editor) we have to deinitialize plugins (Scripting service destroys C# objects later on)
    bool changed = false;
    for (int32 i = EditorPlugins.Count() - 1; i >= 0 && EditorPlugins.Count() > 0; i--)
    {
        auto plugin = EditorPlugins[i];
        {
            PluginManagerService::InvokeDeinitialize(plugin);
            EditorPlugins.RemoveAtKeepOrder(i);
            changed = true;
        }
    }
    for (int32 i = GamePlugins.Count() - 1; i >= 0 && GamePlugins.Count() > 0; i--)
    {
        auto plugin = GamePlugins[i];
        {
            PluginManagerService::InvokeDeinitialize(plugin);
            GamePlugins.RemoveAtKeepOrder(i);
            changed = true;
        }
    }
    if (changed)
        PluginManager::PluginsChanged();
}

bool PluginManagerService::Init()
{
    // Process already loaded modules
    for (auto module : BinaryModule::GetModules())
    {
        OnBinaryModuleLoaded(module);
    }

    // Register for new binary modules load actions
    Scripting::BinaryModuleLoaded.Bind(&OnBinaryModuleLoaded);
    Scripting::ScriptsReloading.Bind(&OnScriptsReloading);

    return false;
}

void PluginManagerService::Dispose()
{
    Scripting::BinaryModuleLoaded.Unbind(&OnBinaryModuleLoaded);
    Scripting::ScriptsReloading.Unbind(&OnScriptsReloading);

    // Cleanup all plugins
    PROFILE_CPU_NAMED("Dispose Plugins");
    const int32 pluginsCount = EditorPlugins.Count() + GamePlugins.Count();
    if (pluginsCount == 0)
        return;
    LOG(Info, "Unloading {0} plugins", pluginsCount);
    for (int32 i = EditorPlugins.Count() - 1; i >= 0 && EditorPlugins.Count() > 0; i--)
    {
        auto plugin = EditorPlugins[i];
        InvokeDeinitialize(plugin);
        EditorPlugins.RemoveAtKeepOrder(i);
    }
    for (int32 i = GamePlugins.Count() - 1; i >= 0 && GamePlugins.Count() > 0; i--)
    {
        auto plugin = GamePlugins[i];
        InvokeDeinitialize(plugin);
        GamePlugins.RemoveAtKeepOrder(i);
    }
    PluginManager::PluginsChanged();
}

const Array<GamePlugin*>& PluginManager::GetGamePlugins()
{
    return GamePlugins;
}

const Array<Plugin*>& PluginManager::GetEditorPlugins()
{
    return EditorPlugins;
}

Plugin* PluginManager::GetPlugin(const StringView& name)
{
    for (Plugin* p : EditorPlugins)
    {
        if (p->GetDescription().Name == name)
            return p;
    }
    for (GamePlugin* gp : GamePlugins)
    {
        if (gp->GetDescription().Name == name)
            return gp;
    }
    return nullptr;
}

Plugin* PluginManager::GetPlugin(const MClass* type)
{
    CHECK_RETURN(type, nullptr);
    for (Plugin* p : EditorPlugins)
    {
        if (p->GetClass()->IsSubClassOf(type))
            return p;
    }
    for (GamePlugin* gp : GamePlugins)
    {
        if (gp->GetClass()->IsSubClassOf(type))
            return gp;
    }
    return nullptr;
}

Plugin* PluginManager::GetPlugin(const ScriptingTypeHandle& type)
{
    CHECK_RETURN(type, nullptr);
    for (Plugin* p : EditorPlugins)
    {
        if (p->Is(type))
            return p;
    }
    for (GamePlugin* gp : GamePlugins)
    {
        if (gp->Is(type))
            return gp;
    }
    return nullptr;
}

#if USE_EDITOR

void PluginManager::InitializeGamePlugins()
{
    PROFILE_CPU();
    for (int32 i = 0; i < GamePlugins.Count(); i++)
    {
        PluginManagerService::InvokeInitialize(GamePlugins[i]);
    }
}

void PluginManager::DeinitializeGamePlugins()
{
    PROFILE_CPU();
    for (int32 i = GamePlugins.Count() - 1; i >= 0; i--)
    {
        PluginManagerService::InvokeDeinitialize(GamePlugins[i]);
    }
}

#endif
