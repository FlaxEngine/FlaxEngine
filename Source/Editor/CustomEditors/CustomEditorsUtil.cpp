// Copyright (c) Wojciech Figat. All rights reserved.

#include "CustomEditorsUtil.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Types/DateTime.h"
#include "Engine/Core/Types/TimeSpan.h"
#include "Engine/Core/Types/Stopwatch.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Scripting/Scripting.h"
#include "Engine/Scripting/BinaryModule.h"
#include "Engine/Scripting/ManagedCLR/MField.h"
#include "Engine/Scripting/ManagedCLR/MAssembly.h"
#include "Engine/Scripting/ManagedCLR/MClass.h"
#include "Engine/Scripting/ManagedCLR/MUtils.h"
#include "FlaxEngine.Gen.h"

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
    MClass* DefaultEditor = nullptr;
    MClass* CustomEditor = nullptr;
    MType* CustomEditorType = nullptr;
};

Dictionary<MType*, Entry> Cache(512);

void OnAssemblyLoaded(MAssembly* assembly);
void OnAssemblyUnloading(MAssembly* assembly);
void OnBinaryModuleLoaded(BinaryModule* module);

MTypeObject* CustomEditorsUtil::GetCustomEditor(MTypeObject* refType)
{
    if (!refType)
        return nullptr;
    MType* type = INTERNAL_TYPE_OBJECT_GET(refType);
    Entry result;
    if (Cache.TryGet(type, result))
    {
        if (result.CustomEditorType)
            return INTERNAL_TYPE_GET_OBJECT(result.CustomEditorType);
        MClass* editor = result.CustomEditor ? result.CustomEditor : result.DefaultEditor;
        if (editor)
            return MUtils::GetType(editor);
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
    Stopwatch stopwatch;

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
        if (attribute == nullptr || MCore::Object::GetClass(attribute) != customEditorAttribute)
            continue;

        // Check if attribute references a valid class
        MTypeObject* refType = nullptr;
        customEditorTypeField->GetValue(attribute, &refType);
        if (refType == nullptr)
            continue;

        MType* type = INTERNAL_TYPE_OBJECT_GET(refType);
        if (type == nullptr)
            continue;
        MClass* typeClass = MCore::Type::GetClass(type);

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

            //LOG(Info, "Custom Editor {0} for type {1} (default: {2})", String(mclass->GetFullName()), MCore::Type::ToString(type), isDefault);
        }
        else if (typeClass)
        {
            auto& entry = Cache[mclass->GetType()];
            entry.CustomEditorType = type;

            //LOG(Info, "Custom Editor {0} for type {1}", String(typeClass->GetFullName()), String(mclass->GetFullName()));
        }
    }

    stopwatch.Stop();
    LOG(Info, "Assembly \'{0}\' scanned for custom editors in {1} ms", assembly->ToString(), stopwatch.GetMilliseconds());
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
        MClass* mClass = MCore::Type::GetClass(i->Key);
        if (mClass && mClass->GetAssembly() == assembly)
        {
            Cache.Remove(i);
        }
        else
        {
            if (i->Value.DefaultEditor && i->Value.DefaultEditor->GetAssembly() == assembly)
                i->Value.DefaultEditor = nullptr;
            if (i->Value.CustomEditor && i->Value.CustomEditor->GetAssembly() == assembly)
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
