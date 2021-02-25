// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "CustomEditorsUtil.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Types/DateTime.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Scripting/Scripting.h"
#include "Engine/Scripting/BinaryModule.h"
#include "Engine/Scripting/ManagedCLR/MField.h"
#include "Engine/Scripting/ManagedCLR/MAssembly.h"
#include "Engine/Scripting/ManagedCLR/MClass.h"
#include "Engine/Scripting/ManagedCLR/MUtils.h"
#include "FlaxEngine.Gen.h"
#include <ThirdParty/mono-2.0/mono/metadata/reflection.h>

#define TRACK_ASSEMBLY(assembly) \
	if (assembly->IsLoaded()) \
	{ \
		OnAssemblyLoaded(assembly); \
	} \
	assembly->Loaded.Bind(OnAssemblyLoaded); \
	assembly->Unloading.Bind(OnAssemblyUnloading)

class CustomEditorsUtilService : public EngineService
{
public:

    CustomEditorsUtilService()
        : EngineService(TEXT("Custom Editors Util"))
    {
    }

    bool Init() override;
};

CustomEditorsUtilService CustomEditorsUtilServiceInstance;

struct Entry
{
    MClass* DefaultEditor;
    MClass* CustomEditor;

    Entry()
    {
        DefaultEditor = nullptr;
        CustomEditor = nullptr;
    }
};

Dictionary<MonoType*, Entry> Cache(512);

void OnAssemblyLoaded(MAssembly* assembly);
void OnAssemblyUnloading(MAssembly* assembly);
void OnBinaryModuleLoaded(BinaryModule* module);

MonoReflectionType* CustomEditorsUtil::GetCustomEditor(MonoReflectionType* refType)
{
    MonoType* type = mono_reflection_type_get_type(refType);

    Entry result;
    if (Cache.TryGet(type, result))
    {
        const auto editor = result.CustomEditor ? result.CustomEditor : result.DefaultEditor;
        if (editor)
        {
            return MUtils::GetType(editor->GetNative());
        }
    }

    return nullptr;
}

bool CustomEditorsUtilService::Init()
{
    TRACK_ASSEMBLY(((NativeBinaryModule*)GetBinaryModuleFlaxEngine())->Assembly);
    Scripting::BinaryModuleLoaded.Bind(&OnBinaryModuleLoaded);

    return false;
}

void OnAssemblyLoaded(MAssembly* assembly)
{
    const auto startTime = DateTime::NowUTC();

    // Prepare FlaxEngine
    auto engineAssembly = ((NativeBinaryModule*)GetBinaryModuleFlaxEngine())->Assembly;
    if (!engineAssembly->IsLoaded())
    {
        LOG(Warning, "Cannot load custom editors meta for assembly {0} because FlaxEngine is not loaded.", assembly->ToString());
        return;
    }
    auto customEditorAttribute = engineAssembly->GetClass("FlaxEngine.CustomEditorAttribute");
    if (customEditorAttribute == nullptr)
    {
        LOG(Warning, "Missing CustomEditorAttribute class.");
        return;
    }
    auto customEditorTypeField = customEditorAttribute->GetField("Type");
    ASSERT(customEditorTypeField);
    const auto defaultEditorAttribute = engineAssembly->GetClass("FlaxEngine.DefaultEditorAttribute");
    if (defaultEditorAttribute == nullptr)
    {
        LOG(Warning, "Missing DefaultEditorAttribute class.");
        return;
    }
    const auto customEditor = engineAssembly->GetClass("FlaxEditor.CustomEditors.CustomEditor");
    if (customEditor == nullptr)
    {
        LOG(Warning, "Missing CustomEditor class.");
        return;
    }

    // Process all classes to find custom editors
    auto& classes = assembly->GetClasses();
    for (auto i = classes.Begin(); i.IsNotEnd(); ++i)
    {
        const auto mclass = i->Value;

        // Skip generic classes
        if (mclass->IsGeneric())
            continue;

        const auto attribute = mclass->GetAttribute(customEditorAttribute);
        if (attribute == nullptr || mono_object_get_class(attribute) != customEditorAttribute->GetNative())
            continue;

        // Check if attribute references a valid class
        MonoReflectionType* refType = nullptr;
        customEditorTypeField->GetValue(attribute, &refType);
        if (refType == nullptr)
            continue;

        MonoType* type = mono_reflection_type_get_type(refType);
        if (type == nullptr)
            continue;
        MonoClass* typeClass = mono_type_get_class(type);

        // Check if it's a custom editor class
        if (mclass->IsSubClassOf(customEditor))
        {
            auto& entry = Cache[type];

            const bool isDefault = mclass->HasAttribute(defaultEditorAttribute);
            if (isDefault)
            {
                entry.DefaultEditor = mclass;
            }
            else
            {
                entry.CustomEditor = mclass;
            }

            //LOG(Info, "Custom Editor {0} for type {1} (default: {2})", String(mclass->GetFullName()), String(mono_type_get_name(type)), isDefault);
        }
        else if (typeClass)
        {
            MClass* referencedClass = Scripting::FindClass(typeClass);
            if (referencedClass)
            {
                auto& entry = Cache[mono_class_get_type(mclass->GetNative())];
                entry.CustomEditor = referencedClass;

                //LOG(Info, "Custom Editor {0} for type {1}", String(referencedClass->GetFullName()), String(mclass->GetFullName()));
            }
        }
    }

    const auto endTime = DateTime::NowUTC();
    LOG(Info, "Assembly \'{0}\' scanned for custom editors in {1} ms", assembly->ToString(), (int32)(endTime - startTime).GetTotalMilliseconds());
}

void OnAssemblyUnloading(MAssembly* assembly)
{
    // Fast clear for editor unloading
    if (assembly == ((NativeBinaryModule*)GetBinaryModuleFlaxEngine())->Assembly)
    {
        Cache.Clear();
        return;
    }

    // Remove entries with user classes
    for (auto i = Cache.Begin(); i.IsNotEnd(); ++i)
    {
        MonoClass* monoClass = (MonoClass*)(void*)i->Key;

        if (assembly->GetClass(monoClass))
        {
            Cache.Remove(i);
        }
        else
        {
            if (i->Value.DefaultEditor && assembly->GetClass(i->Value.DefaultEditor->GetNative()))
                i->Value.DefaultEditor = nullptr;
            if (i->Value.CustomEditor && assembly->GetClass(i->Value.CustomEditor->GetNative()))
                i->Value.CustomEditor = nullptr;
        }
    }
}

void OnBinaryModuleLoaded(BinaryModule* module)
{
    auto managedModule = dynamic_cast<ManagedBinaryModule*>(module);
    if (managedModule)
    {
        TRACK_ASSEMBLY(managedModule->Assembly);
    }
}
