// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Log.h"
#include "Engine/Core/Collections/BitArray.h"
#include "Engine/Scripting/ManagedCLR/MAssembly.h"
#include "Engine/Scripting/ManagedCLR/MClass.h"
#include "Engine/Scripting/ManagedCLR/MField.h"
#include "Engine/Scripting/BinaryModule.h"
#if USE_MONO
#include <mono/metadata/debug-helpers.h>
#include <mono/metadata/appdomain.h>
#include <mono/metadata/object.h>

struct ManagedBitArray
{
    template<typename AllocationType = HeapAllocation>
    static MObject* ToManaged(const BitArray<AllocationType>& data)
    {
#if 0
		// TODO: use actual System.Collections.BitArray for BitArray interop
        MClass* bitArrayClass = Assemblies::Corlib().Assembly->GetClass("System.Collections.BitArray");
        CHECK_RETURN(bitArrayClass, nullptr);
        MonoMethodDesc* desc = mono_method_desc_new(":.ctor(int)", 0);
        MonoMethod* bitArrayCtor = mono_method_desc_search_in_class(desc, bitArrayClass->GetNative());
        mono_method_desc_free(desc);
        CHECK_RETURN(bitArrayCtor, nullptr);
        MonoObject* instance = mono_object_new(mono_domain_get(), bitArrayClass->GetNative());
        CHECK_RETURN(instance, nullptr);
        int32 length = data.Count();
        void* params[1];
        params[0] = &length;
        mono_runtime_invoke(bitArrayCtor, instance, params, nullptr);
        const MField* arrayField = bitArrayClass->GetField("m_array");
        CHECK_RETURN(arrayField, nullptr);
        MonoArray* array = nullptr;
        arrayField->GetValue(instance, &array);
        CHECK_RETURN(array, nullptr);
        const int32 arrayLength = mono_array_length(array);
        //mono_value_copy_array(array, 0, (void*)data.Get(), arrayLength);
        int32* arrayPtr = mono_array_addr(array, int32, 0);
        //Platform::MemoryCopy(mono_array_addr_with_size(array, sizeof(int32), 0), data.Get(), data.Count() / sizeof(byte));
        Platform::MemoryCopy(arrayPtr, data.Get(), length / sizeof(BitArray<AllocationType>::ItemType));
        return instance;
#else
        // Convert into bool[]
        MonoArray* array = mono_array_new(mono_domain_get(), mono_get_boolean_class(), data.Count());
        for (int32 i = 0; i < data.Count(); i++)
        {
            mono_array_set(array, bool, i, data[i]);
        }
        return (MonoObject*)array;
#endif
    }
};

#endif
