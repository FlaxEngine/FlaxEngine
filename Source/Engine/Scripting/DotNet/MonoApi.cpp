// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#include "CoreCLR.h"
#if USE_NETCORE
#include "Engine/Scripting/Types.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Graphics/RenderView.h"
#include "Engine/Core/Types/StringBuilder.h"
#include <ThirdParty/mono-2.0/mono/metadata/object.h>
#include <ThirdParty/mono-2.0/mono/metadata/appdomain.h>

#pragma warning(disable : 4297)

struct CoreCLRClass* GetClass(void* type);
struct CoreCLRClass* GetOrCreateClass(void* type);

struct _MonoString
{
    int32_t length;
    mono_unichar2 chars[MONO_ZERO_LEN_ARRAY];
};

struct CoreCLRAssembly;
struct CoreCLRMethod;
struct CoreCLRField;
struct CoreCLRCustomAttribute;
struct CoreCLRProperty;
struct CoreCLRClass;

// Structures used to pass information from runtime, must match with the structures in managed side
struct NativeClassDefinitions
{
    void* typeHandle;
    const char* name;
    const char* fullname;
    const char* namespace_;
    uint32 typeAttributes;
};

struct NativeMethodDefinitions
{
    const char* name;
    int numParameters;
    void* handle;
    uint32 methodAttributes;
};

struct NativeFieldDefinitions
{
    const char* name;
    void* fieldHandle;
    void* fieldType;
    uint32 fieldAttributes;
};

struct NativePropertyDefinitions
{
    const char* name;
    void* getterHandle;
    void* setterHandle;
    uint32 getterAttributes;
    uint32 setterAttributes;
};

struct ClassAttribute
{
    const char* name;
    void* attributeHandle;
    void* attributeTypeHandle;
};

Dictionary<void*, CoreCLRClass*> classHandles;
Dictionary<void*, CoreCLRAssembly*> assemblyHandles;
uint32 TypeTokenPool = 0;

struct CoreCLRAssembly
{
private:
    StringAnsi _name;
    StringAnsi _fullname;
    Array<CoreCLRClass*> _classes;
    void* _assemblyHandle;

public:
    CoreCLRAssembly(void* assemblyHandle, const char* name, const char* fullname)
    {
        static void* GetManagedClassesPtr = CoreCLR::GetStaticMethodPointer(TEXT("GetManagedClasses"));

        _assemblyHandle = assemblyHandle;
        _name = name;
        _fullname = fullname;

        NativeClassDefinitions* managedClasses;
        int classCount;
        CoreCLR::CallStaticMethod<void, void*, NativeClassDefinitions**, int*>(GetManagedClassesPtr, _assemblyHandle, &managedClasses, &classCount);
        for (int i = 0; i < classCount; i++)
        {
            CoreCLRClass* klass = New<CoreCLRClass>(managedClasses[i].typeHandle, StringAnsi(managedClasses[i].name), StringAnsi(managedClasses[i].fullname), StringAnsi(managedClasses[i].namespace_), managedClasses[i].typeAttributes, this);
            _classes.Add(klass);

            ASSERT(managedClasses[i].typeHandle != nullptr);
            classHandles.Add(managedClasses[i].typeHandle, klass);

            CoreCLR::Free((void*)managedClasses[i].name);
            CoreCLR::Free((void*)managedClasses[i].fullname);
            CoreCLR::Free((void*)managedClasses[i].namespace_);
        }
        CoreCLR::Free(managedClasses);

        assemblyHandles.Add(_assemblyHandle, this);
    }

    ~CoreCLRAssembly()
    {
        _classes.ClearDelete();
        assemblyHandles.Remove(_assemblyHandle);
    }

    void* GetHandle() const
    {
        return _assemblyHandle;
    }

    const StringAnsi& GetName() const
    {
        return _name;
    }

    const StringAnsi& GetFullname() const
    {
        return _fullname;
    }

    const Array<CoreCLRClass*>& GetClasses() const
    {
        return _classes;
    }

    void AddClass(CoreCLRClass* klass)
    {
        _classes.Add(klass);
    }
};

struct CoreCLRClass
{
private:
    StringAnsi _fullname;
    StringAnsi _name;
    StringAnsi _namespace;
    uint32 _typeAttributes;
    CoreCLRAssembly* _image;
    uint32 _typeToken;
    uint32 _size;
    void* _typeHandle;
    bool _cachedMethods = false;
    Array<CoreCLRMethod*> _methods;
    bool _cachedFields = false;
    Array<CoreCLRField*> _fields;
    bool _cachedAttributes = false;
    Array<CoreCLRCustomAttribute*> _attributes;
    bool _cachedProperties = false;
    Array<CoreCLRProperty*> _properties;
    bool _cachedInterfaces = false;
    Array<CoreCLRClass*> _interfaces;
    int _monoType;

public:
    CoreCLRClass(void* typeHandle, StringAnsi&& name, StringAnsi&& fullname, StringAnsi&& namespace_, uint32 typeAttributes, CoreCLRAssembly* image)
        : _fullname(MoveTemp(fullname))
        , _name(MoveTemp(name))
        , _namespace(MoveTemp(namespace_))
        , _typeAttributes(typeAttributes)
        , _image(image)
        , _typeHandle(typeHandle)
    {
        _typeToken = TypeTokenPool++;
        _monoType = 0;
        _size = 0;
    }

    ~CoreCLRClass()
    {
        _methods.ClearDelete();
        _fields.ClearDelete();
        _attributes.ClearDelete();
        _properties.ClearDelete();
        _interfaces.Clear();

        classHandles.Remove(_typeHandle);
    }

    uint32 GetAttributes() const
    {
        return _typeAttributes;
    }

    uint32 GetTypeToken() const
    {
        return _typeToken;
    }

    int GetSize()
    {
        if (_size != 0)
            return _size;
        uint32 align;
        _size = mono_class_value_size((MonoClass*)this, &align);
        return _size;
    }

    const StringAnsi& GetName() const
    {
        return _name;
    }

    const StringAnsi& GetFullname() const
    {
        return _fullname;
    }

    const StringAnsi& GetNamespace() const
    {
        return _namespace;
    }

    void* GetTypeHandle() const
    {
        return _typeHandle;
    }

    const CoreCLRAssembly* GetAssembly() const
    {
        return _image;
    }

    const Array<CoreCLRMethod*>& GetMethods()
    {
        if (_cachedMethods)
            return _methods;
        
        static void* GetClassMethodsPtr = CoreCLR::GetStaticMethodPointer(TEXT("GetClassMethods"));

        NativeMethodDefinitions* foundMethods;
        int numMethods;
        CoreCLR::CallStaticMethod<void, void*, NativeMethodDefinitions**, int*>(GetClassMethodsPtr, _typeHandle, &foundMethods, &numMethods);
        for (int i = 0; i < numMethods; i++)
        {
            CoreCLRMethod* method = New<CoreCLRMethod>(StringAnsi(foundMethods[i].name), foundMethods[i].numParameters, foundMethods[i].handle, foundMethods[i].methodAttributes, this);
            _methods.Add(method);

            CoreCLR::Free((void*)foundMethods[i].name);
        }
        CoreCLR::Free(foundMethods);

        _cachedMethods = true;
        return _methods;
    }

