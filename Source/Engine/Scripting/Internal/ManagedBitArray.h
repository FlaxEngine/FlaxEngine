// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Collections/BitArray.h"
#include "Engine/Scripting/ManagedCLR/MCore.h"
#if USE_CSHARP

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
        MObject* instance = MCore::Object::New(bitArrayClass);
        CHECK_RETURN(instance, nullptr);
        int32 length = data.Count();
        void* params[1];
        params[0] = &length;
        mono_runtime_invoke(bitArrayCtor, instance, params, nullptr);
        const MField* arrayField = bitArrayClass->GetField("m_array");
        CHECK_RETURN(arrayField, nullptr);
        MArray* array = nullptr;
        arrayField->GetValue(instance, &array);
        CHECK_RETURN(array, nullptr);
        const int32 arrayLength = MCore::Array::GetLength(array);
        //mono_value_copy_array(array, 0, (void*)data.Get(), arrayLength);
        int32* arrayPtr = mono_array_addr(array, int32, 0);
        //Platform::MemoryCopy(mono_array_addr_with_size(array, sizeof(int32), 0), data.Get(), data.Count() / sizeof(byte));
        Platform::MemoryCopy(arrayPtr, data.Get(), length / sizeof(BitArray<AllocationType>::ItemType));
        return instance;
#else
        // Convert into bool[]
        MArray* array = MCore::Array::New(MCore::TypeCache::Boolean, data.Count());
        bool* arrayPtr = MCore::Array::GetAddress<bool>(array);
        for (int32 i = 0; i < data.Count(); i++)
            arrayPtr[i] = data[i];
        return (MObject*)array;
#endif
    }
};

#endif
