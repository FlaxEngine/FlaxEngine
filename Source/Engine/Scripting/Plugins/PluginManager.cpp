// Copyright (c) Wojciech Figat. All rights reserved.

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
#include "Engine/Scripting/ManagedCLR/MField.h"

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
#include "Engine/Scripting/ManagedCLR/MException.h"
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
    bool Initialized = false;
    Array<GamePlugin*> GamePlugins;
    Array<Plugin*> EditorPlugins;

    void OnAssemblyLoaded(MAssembly* assembly);
    void OnAssemblyUnloading(MAssembly* assembly);
    void OnBinaryModuleLoaded(BinaryModule* module);
    void OnScriptsReloading();
    void InitializePlugins();
    void DeinitializePlugins();

    template<typename PluginType = Plugin>
    Array<PluginType*> SortPlugins(Array<PluginType*> plugins, MClass* pluginLoadOrderAttribute, MField* typeField)
    {
        // Sort plugins
        Array<PluginType*> newPlugins;
        for (int i = 0; i < plugins.Count(); i++)
        {
            PluginType* plugin = plugins[i];
            int32 insertIndex = -1;
            for (int j = 0; j < newPlugins.Count(); j++)
            {
                // Get first instance where a game plugin needs another one before it
                auto attribute = newPlugins[j]->GetClass()->GetAttribute(pluginLoadOrderAttribute);
                if (attribute == nullptr || MCore::Object::GetClass(attribute) != pluginLoadOrderAttribute)
                    continue;

                // Check if attribute references a valid class
                MTypeObject* refType = nullptr;
                typeField->GetValue(attribute, &refType);
                if (refType == nullptr)
                    continue;

                MType* type = INTERNAL_TYPE_OBJECT_GET(refType);
                if (type == nullptr)
                    continue;
                MClass* typeClass = MCore::Type::GetClass(type);

                if (plugin->GetClass() == typeClass)
                {
                    insertIndex = j;
                    break;
                }
            }
            if (insertIndex == -1)
                newPlugins.Add(plugin);
            else
                newPlugins.Insert(insertIndex, plugin);
        }
        return newPlugins;
    }
}

using namespace PluginManagerImpl;

class PluginManagerService : public EngineService
{
public:
    PluginManagerService()
        : EngineService(TEXT("Plugin Manager"), 100)
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
    const StringAnsiView typeName = plugin->GetType().GetName();
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
    const StringAnsiView typeName = plugin->GetType().GetName();
    PROFILE_CPU();
    ZoneName(typeName.Get(), typeName.Length());

    LOG(Info, "Unloading plugin {}", plugin->ToString());

    PluginManager::PluginUnloading(plugin);

    plugin->Deinitialize();
    plugin->_initialized = false;

    PluginManager::PluginUnloaded(plugin);
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
    bool loadedAnyPlugin = false;
    auto& classes = assembly->GetClasses();
    for (auto i = classes.Begin(); i.IsNotEnd(); ++i)
    {
        auto mclass = i->Value;

        // Skip invalid classes
        if (mclass->IsGeneric() || mclass->IsStatic() || mclass->IsAbstract())
            continue;

        if (mclass->IsSubClassOf(gamePluginClass)
#if USE_EDITOR
            || mclass->IsSubClassOf(editorPluginClass)
#endif
        )
        {
            auto plugin = (Plugin*)Scripting::NewObject(mclass);
            if (plugin)
            {
#if USE_EDITOR
                if (mclass->IsSubClassOf(editorPluginClass))
                {
                    EditorPlugins.Add(plugin);
                }
                else
#endif
                {
                    GamePlugins.Add((GamePlugin*)plugin);
                }
                loadedAnyPlugin = true;
            }
        }
    }

    // Send event and initialize newly added plugins (ignore during startup)
    if (loadedAnyPlugin && Initialized)
    {
        InitializePlugins();
        PluginManager::PluginsChanged();
    }
}

