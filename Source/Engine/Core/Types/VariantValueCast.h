// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/Variant.h"
#include "Engine/Core/Templates.h"

// Helper utility to read Variant value with automatic casting to the template type (eg. cast ScriptingObject* to Actor*).
template<typename T, typename Enable = void>
struct TVariantValueCast
{
    FORCE_INLINE static T Cast(const Variant& v)
    {
        return (T)v;
    }
};

template<typename T>
struct TVariantValueCast<T*, typename TEnableIf<TIsBaseOf<class ScriptingObject, T>::Value>::Type>
{
    FORCE_INLINE static T* Cast(const Variant& v)
    {
        return ScriptingObject::Cast<T>((ScriptingObject*)v);
    }
};
