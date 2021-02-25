// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "PluginManager.h"
#include "FlaxEngine.Gen.h"
#include "Engine/Scripting/Scripting.h"
#include "Engine/Scripting/ScriptingType.h"
#include "Engine/Scripting/BinaryModule.h"
#include "Engine/Scripting/ManagedCLR/MAssembly.h"
#include "Engine/Scripting/ManagedCLR/MMethod.h"
#include "Engine/Scripting/ManagedCLR/MClass.h"
#include "Engine/Scripting/ManagedCLR/MUtils.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Debug/DebugLog.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Core/Log.h"

namespace PluginManagerImpl
{
    MMethod* Internal_LoadPlugin = nullptr;

    void LoadPlugin(MClass* klass, bool isEditor);
    void OnAssemblyLoaded(MAssembly* assembly);
    void OnAssemblyUnloading(MAssembly* assembly);
    void OnBinaryModuleLoaded(BinaryModule* module);
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
};

PluginManagerService PluginManagerServiceInstance;

void PluginManagerImpl::LoadPlugin(MClass* klass, bool isEditor)
{
    if (Internal_LoadPlugin == nullptr)
    {
        Internal_LoadPlugin = PluginManager::GetStaticClass()->GetMethod("Internal_LoadPlugin", 2);
        ASSERT(Internal_LoadPlugin);
    }

    MonoObject* exception = nullptr;
    void* params[2];
    params[0] = MUtils::GetType(klass->GetNative());
    params[1] = &isEditor;
    Internal_LoadPlugin->Invoke(nullptr, params, &exception);
    if (exception)
    {
        DebugLog::LogException(exception);
    }
}

void PluginManagerImpl::OnAssemblyLoaded(MAssembly* assembly)
{
    PROFILE_CPU_NAMED("Load Assembly Plugins");

    // Prepare FlaxEngine
    auto engineAssembly = ((NativeBinaryModule*)GetBinaryModuleFlaxEngine())->Assembly;
    if (!engineAssembly->IsLoaded())
    {
        LOG(Warning, "Cannot find plugin class types for assembly {0} because FlaxEngine is not loaded.", assembly->ToString());
        return;
    }
    const auto gamePluginClass = engineAssembly->GetClass("FlaxEngine.GamePlugin");
    if (gamePluginClass == nullptr)
    {
        LOG(Warning, "Missing GamePlugin class.");
        return;
    }
#if USE_EDITOR
    const auto editorPluginClass = engineAssembly->GetClass("FlaxEditor.EditorPlugin");
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
    // Cleanup plugins from this assembly
    const auto disposeMethod = PluginManager::GetStaticClass()->GetMethod("Internal_Dispose", 1);
    ASSERT(disposeMethod);
    MonoObject* exception = nullptr;
    void* params[1];
    params[0] = assembly->GetNative();
    disposeMethod->Invoke(nullptr, params, &exception);
    if (exception)
    {
        DebugLog::LogException(exception);
    }
}

void PluginManagerImpl::OnBinaryModuleLoaded(BinaryModule* module)
{
    // Skip special modules
    if (module == GetBinaryModuleFlaxEngine() ||
        module == GetBinaryModuleCorlib())
        return;

    // Skip non-managed modules
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

bool PluginManagerService::Init()
{
    // Process already loaded modules
    for (auto module : BinaryModule::GetModules())
    {
        OnBinaryModuleLoaded(module);
    }

    // Register for new binary modules load actions
    Scripting::BinaryModuleLoaded.Bind(&OnBinaryModuleLoaded);

    return false;
}

void PluginManagerService::Dispose()
{
    Scripting::BinaryModuleLoaded.Unbind(&OnBinaryModuleLoaded);

    PROFILE_CPU_NAMED("Dispose Plugins");

    // Cleanup all plugins
    const auto disposeMethod = PluginManager::GetStaticClass()->GetMethod("Internal_Dispose");
    ASSERT(disposeMethod);
    MonoObject* exception = nullptr;
    disposeMethod->Invoke(nullptr, nullptr, &exception);
    if (exception)
    {
        DebugLog::LogException(exception);
    }
}