    const Array<CoreCLRField*>& GetFields()
    {
        if (_cachedFields)
            return _fields;

        static void* GetClassFieldsPtr = CoreCLR::GetStaticMethodPointer(TEXT("GetClassFields"));

        NativeFieldDefinitions* foundFields;
        int numFields;
        CoreCLR::CallStaticMethod<void, void*, NativeFieldDefinitions**, int*>(GetClassFieldsPtr, _typeHandle, &foundFields, &numFields);
        for (int i = 0; i < numFields; i++)
        {
            CoreCLRField* field = New<CoreCLRField>(StringAnsi(foundFields[i].name), foundFields[i].fieldHandle, foundFields[i].fieldType, foundFields[i].fieldAttributes, this);
            _fields.Add(field);

            CoreCLR::Free((void*)foundFields[i].name);
        }
        CoreCLR::Free(foundFields);

        _cachedFields = true;
        return _fields;
    }

    const Array<CoreCLRProperty*>& GetProperties()
    {
        if (_cachedProperties)
            return _properties;

        static void* GetClassPropertiesPtr = CoreCLR::GetStaticMethodPointer(TEXT("GetClassProperties"));

        NativePropertyDefinitions* foundProperties;
        int numProperties;
        CoreCLR::CallStaticMethod<void, void*, NativePropertyDefinitions**, int*>(GetClassPropertiesPtr, _typeHandle, &foundProperties, &numProperties);
        for (int i = 0; i < numProperties; i++)
        {
            const NativePropertyDefinitions& foundProp = foundProperties[i];
            CoreCLRProperty* prop = New<CoreCLRProperty>(StringAnsi(foundProp.name), foundProp.getterHandle, foundProp.setterHandle, foundProp.getterAttributes, foundProp.setterAttributes, this);
            _properties.Add(prop);

            CoreCLR::Free((void*)foundProperties[i].name);
        }
        CoreCLR::Free(foundProperties);

        _cachedProperties = true;
        return _properties;
    }

    const Array<CoreCLRCustomAttribute*>& GetCustomAttributes()
    {
        if (_cachedAttributes)
            return _attributes;

        static void* GetClassAttributesPtr = CoreCLR::GetStaticMethodPointer(TEXT("GetClassAttributes"));

        ClassAttribute* foundAttributes;
        int numAttributes;
        CoreCLR::CallStaticMethod<void, void*, ClassAttribute**, int*>(GetClassAttributesPtr, _typeHandle, &foundAttributes, &numAttributes);
        for (int i = 0; i < numAttributes; i++)
        {
            CoreCLRClass* attributeClass = GetClass(foundAttributes[i].attributeTypeHandle);
            CoreCLRCustomAttribute* attribute = New<CoreCLRCustomAttribute>(StringAnsi(foundAttributes[i].name), foundAttributes[i].attributeHandle, this, attributeClass);
            _attributes.Add(attribute);

            CoreCLR::Free((void*)foundAttributes[i].name);
        }
        CoreCLR::Free(foundAttributes);

        _cachedAttributes = true;
        return _attributes;
    }

    const Array<CoreCLRClass*>& GetInterfaces()
    {
        if (_cachedInterfaces)
            return _interfaces;

        static void* GetClassInterfacesPtr = CoreCLR::GetStaticMethodPointer(TEXT("GetClassInterfaces"));

        void** foundInterfaces;
        int numInterfaces;
        CoreCLR::CallStaticMethod<void, void*, void***, int*>(GetClassInterfacesPtr, _typeHandle, &foundInterfaces, &numInterfaces);
        for (int i = 0; i < numInterfaces; i++)
        {
            CoreCLRClass* interfaceClass = classHandles[foundInterfaces[i]];
            _interfaces.Add(interfaceClass);
        }
        CoreCLR::Free(foundInterfaces);

        _cachedInterfaces = true;
        return _interfaces;
    }

    int GetMonoType()
    {
        if (_monoType == 0)
        {
            static void* GetTypeMonoTypeEnumPtr = CoreCLR::GetStaticMethodPointer(TEXT("GetTypeMonoTypeEnum"));
            _monoType = CoreCLR::CallStaticMethod<int, void*>(GetTypeMonoTypeEnumPtr, _typeHandle);
        }
        return _monoType;
    }
};

struct CoreCLRMethod
{
private:
    StringAnsi _name;
    int _numParams;
    CoreCLRClass* _class;
    void* _methodHandle;
    bool _cachedParameters = false;
    Array<void*> _parameterTypes;
    void* _returnType;
    uint32 _methodAttributes;

public:
    CoreCLRMethod(StringAnsi&& name, int numParams, void* methodHandle, uint32 flags, CoreCLRClass* klass)
        : _name(MoveTemp(name))
        , _numParams(numParams)
        , _class(klass)
        , _methodHandle(methodHandle)
        , _methodAttributes(flags)
    {
        _returnType = nullptr;
    }

    const StringAnsi& GetName() const
    {
        return _name;
    }

    const CoreCLRClass* GetClass() const
    {
        return _class;
    }

    uint32 GetAttributes() const
    {
        return _methodAttributes;
    }

    int GetNumParameters() const
    {
        return _numParams;
    }

    void* GetMethodHandle() const
    {
        return _methodHandle;
    }

    const Array<void*>& GetParameterTypes()
    {
        if (!_cachedParameters)
            CacheParameters();
        return _parameterTypes;
    }

    void* GetReturnType()
    {
        if (!_cachedParameters)
            CacheParameters();
        return _returnType;
    }

private:
    void CacheParameters()
    {
        static void* GetMethodReturnTypePtr = CoreCLR::GetStaticMethodPointer(TEXT("GetMethodReturnType"));
        static void* GetMethodParameterTypesPtr = CoreCLR::GetStaticMethodPointer(TEXT("GetMethodParameterTypes"));

        _returnType = CoreCLR::CallStaticMethod<void*, void*>(GetMethodReturnTypePtr, _methodHandle);

        void** parameterTypeHandles;
        CoreCLR::CallStaticMethod<void, void*, void***>(GetMethodParameterTypesPtr, _methodHandle, &parameterTypeHandles);
        _parameterTypes.Set(parameterTypeHandles, _numParams);
        CoreCLR::Free(parameterTypeHandles);

        _cachedParameters = true;
    }
};

struct CoreCLRField
{
private:
    StringAnsi _name;
    CoreCLRClass* _class;
    void* _fieldHandle;
    void* _fieldType;
    uint32 _fieldAttributes;

public:
    CoreCLRField(StringAnsi name, void* fieldHandle, void* fieldType, uint32 fieldAttributes, CoreCLRClass* klass)
        :_name(name), _fieldHandle(fieldHandle), _fieldType(fieldType), _fieldAttributes(fieldAttributes), _class(klass)
    {
    }

    const StringAnsi& GetName() const
    {
        return _name;
    }

    void* GetType() const
    {
        return _fieldType;
    }

    const CoreCLRClass* GetClass() const
    {
        return _class;
    }

    uint32 GetAttributes() const
    {
        return _fieldAttributes;
    }

    void* GetHandle() const
    {
        return _fieldHandle;
    }
};

