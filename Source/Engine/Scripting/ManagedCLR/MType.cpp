// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "MType.h"
#include "MUtils.h"

#if USE_MONO

#include <ThirdParty/mono-2.0/mono/metadata/mono-debug.h>

String MType::ToString() const
{
    return _monoType ? String(mono_type_get_name(_monoType)) : String::Empty;
}

bool MType::IsStruct() const
{
    ASSERT(_monoType);
    return mono_type_is_struct(_monoType) != 0;
}

bool MType::IsVoid() const
{
    ASSERT(_monoType);
    return mono_type_is_void(_monoType) != 0;
}

bool MType::IsPointer() const
{
    ASSERT(_monoType);
    return mono_type_is_pointer(_monoType) != 0;
}

bool MType::IsReference() const
{
    ASSERT(_monoType);
    return mono_type_is_reference(_monoType) != 0;
}

bool MType::IsByRef() const
{
    ASSERT(_monoType);
    return mono_type_is_byref(_monoType) != 0;
}

#endif
