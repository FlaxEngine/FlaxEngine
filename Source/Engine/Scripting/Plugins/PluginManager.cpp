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
/*
    auto engineAssembly = ((NativeBinaryModule*)GetBinaryModuleFlaxEngine())->Assembly;
    auto pluginLoadOrderAttribute = engineAssembly->GetClass("FlaxEngine.PluginLoadOrderAttribute");
    auto afterTypeField = pluginLoadOrderAttribute->GetField("InitializeAfter");
    ASSERT(afterTypeField);
*/
    if (!isEditor)
    {
        GamePlugins.Add((GamePlugin*)plugin);
    }
#if USE_EDITOR
    else
    {
        EditorPlugins.Add(plugin);
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

    auto engineAssembly = ((NativeBinaryModule*)GetBinaryModuleFlaxEngine())->Assembly;
    auto pluginLoadOrderAttribute = engineAssembly->GetClass("FlaxEngine.PluginLoadOrderAttribute");
    auto afterTypeField = pluginLoadOrderAttribute->GetField("DeinitializeBefore");
    ASSERT(afterTypeField);

    // Sort editor plugins
    Array<Plugin*> editorPlugins;
    for(int i = 0; i < EditorPlugins.Count(); i++)
    {
        Plugin* plugin = EditorPlugins[i];
        // Sort game plugin as needed
        int insertIndex = -1;
        for(int j = 0; j < editorPlugins.Count(); j++)
        {
            // Get first instance where a game plugin needs another one before it
            auto attribute = editorPlugins[j]->GetClass()->GetAttribute(pluginLoadOrderAttribute);
            if (attribute == nullptr || MCore::Object::GetClass(attribute) != pluginLoadOrderAttribute)
                continue;
            
            // Check if attribute references a valid class
            MTypeObject* refType = nullptr;
            afterTypeField->GetValue(attribute, &refType);
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
            editorPlugins.Add(plugin);
        else
            editorPlugins.Insert(insertIndex, plugin);
    }

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

    // Sort game plugins
    Array<GamePlugin*> gamePlugins;
    for(int i = 0; i < GamePlugins.Count(); i++)
    {
        GamePlugin* plugin = GamePlugins[i];
        // Sort game plugin as needed
        int insertIndex = -1;
        for(int j = 0; j < gamePlugins.Count(); j++)
        {
            // Get first instance where a game plugin needs another one before it
            auto attribute = gamePlugins[j]->GetClass()->GetAttribute(pluginLoadOrderAttribute);
            if (attribute == nullptr || MCore::Object::GetClass(attribute) != pluginLoadOrderAttribute)
                continue;
            
            // Check if attribute references a valid class
            MTypeObject* refType = nullptr;
            afterTypeField->GetValue(attribute, &refType);
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
            gamePlugins.Add(plugin);
        else
            gamePlugins.Insert(insertIndex, plugin);
    }

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
    bool changed = false;

    auto engineAssembly = ((NativeBinaryModule*)GetBinaryModuleFlaxEngine())->Assembly;
    auto pluginLoadOrderAttribute = engineAssembly->GetClass("FlaxEngine.PluginLoadOrderAttribute");
    auto afterTypeField = pluginLoadOrderAttribute->GetField("DeinitializeBefore");
    ASSERT(afterTypeField);

    // Sort editor plugins
    Array<Plugin*> editorPlugins;
    for(int i = 0; i < EditorPlugins.Count(); i++)
    {
        Plugin* plugin = EditorPlugins[i];
        // Sort game plugin as needed
        int insertIndex = -1;
        for(int j = 0; j < editorPlugins.Count(); j++)
        {
            // Get first instance where a game plugin needs another one before it
            auto attribute = editorPlugins[j]->GetClass()->GetAttribute(pluginLoadOrderAttribute);
            if (attribute == nullptr || MCore::Object::GetClass(attribute) != pluginLoadOrderAttribute)
                continue;
            
            // Check if attribute references a valid class
            MTypeObject* refType = nullptr;
            afterTypeField->GetValue(attribute, &refType);
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
            editorPlugins.Add(plugin);
        else
            editorPlugins.Insert(insertIndex, plugin);
    }

    for (int32 i = editorPlugins.Count() - 1; i >= 0 && editorPlugins.Count() > 0; i--)
    {
        auto plugin = editorPlugins[i];
        {
            PluginManagerService::InvokeDeinitialize(plugin);
            EditorPlugins.Remove(plugin);
            changed = true;
        }
    }

    // Sort game plugins
    Array<GamePlugin*> gamePlugins;
    for(int i = 0; i < GamePlugins.Count(); i++)
    {
        GamePlugin* plugin = GamePlugins[i];
        // Sort game plugin as needed
        int insertIndex = -1;
        for(int j = 0; j < gamePlugins.Count(); j++)
        {
            // Get first instance where a game plugin needs another one before it
            auto attribute = gamePlugins[j]->GetClass()->GetAttribute(pluginLoadOrderAttribute);
            if (attribute == nullptr || MCore::Object::GetClass(attribute) != pluginLoadOrderAttribute)
                continue;
            
            // Check if attribute references a valid class
            MTypeObject* refType = nullptr;
            afterTypeField->GetValue(attribute, &refType);
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
            gamePlugins.Add(plugin);
        else
            gamePlugins.Insert(insertIndex, plugin);
    }

    for (int32 i = gamePlugins.Count() - 1; i >= 0 && gamePlugins.Count() > 0; i--)
    {
        auto plugin = gamePlugins[i];
        {
            PluginManagerService::InvokeDeinitialize(plugin);
            GamePlugins.Remove(plugin);
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

    // Initialize plugins
    auto engineAssembly = ((NativeBinaryModule*)GetBinaryModuleFlaxEngine())->Assembly;
    auto pluginLoadOrderAttribute = engineAssembly->GetClass("FlaxEngine.PluginLoadOrderAttribute");
    auto afterTypeField = pluginLoadOrderAttribute->GetField("InitializeAfter");
    ASSERT(afterTypeField);
    
#if !USE_EDITOR
    // Sort game plugins
    Array<GamePlugin*> gamePlugins;
    for(int i = 0; i < GamePlugins.Count(); i++)
    {
        GamePlugin* plugin = GamePlugins[i];
        // Sort game plugin as needed
        int insertIndex = -1;
        for(int j = 0; j < gamePlugins.Count(); j++)
        {
            // Get first instance where a game plugin needs another one before it
            auto attribute = gamePlugins[j]->GetClass()->GetAttribute(pluginLoadOrderAttribute);
            if (attribute == nullptr || MCore::Object::GetClass(attribute) != pluginLoadOrderAttribute)
                continue;
            
            // Check if attribute references a valid class
            MTypeObject* refType = nullptr;
            afterTypeField->GetValue(attribute, &refType);
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
            gamePlugins.Add(plugin);
        else
            gamePlugins.Insert(insertIndex, plugin);
    }

    // Initalize game plugins
    for (auto plugin : gamePlugins)
    {
        PluginManagerService::InvokeInitialize(plugin);
    }
#endif
    
#if USE_EDITOR
    // Sort editor plugins
    Array<Plugin*> editorPlugins;
    for(int i = 0; i < EditorPlugins.Count(); i++)
    {
        Plugin* plugin = EditorPlugins[i];
        // Sort game plugin as needed
        int insertIndex = -1;
        for(int j = 0; j < editorPlugins.Count(); j++)
        {
            // Get first instance where a game plugin needs another one before it
            auto attribute = editorPlugins[j]->GetClass()->GetAttribute(pluginLoadOrderAttribute);
            if (attribute == nullptr || MCore::Object::GetClass(attribute) != pluginLoadOrderAttribute)
                continue;
            
            // Check if attribute references a valid class
            MTypeObject* refType = nullptr;
            afterTypeField->GetValue(attribute, &refType);
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
            editorPlugins.Add(plugin);
        else
            editorPlugins.Insert(insertIndex, plugin);
    }

    // Initialize editor plugins
    for (auto plugin : editorPlugins)
    {
        PluginManagerService::InvokeInitialize(plugin);
    }
#endif
    
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
    
    auto engineAssembly = ((NativeBinaryModule*)GetBinaryModuleFlaxEngine())->Assembly;
    auto pluginLoadOrderAttribute = engineAssembly->GetClass("FlaxEngine.PluginLoadOrderAttribute");
    auto afterTypeField = pluginLoadOrderAttribute->GetField("DeinitializeBefore");
    ASSERT(afterTypeField);

    // Sort editor plugins
    Array<Plugin*> editorPlugins;
    for(int i = 0; i < EditorPlugins.Count(); i++)
    {
        Plugin* plugin = EditorPlugins[i];
        // Sort game plugin as needed
        int insertIndex = -1;
        for(int j = 0; j < editorPlugins.Count(); j++)
        {
            // Get first instance where a game plugin needs another one before it
            auto attribute = editorPlugins[j]->GetClass()->GetAttribute(pluginLoadOrderAttribute);
            if (attribute == nullptr || MCore::Object::GetClass(attribute) != pluginLoadOrderAttribute)
                continue;
            
            // Check if attribute references a valid class
            MTypeObject* refType = nullptr;
            afterTypeField->GetValue(attribute, &refType);
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
            editorPlugins.Add(plugin);
        else
            editorPlugins.Insert(insertIndex, plugin);
    }
    
    for (int32 i = editorPlugins.Count() - 1; i >= 0 && editorPlugins.Count() > 0; i--)
    {
        auto plugin = editorPlugins[i];
        InvokeDeinitialize(plugin);
        EditorPlugins.Remove(plugin);
    }
    
    // Sort game plugins
    Array<GamePlugin*> gamePlugins;
    for(int i = 0; i < GamePlugins.Count(); i++)
    {
        GamePlugin* plugin = GamePlugins[i];
        // Sort game plugin as needed
        int insertIndex = -1;
        for(int j = 0; j < gamePlugins.Count(); j++)
        {
            // Get first instance where a game plugin needs another one before it
            auto attribute = gamePlugins[j]->GetClass()->GetAttribute(pluginLoadOrderAttribute);
            if (attribute == nullptr || MCore::Object::GetClass(attribute) != pluginLoadOrderAttribute)
                continue;
            
            // Check if attribute references a valid class
            MTypeObject* refType = nullptr;
            afterTypeField->GetValue(attribute, &refType);
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
            gamePlugins.Add(plugin);
        else
            gamePlugins.Insert(insertIndex, plugin);
    }
    for (int32 i = gamePlugins.Count() - 1; i >= 0 && gamePlugins.Count() > 0; i--)
    {
        auto plugin = gamePlugins[i];
        InvokeDeinitialize(plugin);
        GamePlugins.Remove(plugin);
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

    auto engineAssembly = ((NativeBinaryModule*)GetBinaryModuleFlaxEngine())->Assembly;
    auto pluginLoadOrderAttribute = engineAssembly->GetClass("FlaxEngine.PluginLoadOrderAttribute");
    auto afterTypeField = pluginLoadOrderAttribute->GetField("InitializeAfter");
    ASSERT(afterTypeField);
    
    // Sort game plugins
    Array<GamePlugin*> gamePlugins;
    for(int i = 0; i < GamePlugins.Count(); i++)
    {
        GamePlugin* plugin = GamePlugins[i];
        // Sort game plugin as needed
        int insertIndex = -1;
        for(int j = 0; j < gamePlugins.Count(); j++)
        {
            // Get first instance where a game plugin needs another one before it
            auto attribute = gamePlugins[j]->GetClass()->GetAttribute(pluginLoadOrderAttribute);
            if (attribute == nullptr || MCore::Object::GetClass(attribute) != pluginLoadOrderAttribute)
                continue;
            
            // Check if attribute references a valid class
            MTypeObject* refType = nullptr;
            afterTypeField->GetValue(attribute, &refType);
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
            gamePlugins.Add(plugin);
        else
            gamePlugins.Insert(insertIndex, plugin);
    }
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
    auto afterTypeField = pluginLoadOrderAttribute->GetField("DeinitializeBefore");
    ASSERT(afterTypeField);
    
    // Sort game plugins
    Array<GamePlugin*> gamePlugins;
    for(int i = 0; i < GamePlugins.Count(); i++)
    {
        GamePlugin* plugin = GamePlugins[i];
        // Sort game plugin as needed
        int insertIndex = -1;
        for(int j = 0; j < gamePlugins.Count(); j++)
        {
            // Get first instance where a game plugin needs another one before it
            auto attribute = gamePlugins[j]->GetClass()->GetAttribute(pluginLoadOrderAttribute);
            if (attribute == nullptr || MCore::Object::GetClass(attribute) != pluginLoadOrderAttribute)
                continue;
            
            // Check if attribute references a valid class
            MTypeObject* refType = nullptr;
            afterTypeField->GetValue(attribute, &refType);
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
            gamePlugins.Add(plugin);
        else
            gamePlugins.Insert(insertIndex, plugin);
    }
    for (int32 i = gamePlugins.Count() - 1; i >= 0; i--)
    {
        PluginManagerService::InvokeDeinitialize(gamePlugins[i]);
    }
}

#endif