struct CoreCLRProperty
{
private:
    StringAnsi _name;
    CoreCLRClass* _class;
    CoreCLRMethod* _getMethod;
    CoreCLRMethod* _setMethod;

public:
    CoreCLRProperty(StringAnsi&& name, void* getter, void* setter, uint32 getterAttributes, uint32 setterAttributes, CoreCLRClass* klass)
        : _name(MoveTemp(name))
        , _class(klass)
    {
        if (getter != nullptr)
            _getMethod = New<CoreCLRMethod>(StringAnsi(_name + "Get"), 1, getter, getterAttributes, klass);
        else
            _getMethod = nullptr;
        if (setter != nullptr)
            _setMethod = New<CoreCLRMethod>(StringAnsi(_name + "Set"), 1, setter, setterAttributes, klass);
        else
            _setMethod = nullptr;
    }

    const StringAnsi& GetName() const
    {
        return _name;
    }

    const CoreCLRClass* GetClass() const
    {
        return _class;
    }

    const CoreCLRMethod* GetGetMethod() const
    {
        return _getMethod;
    }

    const CoreCLRMethod* GetSetMethod() const
    {
        return _setMethod;
    }
};

struct CoreCLRCustomAttribute
{
private:
    StringAnsi _name;
    void* _handle;
    CoreCLRClass* _owningClass;
    CoreCLRClass* _attributeClass;

public:
    CoreCLRCustomAttribute(StringAnsi&& name, void* handle, CoreCLRClass* owningClass, CoreCLRClass* attributeClass)
        : _name(MoveTemp(name))
        , _handle(handle)
        , _owningClass(owningClass)
        , _attributeClass(attributeClass)
    {
    }

    void* GetHandle() const
    {
        return _handle;
    }

    const CoreCLRClass* GetClass() const
    {
        return _attributeClass;
    }
};

CoreCLRAssembly* GetAssembly(void* assemblyHandle)
{
    CoreCLRAssembly* assembly;
    if (assemblyHandles.TryGet(assemblyHandle, assembly))
        return assembly;
    return nullptr;
}

CoreCLRClass* GetClass(void* type)
{
    CoreCLRClass* klass = nullptr;
    classHandles.TryGet(type, klass);
    return nullptr;
}

CoreCLRClass* GetOrCreateClass(void* type)
{
    CoreCLRClass* klass;
    if (!classHandles.TryGet(type, klass))
    {
        static void* GetManagedClassFromTypePtr = CoreCLR::GetStaticMethodPointer(TEXT("GetManagedClassFromType"));

        NativeClassDefinitions classInfo;
        void* assemblyHandle;
        CoreCLR::CallStaticMethod<void, void*, void*>(GetManagedClassFromTypePtr, type, &classInfo, &assemblyHandle);
        CoreCLRAssembly* image = GetAssembly(assemblyHandle);
        klass = New<CoreCLRClass>(classInfo.typeHandle, StringAnsi(classInfo.name), StringAnsi(classInfo.fullname), StringAnsi(classInfo.namespace_), classInfo.typeAttributes, image);
        if (image != nullptr)
            image->AddClass(klass);

        if (type != classInfo.typeHandle)
            CoreCLR::CallStaticMethod<void, void*, void*>(GetManagedClassFromTypePtr, type, &classInfo);
        classHandles.Add(classInfo.typeHandle, klass);

        CoreCLR::Free((void*)classInfo.name);
        CoreCLR::Free((void*)classInfo.fullname);
        CoreCLR::Free((void*)classInfo.namespace_);
    }
    ASSERT(klass != nullptr);
    return klass;
}

MGCHandle CoreCLR::NewGCHandle(void* obj, bool pinned)
{
    static void* NewGCHandlePtr = CoreCLR::GetStaticMethodPointer(TEXT("NewGCHandle"));
    return (MGCHandle)CoreCLR::CallStaticMethod<void*, void*, bool>(NewGCHandlePtr, obj, pinned);
}

MGCHandle CoreCLR::NewGCHandleWeakref(void* obj, bool track_resurrection)
{
    static void* NewGCHandleWeakrefPtr = CoreCLR::GetStaticMethodPointer(TEXT("NewGCHandleWeakref"));
    return (MGCHandle)CoreCLR::CallStaticMethod<void*, void*, bool>(NewGCHandleWeakrefPtr, obj, track_resurrection);
}

void* CoreCLR::GetGCHandleTarget(const MGCHandle& MGCHandle)
{
    return (void*)MGCHandle;
}

void CoreCLR::FreeGCHandle(const MGCHandle& MGCHandle)
{
    static void* FreeGCHandlePtr = CoreCLR::GetStaticMethodPointer(TEXT("FreeGCHandle"));
    CoreCLR::CallStaticMethod<void, void*>(FreeGCHandlePtr, (void*)MGCHandle);
}

const char* CoreCLR::GetClassFullname(void* klass)
{
    return ((CoreCLRClass*)klass)->GetFullname().Get();
}

bool CoreCLR::HasCustomAttribute(void* klass, void* attribClass)
{
    return CoreCLR::GetCustomAttribute(klass, attribClass) != nullptr;
}

bool CoreCLR::HasCustomAttribute(void* klass)
{
    return CoreCLR::GetCustomAttribute(klass, nullptr) != nullptr;
}

void* CoreCLR::GetCustomAttribute(void* klass, void* attribClass)
{
    static void* GetCustomAttributePtr = CoreCLR::GetStaticMethodPointer(TEXT("GetCustomAttribute"));
    return CoreCLR::CallStaticMethod<void*, void*, void*>(GetCustomAttributePtr, ((CoreCLRClass*)klass)->GetTypeHandle(), ((CoreCLRClass*)attribClass)->GetTypeHandle());
}

Array<MObject*> CoreCLR::GetCustomAttributes(void* klass)
{
    const Array<CoreCLRCustomAttribute*>& attribs = ((CoreCLRClass*)klass)->GetCustomAttributes();
    Array<MObject*> attributes;
    attributes.Resize(attribs.Count(), false);
    for (int32 i = 0; i < attribs.Count(); i++)
        attributes.Add((MObject*)attribs[i]->GetHandle());
    return attributes;
}

/*
 * loader.h
*/

MONO_API MonoMethodSignature* mono_method_signature(MonoMethod* method)
{
    return (MonoMethodSignature*)method;
}

MONO_API const char* mono_method_get_name(MonoMethod* method)
{
    return ((CoreCLRMethod*)method)->GetName().Get();
}

MONO_API MonoClass* mono_method_get_class(MonoMethod* method)
{
    return (MonoClass*)((CoreCLRMethod*)method)->GetClass();
}

MONO_API uint32 mono_method_get_flags(MonoMethod* method, uint32* iflags)
{
    return ((CoreCLRMethod*)method)->GetAttributes();
}

MONO_API MONO_RT_EXTERNAL_ONLY void mono_add_internal_call(const char* name, const void* method)
{
    // Unused, CoreCRL uses exported symbols linkage via LibraryImport atttribute from C#
}

/*
 * objects.h
*/

MONO_API mono_unichar2* mono_string_chars(MonoString* s)
{
    static void* GetStringPointerPtr = CoreCLR::GetStaticMethodPointer(TEXT("GetStringPointer"));
    _MonoString* str = (_MonoString*)CoreCLR::CallStaticMethod<void*, void*>(GetStringPointerPtr, s);
    return str->chars;
}

MONO_API int mono_string_length(MonoString* s)
{
    static void* GetStringPointerPtr = CoreCLR::GetStaticMethodPointer(TEXT("GetStringPointer"));
    _MonoString* str = (_MonoString*)CoreCLR::CallStaticMethod<void*, void*>(GetStringPointerPtr, s);
    return str->length;
}