void PluginManagerImpl::OnAssemblyUnloading(MAssembly* assembly)
{
    bool changed = false;

    auto engineAssembly = ((NativeBinaryModule*)GetBinaryModuleFlaxEngine())->Assembly;
    auto pluginLoadOrderAttribute = engineAssembly->GetClass("FlaxEngine.PluginLoadOrderAttribute");
    auto beforeTypeField = pluginLoadOrderAttribute->GetField("DeinitializeBefore");
    ASSERT(beforeTypeField);

#if USE_EDITOR
    auto editorPlugins = SortPlugins(EditorPlugins, pluginLoadOrderAttribute, beforeTypeField);
    for (int32 i = editorPlugins.Count() - 1; i >= 0 && editorPlugins.Count() > 0; i--)
    {
        auto plugin = editorPlugins[i];
        if (plugin->GetType().ManagedClass->GetAssembly() == assembly)
        {
            PluginManagerService::InvokeDeinitialize(plugin);
            EditorPlugins.Remove(plugin);
            changed = true;
        }
    }
#endif

    auto gamePlugins = SortPlugins(GamePlugins, pluginLoadOrderAttribute, beforeTypeField);
    for (int32 i = gamePlugins.Count() - 1; i >= 0 && gamePlugins.Count() > 0; i--)
    {
        auto plugin = gamePlugins[i];
        if (plugin->GetType().ManagedClass->GetAssembly() == assembly)
        {
            PluginManagerService::InvokeDeinitialize(plugin);
            GamePlugins.Remove(plugin);
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
    DeinitializePlugins();
}

void PluginManagerImpl::InitializePlugins()
{
    if (EditorPlugins.Count() + GamePlugins.Count() == 0)
        return;
    PROFILE_CPU_NAMED("InitializePlugins");

    auto engineAssembly = ((NativeBinaryModule*)GetBinaryModuleFlaxEngine())->Assembly;
    auto pluginLoadOrderAttribute = engineAssembly->GetClass("FlaxEngine.PluginLoadOrderAttribute");
    auto afterTypeField = pluginLoadOrderAttribute->GetField("InitializeAfter");
    ASSERT(afterTypeField);

#if USE_EDITOR
    auto editorPlugins = SortPlugins(EditorPlugins, pluginLoadOrderAttribute, afterTypeField);
    for (auto plugin : editorPlugins)
    {
        PluginManagerService::InvokeInitialize(plugin);
    }
#else
    // Game plugins are managed via InitializeGamePlugins/DeinitializeGamePlugins by Editor for play mode
    auto gamePlugins = SortPlugins(GamePlugins, pluginLoadOrderAttribute, afterTypeField);
    for (auto plugin : gamePlugins)
    {
        PluginManagerService::InvokeInitialize(plugin);
    }
#endif
}

void PluginManagerImpl::DeinitializePlugins()
{
    if (EditorPlugins.Count() + GamePlugins.Count() == 0)
        return;
    PROFILE_CPU_NAMED("DeinitializePlugins");

    auto engineAssembly = ((NativeBinaryModule*)GetBinaryModuleFlaxEngine())->Assembly;
    auto pluginLoadOrderAttribute = engineAssembly->GetClass("FlaxEngine.PluginLoadOrderAttribute");
    auto beforeTypeField = pluginLoadOrderAttribute->GetField("DeinitializeBefore");
    ASSERT(beforeTypeField);

#if USE_EDITOR
    auto editorPlugins = SortPlugins(EditorPlugins, pluginLoadOrderAttribute, beforeTypeField);
    for (int32 i = editorPlugins.Count() - 1; i >= 0 && editorPlugins.Count() > 0; i--)
    {
        auto plugin = editorPlugins[i];
        PluginManagerService::InvokeDeinitialize(plugin);
        EditorPlugins.Remove(plugin);
    }
#endif

    auto gamePlugins = SortPlugins(GamePlugins, pluginLoadOrderAttribute, beforeTypeField);
    for (int32 i = gamePlugins.Count() - 1; i >= 0 && gamePlugins.Count() > 0; i--)
    {
        auto plugin = gamePlugins[i];
        PluginManagerService::InvokeDeinitialize(plugin);
        GamePlugins.Remove(plugin);
    }

    PluginManager::PluginsChanged();
}

bool PluginManagerService::Init()
{
    Initialized = false;

    // Process already loaded modules
    for (auto module : BinaryModule::GetModules())
    {
        OnBinaryModuleLoaded(module);
    }

    // Invoke plugins initialization for all of them
    InitializePlugins();

    // Register for new binary modules load actions
    Initialized = true;
    Scripting::BinaryModuleLoaded.Bind(&OnBinaryModuleLoaded);
    Scripting::ScriptsReloading.Bind(&OnScriptsReloading);

    return false;
}

void PluginManagerService::Dispose()
{
    // Unregister from new modules loading
    Initialized = false;
    Scripting::BinaryModuleLoaded.Unbind(&OnBinaryModuleLoaded);
    Scripting::ScriptsReloading.Unbind(&OnScriptsReloading);

    // Cleanup all plugins
    DeinitializePlugins();
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
#if USE_EDITOR
    for (Plugin* p : EditorPlugins)
    {
        if (p->GetDescription().Name == name)
            return p;
    }
#endif
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
#if USE_EDITOR
    for (Plugin* p : EditorPlugins)
    {
        if (p->GetClass()->IsSubClassOf(type))
            return p;
    }
#endif
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
#if USE_EDITOR
    for (Plugin* p : EditorPlugins)
    {
        if (p->Is(type))
            return p;
    }
#endif
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

    auto engineAssembly = ((NativeBinaryModule*)GetBinaryModuleFlaxEngine())->Assembly;
    auto pluginLoadOrderAttribute = engineAssembly->GetClass("FlaxEngine.PluginLoadOrderAttribute");
    auto afterTypeField = pluginLoadOrderAttribute->GetField("InitializeAfter");
    ASSERT(afterTypeField);

    auto gamePlugins = SortPlugins(GamePlugins, pluginLoadOrderAttribute, afterTypeField);
    for (int32 i = 0; i < gamePlugins.Count(); i++)
    {
        PluginManagerService::InvokeInitialize(gamePlugins[i]);
    }
}

void PluginManager::DeinitializeGamePlugins()
{
    PROFILE_CPU();

    auto engineAssembly = ((NativeBinaryModule*)GetBinaryModuleFlaxEngine())->Assembly;
    auto pluginLoadOrderAttribute = engineAssembly->GetClass("FlaxEngine.PluginLoadOrderAttribute");
    auto beforeTypeField = pluginLoadOrderAttribute->GetField("DeinitializeBefore");
    ASSERT(beforeTypeField);

    auto gamePlugins = SortPlugins(GamePlugins, pluginLoadOrderAttribute, beforeTypeField);
    for (int32 i = gamePlugins.Count() - 1; i >= 0; i--)
    {
        PluginManagerService::InvokeDeinitialize(gamePlugins[i]);
    }
}

#endif
