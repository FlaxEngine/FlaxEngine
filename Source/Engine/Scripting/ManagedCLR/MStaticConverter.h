// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/String.h"
#if USE_MONO
#include <mono/metadata/mono-debug.h>
#endif

/// <summary>
/// Class for converting mono classes and methods into usable form without instancing a class.
/// Mainly used for reflection where full object are not necessary.
/// </summary>
class MStaticConverter
{
public:

#if USE_MONO
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
#endif
};