MONO_API MONO_RT_EXTERNAL_ONLY MonoObject* mono_object_new(MonoDomain* domain, MonoClass* klass)
{
    static void* NewObjectPtr = CoreCLR::GetStaticMethodPointer(TEXT("NewObject"));
    return (MonoObject*)CoreCLR::CallStaticMethod<void*, void*>(NewObjectPtr, ((CoreCLRClass*)klass)->GetTypeHandle());
}

MONO_API MONO_RT_EXTERNAL_ONLY MonoArray* mono_array_new(MonoDomain* domain, MonoClass* eclass, uintptr_t n)
{
    static void* NewArrayPtr = CoreCLR::GetStaticMethodPointer(TEXT("NewArray"));
    return (MonoArray*)CoreCLR::CallStaticMethod<void*, void*, long long>(NewArrayPtr, ((CoreCLRClass*)eclass)->GetTypeHandle(), n);
}

MONO_API char* mono_array_addr_with_size(MonoArray* array, int size, uintptr_t idx)
{
    static void* GetArrayPointerToElementPtr = CoreCLR::GetStaticMethodPointer(TEXT("GetArrayPointerToElement"));
    return (char*)CoreCLR::CallStaticMethod<void*, void*, int, int>(GetArrayPointerToElementPtr, array, size, (int)idx);
}

MONO_API uintptr_t mono_array_length(MonoArray* array)
{
    static void* GetArrayLengthPtr = CoreCLR::GetStaticMethodPointer(TEXT("GetArrayLength"));
    return CoreCLR::CallStaticMethod<int, void*>(GetArrayLengthPtr, array);
}

MONO_API MonoString* mono_string_empty(MonoDomain* domain)
{
    static void* GetStringEmptyPtr = CoreCLR::GetStaticMethodPointer(TEXT("GetStringEmpty"));
    return (MonoString*)CoreCLR::CallStaticMethod<void*>(GetStringEmptyPtr);
}

MONO_API MONO_RT_EXTERNAL_ONLY MonoString* mono_string_new_utf16(MonoDomain* domain, const mono_unichar2* text, int32_t len)
{
    static void* NewStringUTF16Ptr = CoreCLR::GetStaticMethodPointer(TEXT("NewStringUTF16"));
    return (MonoString*)CoreCLR::CallStaticMethod<void*, const mono_unichar2*, int>(NewStringUTF16Ptr, text, len);
}

MONO_API MONO_RT_EXTERNAL_ONLY MonoString* mono_string_new(MonoDomain* domain, const char* text)
{
    static void* NewStringPtr = CoreCLR::GetStaticMethodPointer(TEXT("NewString"));
    return (MonoString*)CoreCLR::CallStaticMethod<void*, const char*>(NewStringPtr, text);
}

MONO_API MONO_RT_EXTERNAL_ONLY MonoString* mono_string_new_len(MonoDomain* domain, const char* text, unsigned int length)
{
    static void* NewStringLengthPtr = CoreCLR::GetStaticMethodPointer(TEXT("NewStringLength"));
    return (MonoString*)CoreCLR::CallStaticMethod<void*, const char*, int>(NewStringLengthPtr, text, length);
}

MONO_API MONO_RT_EXTERNAL_ONLY char* mono_string_to_utf8(MonoString* string_obj)
{
    static void* GetStringPointerPtr = CoreCLR::GetStaticMethodPointer(TEXT("GetStringPointer"));
    _MonoString* monoString = (_MonoString*)CoreCLR::CallStaticMethod<void*, void*>(GetStringPointerPtr, string_obj);

    auto len = monoString->length;
    ASSERT(len >= 0)
    char* str = (char*)CoreCLR::Allocate(sizeof(char) * (len + 1));
    StringUtils::ConvertUTF162UTF8((Char*)monoString->chars, str, len, len);
    str[len] = 0;

    return str;
}

MONO_API MONO_RT_EXTERNAL_ONLY MonoString* mono_object_to_string(MonoObject* obj, MonoObject** exc)
{
    ASSERT(false);
}

MONO_API int mono_object_hash(MonoObject* obj)
{
    ASSERT(false);
}

MONO_API MONO_RT_EXTERNAL_ONLY MonoObject* mono_value_box(MonoDomain* domain, MonoClass* klass, void* val)
{
    static void* BoxValuePtr = CoreCLR::GetStaticMethodPointer(TEXT("BoxValue"));
    return (MonoObject*)CoreCLR::CallStaticMethod<void*, void*, void*>(BoxValuePtr, ((CoreCLRClass*)klass)->GetTypeHandle(), val);
}

MONO_API void mono_value_copy(void* dest, /*const*/ void* src, MonoClass* klass)
{
    Platform::MemoryCopy(dest, src, ((CoreCLRClass*)klass)->GetSize());
}

MONO_API MONO_RT_EXTERNAL_ONLY MonoClass* mono_object_get_class(MonoObject* obj)
{
    static void* GetObjectTypePtr = CoreCLR::GetStaticMethodPointer(TEXT("GetObjectType"));
    void* classHandle = CoreCLR::CallStaticMethod<void*, void*>(GetObjectTypePtr, obj);

    CoreCLRClass* klass = GetOrCreateClass((void*)classHandle);
    ASSERT(klass != nullptr)
    return (MonoClass*)klass;
}

MONO_API void* mono_object_unbox(MonoObject* obj)
{
    static void* UnboxValuePtr = CoreCLR::GetStaticMethodPointer(TEXT("UnboxValue"));
    return CoreCLR::CallStaticMethod<void*, void*>(UnboxValuePtr, obj);
}

MONO_API MONO_RT_EXTERNAL_ONLY void mono_raise_exception(MonoException* ex)
{
    static void* RaiseExceptionPtr = CoreCLR::GetStaticMethodPointer(TEXT("RaiseException"));
    CoreCLR::CallStaticMethod<void*, void*>(RaiseExceptionPtr, ex);
}

MONO_API MONO_RT_EXTERNAL_ONLY void mono_runtime_object_init(MonoObject* this_obj)
{
    static void* ObjectInitPtr = CoreCLR::GetStaticMethodPointer(TEXT("ObjectInit"));
    CoreCLR::CallStaticMethod<void, void*>(ObjectInitPtr, this_obj);
}

MONO_API MonoMethod* mono_object_get_virtual_method(MonoObject* obj, MonoMethod* method)
{
    return method;
}

MONO_API MONO_RT_EXTERNAL_ONLY MonoObject* mono_runtime_invoke(MonoMethod* method, void* obj, void** params, MonoObject** exc)
{
    static void* InvokeMethodPtr = CoreCLR::GetStaticMethodPointer(TEXT("InvokeMethod"));
    return (MonoObject*)CoreCLR::CallStaticMethod<void*, void*, void*, void*, void*>(InvokeMethodPtr, obj, ((CoreCLRMethod*)method)->GetMethodHandle(), params, exc);
}

MONO_API MONO_RT_EXTERNAL_ONLY void* mono_method_get_unmanaged_thunk(MonoMethod* method)
{
    static void* GetMethodUnmanagedFunctionPointerPtr = CoreCLR::GetStaticMethodPointer(TEXT("GetMethodUnmanagedFunctionPointer"));
    return CoreCLR::CallStaticMethod<void*, void*>(GetMethodUnmanagedFunctionPointerPtr, ((CoreCLRMethod*)method)->GetMethodHandle());
}

