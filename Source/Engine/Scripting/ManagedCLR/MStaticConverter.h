// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/String.h"
#include <ThirdParty/mono-2.0/mono/metadata/mono-debug.h>

/// <summary>
/// Class for converting mono classes and methods into usable form without instancing a class.
/// Mainly used for reflection where full object are not necessary.
/// </summary>
class MStaticConverter
{
public:

    static MonoClass* GetMonoClassFromObject(MonoObject* monoObject)
    {
        ASSERT(monoObject);
        return mono_object_get_class(monoObject);
    }

    static Array<MonoClass*> GetMonoClassArrayFromObjects(Array<MonoObject*> monoObjectArray)
    {
        ASSERT(monoObjectArray.Count() > 0);
        Array<MonoClass*> array = Array<MonoClass*>(monoObjectArray.Count());
        for (auto i = 0; i < monoObjectArray.Count(); i++)
        {
            array.Add(GetMonoClassFromObject(monoObjectArray[i]));
        }
        return array;
    }

    static String GetClassName(MonoClass* monoClass)
    {
        ASSERT(monoClass);
        return String(mono_class_get_name(monoClass));
    }

    static Array<String> GetClassNames(Array<MonoClass*> monoClassArray)
    {
        ASSERT(monoClassArray.Count() > 0);
        Array<String> array = Array<String>(monoClassArray.Count());
        for (auto i = 0; i < monoClassArray.Count(); i++)
        {
            array.Add(GetClassName(monoClassArray[i]));
        }
        return array;
    }

    static String GetClassNamespace(MonoClass* monoClass)
    {
        ASSERT(monoClass);
        return String(mono_class_get_namespace(monoClass));
    }

    static Array<String> GetClassNamespaces(Array<MonoClass*> monoClassArray)
    {
        ASSERT(monoClassArray.Count() > 0);
        Array<String> array = Array<String>(monoClassArray.Count());
        for (auto i = 0; i < monoClassArray.Count(); i++)
        {
            array.Add(GetClassName(monoClassArray[i]));
        }
        return array;
    }
};