MONO_API void mono_field_set_value(MonoObject* obj, MonoClassField* field, void* value)
{
    static void* FieldSetValuePtr = CoreCLR::GetStaticMethodPointer(TEXT("FieldSetValue"));
    CoreCLR::CallStaticMethod<void, void*, void*, void*>(FieldSetValuePtr, obj, ((CoreCLRField*)field)->GetHandle(), value);
}

MONO_API void mono_field_get_value(MonoObject* obj, MonoClassField* field, void* value)
{
    static void* FieldGetValuePtr = CoreCLR::GetStaticMethodPointer(TEXT("FieldGetValue"));
    CoreCLR::CallStaticMethod<void, void*, void*, void*>(FieldGetValuePtr, obj, ((CoreCLRField*)field)->GetHandle(), value);
}

MONO_API MONO_RT_EXTERNAL_ONLY MonoObject* mono_field_get_value_object(MonoDomain* domain, MonoClassField* field, MonoObject* obj)
{
    ASSERT(false);
}

MONO_API MONO_RT_EXTERNAL_ONLY void mono_property_set_value(MonoProperty* prop, void* obj, void** params, MonoObject** exc)
{
    ASSERT(false);
}

MONO_API MONO_RT_EXTERNAL_ONLY MonoObject* mono_property_get_value(MonoProperty* prop, void* obj, void** params, MonoObject** exc)
{
    ASSERT(false);
}

MONO_API void mono_gc_wbarrier_set_field(MonoObject* obj, void* field_ptr, MonoObject* value)
{
    ASSERT(false);
}
MONO_API void mono_gc_wbarrier_set_arrayref(MonoArray* arr, void* slot_ptr, MonoObject* value)
{
    static void* SetArrayValueReferencePtr = CoreCLR::GetStaticMethodPointer(TEXT("SetArrayValueReference"));
    CoreCLR::CallStaticMethod<void, void*, void*, void*>(SetArrayValueReferencePtr, arr, slot_ptr, value);
}
MONO_API void mono_gc_wbarrier_generic_store(void* ptr, MonoObject* value)
{
    // Ignored
    *((void**)ptr) = value;
}
MONO_API void mono_gc_wbarrier_value_copy(void* dest, /*const*/ void* src, int count, MonoClass* klass)
{
    // Ignored
    int size = ((CoreCLRClass*)klass)->GetSize();
    memcpy(dest, src, count * size);
}

/*
 * appdomain.h
*/

MonoDomain* currentDomain = nullptr;

MONO_API MonoDomain* mono_domain_get(void)
{
    return currentDomain;
}

MONO_API mono_bool mono_domain_set(MonoDomain* domain, mono_bool force)
{
    currentDomain = domain;
    return true;
}

MONO_API MonoAssembly* mono_domain_assembly_open(MonoDomain* domain, const char* path)
{
    const char* name;
    const char* fullname;
    static void* LoadAssemblyFromPathPtr = CoreCLR::GetStaticMethodPointer(TEXT("LoadAssemblyFromPath"));
    void* assemblyHandle = CoreCLR::CallStaticMethod<void*, const char*, const char**, const char**>(LoadAssemblyFromPathPtr, path, &name, &fullname);
    CoreCLRAssembly* assembly = New<CoreCLRAssembly>(assemblyHandle, name, fullname);

    CoreCLR::Free((void*)name);
    CoreCLR::Free((void*)fullname);

    return (MonoAssembly*)assembly;
}

static CoreCLRAssembly* corlibimage = nullptr;

MONO_API MonoImage* mono_get_corlib(void)
{
    if (corlibimage == nullptr)
    {
        const char* name;
        const char* fullname;
        static void* GetAssemblyByNamePtr = CoreCLR::GetStaticMethodPointer(TEXT("GetAssemblyByName"));
        void* assemblyHandle = CoreCLR::CallStaticMethod<void*, const char*, const char**, const char**>(GetAssemblyByNamePtr, "System.Private.CoreLib", &name, &fullname);
        corlibimage = New<CoreCLRAssembly>(assemblyHandle, name, fullname);

        CoreCLR::Free((void*)name);
        CoreCLR::Free((void*)fullname);
    }
        
    return (MonoImage*)corlibimage;
}

#define CACHE_CLASS_BY_NAME(name) \
nullptr; \
if (klass == nullptr) \
    for (CoreCLRClass* k : corlibimage->GetClasses()) \
        if (k->GetFullname() == name) \
        { \
            klass = k; \
            break; \
        }

MONO_API MonoClass* mono_get_object_class(void)
{
    static CoreCLRClass* klass = CACHE_CLASS_BY_NAME("System.Object");
    return (MonoClass*)klass;
}

MONO_API MonoClass* mono_get_byte_class(void)
{
    static CoreCLRClass* klass = CACHE_CLASS_BY_NAME("System.Byte");
    return (MonoClass*)klass;
}

MONO_API MonoClass* mono_get_void_class(void)
{
    static CoreCLRClass* klass = CACHE_CLASS_BY_NAME("System.Void");
    return (MonoClass*)klass;
}

MONO_API MonoClass* mono_get_boolean_class(void)
{
    static CoreCLRClass* klass = CACHE_CLASS_BY_NAME("System.Boolean");
    return (MonoClass*)klass;
}

MONO_API MonoClass* mono_get_sbyte_class(void)
{
    static CoreCLRClass* klass = CACHE_CLASS_BY_NAME("System.SByte");
    return (MonoClass*)klass;
}

MONO_API MonoClass* mono_get_int16_class(void)
{
    static CoreCLRClass* klass = CACHE_CLASS_BY_NAME("System.Int16");
    return (MonoClass*)klass;
}

MONO_API MonoClass* mono_get_uint16_class(void)
{
    static CoreCLRClass* klass = CACHE_CLASS_BY_NAME("System.UInt16");
    return (MonoClass*)klass;
}

MONO_API MonoClass* mono_get_int32_class(void)
{
    static CoreCLRClass* klass = CACHE_CLASS_BY_NAME("System.Int32");
    return (MonoClass*)klass;
}

MONO_API MonoClass* mono_get_uint32_class(void)
{
    static CoreCLRClass* klass = CACHE_CLASS_BY_NAME("System.UInt32");
    return (MonoClass*)klass;
}

MONO_API MonoClass* mono_get_intptr_class(void)
{
    static CoreCLRClass* klass = CACHE_CLASS_BY_NAME("System.IntPtr");
    return (MonoClass*)klass;
}

MONO_API MonoClass* mono_get_uintptr_class(void)
{
    static CoreCLRClass* klass = CACHE_CLASS_BY_NAME("System.UIntPtr");
    return (MonoClass*)klass;
}

MONO_API MonoClass* mono_get_int64_class(void)
{
    static CoreCLRClass* klass = CACHE_CLASS_BY_NAME("System.Int64");
    return (MonoClass*)klass;
}

MONO_API MonoClass* mono_get_uint64_class(void)
{
    static CoreCLRClass* klass = CACHE_CLASS_BY_NAME("System.UInt64");
    return (MonoClass*)klass;
}

MONO_API MonoClass* mono_get_single_class(void)
{
    static CoreCLRClass* klass = CACHE_CLASS_BY_NAME("System.Single");
    return (MonoClass*)klass;
}

MONO_API MonoClass* mono_get_double_class(void)
{
    static CoreCLRClass* klass = CACHE_CLASS_BY_NAME("System.Double");
    return (MonoClass*)klass;
}

MONO_API MonoClass* mono_get_char_class(void)
{
    static CoreCLRClass* klass = CACHE_CLASS_BY_NAME("System.Char");
    return (MonoClass*)klass;
}

MONO_API MonoClass* mono_get_string_class(void)
{
    static CoreCLRClass* klass = CACHE_CLASS_BY_NAME("System.String");
    return (MonoClass*)klass;
}

/*
 * assembly.h
*/

MONO_API MONO_RT_EXTERNAL_ONLY MonoAssembly* mono_assembly_load_from_full(MonoImage* image, const char* fname, MonoImageOpenStatus* status, mono_bool refonly)
{
    auto assembly = (MonoAssembly*)((CoreCLRAssembly*)image);
    *status = MONO_IMAGE_OK;
    return assembly;
}

MONO_API void mono_assembly_close(MonoAssembly* assembly)
{
    static void* CloseAssemblyPtr = CoreCLR::GetStaticMethodPointer(TEXT("CloseAssembly"));
    CoreCLR::CallStaticMethod<void, const void*>(CloseAssemblyPtr, ((CoreCLRAssembly*)assembly)->GetHandle());

    Delete((CoreCLRAssembly*)assembly);
}

MONO_API MONO_RT_EXTERNAL_ONLY MonoImage* mono_assembly_get_image(MonoAssembly* assembly)
{
    return (MonoImage*)((CoreCLRAssembly*)assembly);
}

/*
 * threads.h
*/

static MonoThread* notImplMonoThreadValue = New<MonoThread>();

MONO_API MonoThread* mono_thread_current(void)
{
    // Ignored
    return notImplMonoThreadValue;
}

MONO_API MonoThread* mono_thread_attach(MonoDomain* domain)
{
    // Ignored
    return notImplMonoThreadValue;
}

MONO_API void mono_thread_exit(void)
{
    // Ignored
}

/*
 * reflection.h
*/

MONO_API MONO_RT_EXTERNAL_ONLY MonoReflectionAssembly* mono_assembly_get_object(MonoDomain* domain, MonoAssembly* assembly)
{
    static void* GetAssemblyObjectPtr = CoreCLR::GetStaticMethodPointer(TEXT("GetAssemblyObject"));
    return (MonoReflectionAssembly*)CoreCLR::CallStaticMethod<void*, const char*>(GetAssemblyObjectPtr, ((CoreCLRAssembly*)assembly)->GetFullname().Get());
}

MONO_API MONO_RT_EXTERNAL_ONLY MonoReflectionType* mono_type_get_object(MonoDomain* domain, MonoType* type)
{
    return (MonoReflectionType*)type;
}

MONO_API MONO_RT_EXTERNAL_ONLY MonoArray* mono_custom_attrs_construct(MonoCustomAttrInfo* cinfo)
{
    ASSERT(false);
}

MONO_API MONO_RT_EXTERNAL_ONLY MonoCustomAttrInfo* mono_custom_attrs_from_method(MonoMethod* method)
{
    ASSERT(false);
}

MONO_API MONO_RT_EXTERNAL_ONLY MonoCustomAttrInfo* mono_custom_attrs_from_class(MonoClass* klass)
{
    MonoCustomAttrInfo* info = (MonoCustomAttrInfo*)New<Array<CoreCLRCustomAttribute*>>(((CoreCLRClass*)klass)->GetCustomAttributes());
    return info;
}

MONO_API MONO_RT_EXTERNAL_ONLY MonoCustomAttrInfo* mono_custom_attrs_from_property(MonoClass* klass, MonoProperty* property)
{
    ASSERT(false);
}
MONO_API MONO_RT_EXTERNAL_ONLY MonoCustomAttrInfo* mono_custom_attrs_from_event(MonoClass* klass, MonoEvent* event)
{
    ASSERT(false);
}
MONO_API MONO_RT_EXTERNAL_ONLY MonoCustomAttrInfo* mono_custom_attrs_from_field(MonoClass* klass, MonoClassField* field)
{
    ASSERT(false);
}

MONO_API mono_bool mono_custom_attrs_has_attr(MonoCustomAttrInfo* ainfo, MonoClass* attr_klass)
{
    Array<CoreCLRCustomAttribute*>* attribs = (Array<CoreCLRCustomAttribute*>*)ainfo;
    for (int i = 0; i < attribs->Count(); i++)
    {
        CoreCLRCustomAttribute* attrib = attribs->At(i);
        if (attrib->GetClass() == (CoreCLRClass*)attr_klass)
            return true;
    }
    return false;
}

MONO_API MONO_RT_EXTERNAL_ONLY MonoObject* mono_custom_attrs_get_attr(MonoCustomAttrInfo* ainfo, MonoClass* attr_klass)
{
    Array<CoreCLRCustomAttribute*>* attribs = (Array<CoreCLRCustomAttribute*>*)ainfo;
    for (int i = 0; i < attribs->Count(); i++)
    {
        CoreCLRCustomAttribute* attrib = attribs->At(i);
        if (attrib->GetClass() == (CoreCLRClass*)attr_klass)
        {
            return (MonoObject*)(attrib)->GetHandle();
        }
    }
    return nullptr;
}

MONO_API void mono_custom_attrs_free(MonoCustomAttrInfo* ainfo)
{
    Array<CoreCLRCustomAttribute*>* attribs = (Array<CoreCLRCustomAttribute*>*)ainfo;
    Delete(attribs);
}

MONO_API MONO_RT_EXTERNAL_ONLY MonoType* mono_reflection_type_get_type(MonoReflectionType* reftype)
{
    return (MonoType*)reftype;
}

/*
 * class.h
*/

MONO_API MONO_RT_EXTERNAL_ONLY MonoClass* mono_class_get(MonoImage* image, uint32 type_token)
{
    int index = type_token - 0x02000000 - 2; //MONO_TOKEN_TYPE_DEF
    auto classes = ((CoreCLRAssembly*)image)->GetClasses();
    return (MonoClass*)classes[index];
}

MONO_API MONO_RT_EXTERNAL_ONLY MonoClass* mono_class_from_name(MonoImage* image, const char* name_space_, const char* name_)
{
    StringAnsi name_space(name_space_);
    StringAnsi name(name_);

    for (auto klass : ((CoreCLRAssembly*)image)->GetClasses())
    {
        if (klass->GetNamespace() == name_space && klass->GetName() == name)
            return (MonoClass*)klass;
    }

    return nullptr;
}

MONO_API MONO_RT_EXTERNAL_ONLY MonoMethod* mono_class_inflate_generic_method(MonoMethod* method, MonoGenericContext* context)
{
    ASSERT(false);
}

MONO_API MONO_RT_EXTERNAL_ONLY MonoClass* mono_array_class_get(MonoClass* element_class, uint32 rank)
{
    ASSERT(false);
}

MONO_API MONO_RT_EXTERNAL_ONLY MonoClassField* mono_class_get_field_from_name(MonoClass* klass, const char* name)
{
    for (auto field : ((CoreCLRClass*)klass)->GetFields())
    {
        if (field->GetName() == name)
            return (MonoClassField*)field;
    }
    return nullptr;
}

MONO_API MonoProperty* mono_class_get_property_from_name(MonoClass* klass, const char* name)
{
    for (auto prop : ((CoreCLRClass*)klass)->GetProperties())
    {
        if (prop->GetName() == name)
            return (MonoProperty*)prop;
    }
    return nullptr;
}

MONO_API int32_t mono_class_instance_size(MonoClass* klass)
{
    ASSERT(false);
}

MONO_API int32_t mono_class_value_size(MonoClass* klass, uint32* align)
{
    static void* NativeSizeOfPtr = CoreCLR::GetStaticMethodPointer(TEXT("NativeSizeOf"));
    return CoreCLR::CallStaticMethod<int, void*, uint32*>(NativeSizeOfPtr, ((CoreCLRClass*)klass)->GetTypeHandle(), align);
}

MONO_API MonoClass* mono_class_from_mono_type(MonoType* type)
{
    CoreCLRClass* klass = GetOrCreateClass((void*)type);
    return (MonoClass*)klass;
}

MONO_API mono_bool mono_class_is_subclass_of(MonoClass* klass, MonoClass* klassc, mono_bool check_interfaces)
{
    static void* TypeIsSubclassOfPtr = CoreCLR::GetStaticMethodPointer(TEXT("TypeIsSubclassOf"));
    return CoreCLR::CallStaticMethod<bool, void*, void*, bool>(TypeIsSubclassOfPtr, ((CoreCLRClass*)klass)->GetTypeHandle(), ((CoreCLRClass*)klassc)->GetTypeHandle(), check_interfaces);
}

MONO_API char* mono_type_get_name(MonoType* type)
{
    CoreCLRClass* klass = (CoreCLRClass*)mono_type_get_class(type);
    return StringAnsi(klass->GetFullname()).Get();
}

MONO_API MonoImage* mono_class_get_image(MonoClass* klass)
{
    return (MonoImage*)((CoreCLRClass*)klass)->GetAssembly();
}

MONO_API MONO_RT_EXTERNAL_ONLY MonoClass* mono_class_get_element_class(MonoClass* klass)
{
    ASSERT(false);
}

MONO_API MONO_RT_EXTERNAL_ONLY mono_bool mono_class_is_valuetype(MonoClass* klass)
{
    static void* TypeIsValueTypePtr = CoreCLR::GetStaticMethodPointer(TEXT("TypeIsValueType"));
    return (mono_bool)CoreCLR::CallStaticMethod<bool, void*>(TypeIsValueTypePtr, ((CoreCLRClass*)klass)->GetTypeHandle());
}

MONO_API MONO_RT_EXTERNAL_ONLY mono_bool mono_class_is_enum(MonoClass* klass)
{
    static void* TypeIsEnumPtr = CoreCLR::GetStaticMethodPointer(TEXT("TypeIsEnum"));
    return (mono_bool)CoreCLR::CallStaticMethod<bool, void*>(TypeIsEnumPtr, ((CoreCLRClass*)klass)->GetTypeHandle());
}

MONO_API MONO_RT_EXTERNAL_ONLY MonoClass* mono_class_get_parent(MonoClass* klass)
{
    static void* GetClassParentPtr = CoreCLR::GetStaticMethodPointer(TEXT("GetClassParent"));
    void* parentHandle = CoreCLR::CallStaticMethod<void*, void*>(GetClassParentPtr, ((CoreCLRClass*)klass)->GetTypeHandle());
    return (MonoClass*)classHandles[parentHandle];
}

MONO_API MonoClass* mono_class_get_nesting_type(MonoClass* klass)
{
    // Ignored
    return nullptr;
}

MONO_API uint32 mono_class_get_flags(MonoClass* klass)
{
    return ((CoreCLRClass*)klass)->GetAttributes();
}

MONO_API MONO_RT_EXTERNAL_ONLY const char* mono_class_get_name(MonoClass* klass)
{
    return ((CoreCLRClass*)klass)->GetName().Get();
}

MONO_API MONO_RT_EXTERNAL_ONLY const char* mono_class_get_namespace(MonoClass* klass)
{
    return ((CoreCLRClass*)klass)->GetNamespace().Get();
}

MONO_API MonoType* mono_class_get_type(MonoClass* klass)
{
    return (MonoType*)((CoreCLRClass*)klass)->GetTypeHandle();
}

MONO_API uint32 mono_class_get_type_token(MonoClass* klass)
{
    return ((CoreCLRClass*)klass)->GetTypeToken();
}

MONO_API MONO_RT_EXTERNAL_ONLY MonoClassField* mono_class_get_fields(MonoClass* klass, void** iter)
{
    auto fields = ((CoreCLRClass*)klass)->GetFields();
    uintptr_t index = (uintptr_t)(*iter);
    if (index >= 0 && index < fields.Count())
    {
        *iter = (void*)(index + 1);
        return (MonoClassField*)fields[(int)index];
    }
    *iter = nullptr;
    return nullptr;
}

MONO_API MonoMethod* mono_class_get_methods(MonoClass* klass, void** iter)
{
    auto methods = ((CoreCLRClass*)klass)->GetMethods();
    uintptr_t index = (uintptr_t)(*iter);
    if (index >= 0 && index < methods.Count())
    {
        *iter = (void*)(index + 1);
        return (MonoMethod*)methods[(int)index];
    }
    *iter = nullptr;
    return nullptr;
}

MONO_API MonoProperty* mono_class_get_properties(MonoClass* klass, void** iter)
{
    auto properties = ((CoreCLRClass*)klass)->GetProperties();
    uintptr_t index = (uintptr_t)(*iter);
    if (index >= 0 && index < properties.Count())
    {
        *iter = (void*)(index + 1);
        return (MonoProperty*)properties[(int)index];
    }
    *iter = nullptr;
    return nullptr;
}

MONO_API MonoEvent* mono_class_get_events(MonoClass* klass, void** iter)
{
    ASSERT(false);
}

MONO_API MonoClass* mono_class_get_interfaces(MonoClass* klass, void** iter)
{
    auto interfaces = ((CoreCLRClass*)klass)->GetInterfaces();
    uintptr_t index = (uintptr_t)(*iter);
    if (index >= 0 && index < interfaces.Count())
    {
        *iter = (void*)(index + 1);
        return (MonoClass*)interfaces[(int)index];
    }
    *iter = nullptr;
    return nullptr;
}

MONO_API const char* mono_field_get_name(MonoClassField* field)
{
    return ((CoreCLRField*)field)->GetName().Get();
}

MONO_API MonoType* mono_field_get_type(MonoClassField* field)
{
    return (MonoType*)((CoreCLRField*)field)->GetClass()->GetTypeHandle();
}

MONO_API MonoClass* mono_field_get_parent(MonoClassField* field)
{
    ASSERT(false);
}

MONO_API uint32 mono_field_get_flags(MonoClassField* field)
{
    return ((CoreCLRField*)field)->GetAttributes();
}

MONO_API uint32 mono_field_get_offset(MonoClassField* field)
{
    ASSERT(false);
}

MONO_API const char* mono_property_get_name(MonoProperty* prop)
{
    return ((CoreCLRProperty*)prop)->GetName().Get();
}

MONO_API MonoMethod* mono_property_get_set_method(MonoProperty* prop)
{
    return (MonoMethod*)((CoreCLRProperty*)prop)->GetSetMethod();
}

MONO_API MonoMethod* mono_property_get_get_method(MonoProperty* prop)
{
    return (MonoMethod*)((CoreCLRProperty*)prop)->GetGetMethod();
}

MONO_API MonoClass* mono_property_get_parent(MonoProperty* prop)
{
    return (MonoClass*)((CoreCLRProperty*)prop)->GetClass();
}

MONO_API const char* mono_event_get_name(MonoEvent* event)
{
    ASSERT(false);
}

MONO_API MonoMethod* mono_event_get_add_method(MonoEvent* event)
{
    ASSERT(false);
}

MONO_API MonoMethod* mono_event_get_remove_method(MonoEvent* event)
{
    ASSERT(false);
}

MONO_API MonoClass* mono_event_get_parent(MonoEvent* event)
{
    ASSERT(false);
}

MONO_API MONO_RT_EXTERNAL_ONLY MonoMethod* mono_class_get_method_from_name(MonoClass* klass, const char* name, int param_count)
{
    for (CoreCLRMethod* method : ((CoreCLRClass*)klass)->GetMethods())
    {
        if (method->GetName() == name && method->GetNumParameters() == param_count)
            return (MonoMethod*)method;
    }

    return nullptr;
}

/*
 * mono-publib.h
*/

MONO_API void mono_free(void* ptr)
{
    if (ptr != nullptr)
        CoreCLR::Free(ptr);
}

/*
 * metadata.h
*/

MONO_API mono_bool mono_type_is_byref(MonoType* type)
{
    ASSERT(false);
}

MONO_API int mono_type_get_type(MonoType* type)
{
    CoreCLRClass* klass = GetOrCreateClass((void*)type);
    return klass->GetMonoType();
}

MONO_API MonoClass* mono_type_get_class(MonoType* type)
{
    return (MonoClass*)classHandles[(void*)type];
}

MONO_API mono_bool mono_type_is_struct(MonoType* type)
{
    ASSERT(false);
}

MONO_API mono_bool mono_type_is_void(MonoType* type)
{
    ASSERT(false);
}

MONO_API mono_bool mono_type_is_pointer(MonoType* type)
{
    ASSERT(false);
}

MONO_API mono_bool mono_type_is_reference(MonoType* type)
{
    ASSERT(false);
}

MONO_API MonoType* mono_signature_get_return_type(MonoMethodSignature* sig)
{
    return (MonoType*)((CoreCLRMethod*)sig)->GetReturnType();
}

MONO_API MonoType* mono_signature_get_params(MonoMethodSignature* sig, void** iter)
{
    auto parameterTypes = ((CoreCLRMethod*)sig)->GetParameterTypes();
    uintptr_t index = (uintptr_t)(*iter);
    if (index >= 0 && index < parameterTypes.Count())
    {
        *iter = (void*)(index+1);
        return (MonoType*)parameterTypes[(int)index];
    }
    *iter = nullptr;
    return nullptr;
}

MONO_API uint32 mono_signature_get_param_count(MonoMethodSignature* sig)
{
    return ((CoreCLRMethod*)sig)->GetNumParameters();
}

MONO_API mono_bool mono_signature_param_is_out(MonoMethodSignature* sig, int param_num)
{
    static void* GetMethodParameterIsOutPtr = CoreCLR::GetStaticMethodPointer(TEXT("GetMethodParameterIsOut"));
    return CoreCLR::CallStaticMethod<bool, void*, int>(GetMethodParameterIsOutPtr, ((CoreCLRMethod*)sig)->GetMethodHandle(), param_num);
}

MONO_API int mono_type_stack_size(MonoType* type, int* alignment)
{
    ASSERT(false);
}

/*
 * exception.h
*/

MONO_API MonoException* mono_exception_from_name_msg(MonoImage* image, const char* name_space, const char* name, const char* msg)
{
    ASSERT(false);
}

MONO_API MonoException* mono_get_exception_null_reference(void)
{
    static void* GetNullReferenceExceptionPtr = CoreCLR::GetStaticMethodPointer(TEXT("GetNullReferenceException"));
    return (MonoException*)CoreCLR::CallStaticMethod<void*>(GetNullReferenceExceptionPtr);
}

MONO_API MonoException* mono_get_exception_not_supported(const char* msg)
{
    static void* GetNotSupportedExceptionPtr = CoreCLR::GetStaticMethodPointer(TEXT("GetNotSupportedException"));
    return (MonoException*)CoreCLR::CallStaticMethod<void*>(GetNotSupportedExceptionPtr);
}

MONO_API MONO_RT_EXTERNAL_ONLY MonoException* mono_get_exception_argument_null(const char* arg)
{
    static void* GetArgumentNullExceptionPtr = CoreCLR::GetStaticMethodPointer(TEXT("GetArgumentNullException"));
    return (MonoException*)CoreCLR::CallStaticMethod<void*>(GetArgumentNullExceptionPtr);
}

MONO_API MonoException* mono_get_exception_argument(const char* arg, const char* msg)
{
    static void* GetArgumentExceptionPtr = CoreCLR::GetStaticMethodPointer(TEXT("GetArgumentException"));
    return (MonoException*)CoreCLR::CallStaticMethod<void*>(GetArgumentExceptionPtr);
}

MONO_API MONO_RT_EXTERNAL_ONLY MonoException* mono_get_exception_argument_out_of_range(const char* arg)
{
    static void* GetArgumentOutOfRangeExceptionPtr = CoreCLR::GetStaticMethodPointer(TEXT("GetArgumentOutOfRangeException"));
    return (MonoException*)CoreCLR::CallStaticMethod<void*>(GetArgumentOutOfRangeExceptionPtr);
}

/*
 * image.h
*/

MONO_API MONO_RT_EXTERNAL_ONLY MonoImage* mono_image_open_from_data_with_name(char* data, uint32 data_len, mono_bool need_copy, MonoImageOpenStatus* status, mono_bool refonly, const char* path)
{
    const char* name;
    const char* fullname;
    static void* LoadAssemblyImagePtr = CoreCLR::GetStaticMethodPointer(TEXT("LoadAssemblyImage"));
    void* assemblyHandle = CoreCLR::CallStaticMethod<void*, char*, int, const char*, const char**, const char**>(LoadAssemblyImagePtr, data, data_len, path, &name, &fullname);
    if (!assemblyHandle)
    {
        *status = MONO_IMAGE_IMAGE_INVALID;
        return nullptr;
    }
    CoreCLRAssembly* assembly = New<CoreCLRAssembly>(assemblyHandle, name, fullname);
    
    CoreCLR::Free((void*)name);
    CoreCLR::Free((void*)fullname);

    *status = MONO_IMAGE_OK;
    return (MonoImage*)assembly;
}

MONO_API void mono_image_close(MonoImage* image)
{
    // Ignored
}
MONO_API const char* mono_image_get_name(MonoImage* image)
{
    return ((CoreCLRAssembly*)image)->GetName().Get();
}
MONO_API MonoAssembly* mono_image_get_assembly(MonoImage* image)
{
    return (MonoAssembly*)image;
}
MONO_API int mono_image_get_table_rows(MonoImage* image, int table_id)
{
    return ((CoreCLRAssembly*)image)->GetClasses().Count() + 1;
}

/*
 * mono-gc.h
*/

MONO_API void mono_gc_collect(int generation)
{
    // Ignored
}

MONO_API int mono_gc_max_generation(void)
{
    // Ignored
    return 0;
}

MONO_API MonoBoolean mono_gc_pending_finalizers(void)
{
    // Ignored
    return false;
}

MONO_API void mono_gc_finalize_notify(void)
{
    // Ignored
}

#pragma warning(default : 4297)

#endif
