#include <ThirdParty/mono-2.0/mono/metadata/object.h>
#include <ThirdParty/mono-2.0/mono/metadata/appdomain.h>

#include "CoreCLR.h"
#include "Engine/Scripting/Types.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Graphics/RenderView.h"
#include "Engine/Core/Types/StringBuilder.h"

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
struct ManagedClass
{
    void* typeHandle;
    const char* name;
    const char* fullname;
    const char* namespace_;
    uint32 typeAttributes;
};

struct ClassMethod
{
    const char* name;
    int numParameters;
    void* handle;
    uint32 methodAttributes;
};

struct ClassField
{
    const char* name;
    void* fieldHandle;
    void* fieldType;
    uint32 fieldAttributes;
};

struct ClassProperty
{
    const char* name;
    void* getterHandle;
    void* setterHandle;
    uint32 getterFlags;
    uint32 setterFlags;
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
        _assemblyHandle = assemblyHandle;
        _name = name;
        _fullname = fullname;

        ManagedClass* managedClasses;
        int classCount;

        CoreCLR::CallStaticMethodInternal<void, void*, ManagedClass**, int*>(TEXT("GetManagedClasses"), _assemblyHandle, &managedClasses, &classCount);
        for (int i = 0; i < classCount; i++)
        {
            CoreCLRClass* mci = New<CoreCLRClass>(managedClasses[i].typeHandle, StringAnsi(managedClasses[i].name), StringAnsi(managedClasses[i].fullname), StringAnsi(managedClasses[i].namespace_), managedClasses[i].typeAttributes, this);
            _classes.Add(mci);

            ASSERT(managedClasses[i].typeHandle != nullptr);
            classHandles.Add(managedClasses[i].typeHandle, mci);

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

    void* GetHandle()
    {
        return _assemblyHandle;
    }

    const StringAnsi& GetName()
    {
        return _name;
    }

    const StringAnsi& GetFullname()
    {
        return _fullname;
    }

    Array<CoreCLRClass*> GetClasses()
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

public:
    CoreCLRClass(void* typeHandle, StringAnsi name, StringAnsi fullname, StringAnsi namespace_, uint32 typeAttributes, CoreCLRAssembly* image)
        : _typeHandle(typeHandle), _name(name), _fullname(fullname), _namespace(namespace_), _typeAttributes(typeAttributes), _image(image)
    {
        _typeToken = TypeTokenPool++;
    }

    ~CoreCLRClass()
    {
        for (auto method : _methods)
        {
            //int rem = monoMethods.RemoveValue(method);
            //ASSERT(rem > 0)
        }
        _methods.ClearDelete();
        _fields.ClearDelete();
        _attributes.ClearDelete();
        _properties.ClearDelete();
        _interfaces.Clear();

        classHandles.Remove(_typeHandle);
    }

    uint32 GetAttributes()
    {
        return _typeAttributes;
    }

    uint32 GetTypeToken()
    {
        return _typeToken;
    }

    int GetSize()
    {
        if (_size != 0)
            return _size;

        uint32 dummy;
        _size = mono_class_value_size((MonoClass*)this, &dummy);

        return _size;
    }

    const StringAnsi& GetName()
    {
        return _name;
    }

    const StringAnsi& GetFullname()
    {
        return _fullname; // FIXME: this should probably return the decorated C# name for generic types (foo<T>) and not the IL-name (foo`1[[T)
    }

    const StringAnsi& GetNamespace()
    {
        return _namespace;
    }

    void* GetTypeHandle()
    {
        return _typeHandle;
    }

    CoreCLRAssembly* GetAssembly()
    {
        return _image;
    }

    Array<CoreCLRMethod*> GetMethods()
    {
        if (_cachedMethods)
            return _methods;

        ClassMethod* foundMethods;
        int numMethods;

        CoreCLR::CallStaticMethodInternal<void, void*, ClassMethod**, int*>(TEXT("GetClassMethods"), _typeHandle, &foundMethods, &numMethods);
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

    Array<CoreCLRField*> GetFields()
    {
        if (_cachedFields)
            return _fields;

        ClassField* foundFields;
        int numFields;

        CoreCLR::CallStaticMethodInternal<void, void*, ClassField**, int*>(TEXT("GetClassFields"), _typeHandle, &foundFields, &numFields);
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

    Array<CoreCLRProperty*> GetProperties()
    {
        if (_cachedProperties)
            return _properties;

        ClassProperty* foundProperties;
        int numProperties;

        CoreCLR::CallStaticMethodInternal<void, void*, ClassProperty**, int*>(TEXT("GetClassProperties"), _typeHandle, &foundProperties, &numProperties);
        for (int i = 0; i < numProperties; i++)
        {
            CoreCLRProperty* prop = New<CoreCLRProperty>(StringAnsi(foundProperties[i].name), foundProperties[i].getterHandle, foundProperties[i].setterHandle, foundProperties[i].getterFlags, foundProperties[i].setterFlags, this);
            _properties.Add(prop);

            CoreCLR::Free((void*)foundProperties[i].name);
        }
        CoreCLR::Free(foundProperties);

        _cachedProperties = true;
        return _properties;
    }

    Array<CoreCLRCustomAttribute*> GetCustomAttributes()
    {
        if (_cachedAttributes)
            return _attributes;

        ClassAttribute* foundAttributes;
        int numAttributes;

        CoreCLR::CallStaticMethodInternal<void, void*, ClassAttribute**, int*>(TEXT("GetClassAttributes"), _typeHandle, &foundAttributes, &numAttributes);
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

    Array<CoreCLRClass*> GetInterfaces()
    {
        if (_cachedInterfaces)
            return _interfaces;

        void** foundInterfaces;
        int numInterfaces;

        CoreCLR::CallStaticMethodInternal<void, void*, void***, int*>(TEXT("GetClassInterfaces"), _typeHandle, &foundInterfaces, &numInterfaces);
        for (int i = 0; i < numInterfaces; i++)
        {
            CoreCLRClass* interfaceClass = classHandles[foundInterfaces[i]];
            _interfaces.Add(interfaceClass);
        }
        CoreCLR::Free(foundInterfaces);

        _cachedInterfaces = true;
        return _interfaces;
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
    CoreCLRMethod(StringAnsi name, int numParams, void* methodHandle, uint32 flags, CoreCLRClass* klass)
        :_name(name), _numParams(numParams), _methodHandle(methodHandle), _methodAttributes(flags), _class(klass)
    {
    }

    const StringAnsi& GetName()
    {
        return _name;
    }

    CoreCLRClass* GetClass()
    {
        return _class;
    }

    uint32 GetAttributes()
    {
        return _methodAttributes;
    }

    int GetNumParameters()
    {
        return _numParams;
    }

    void* GetMethodHandle()
    {
        return _methodHandle;
    }

    Array<void*> GetParameterTypes()
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

    void CacheParameters()
    {
        _returnType = CoreCLR::CallStaticMethodInternal<void*, void*>(TEXT("GetMethodReturnType"), _methodHandle);

        void** parameterTypeHandles;
        CoreCLR::CallStaticMethodInternal<void, void*, void***>(TEXT("GetMethodParameterTypes"), _methodHandle, &parameterTypeHandles);

        _parameterTypes.SetCapacity(_numParams, false);

        for (int i = 0; i < _numParams; i++)
        {
            _parameterTypes.Add(parameterTypeHandles[i]);
        }
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

    const StringAnsi& GetName()
    {
        return _name;
    }

    void* GetType()
    {
        return _fieldType;
    }

    CoreCLRClass* GetClass()
    {
        return _class;
    }

    uint32 GetAttributes()
    {
        return _fieldAttributes;
    }

    void* GetHandle()
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
    CoreCLRProperty(StringAnsi name, void* getter, void* setter, uint32 getterFlags, uint32 setterFlags, CoreCLRClass* klass)
        :_name(name), _class(klass)
    {
        if (getter != nullptr)
            _getMethod = New<CoreCLRMethod>(StringAnsi(_name + "Get"), 1, getter, getterFlags, klass);
        if (setter != nullptr)
            _setMethod = New<CoreCLRMethod>(StringAnsi(_name + "Set"), 1, setter, setterFlags, klass);
    }

    const StringAnsi& GetName()
    {
        return _name;
    }

    CoreCLRClass* GetClass()
    {
        return _class;
    }

    CoreCLRMethod* GetGetMethod()
    {
        return _getMethod;
    }

    CoreCLRMethod* GetSetMethod()
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
    CoreCLRCustomAttribute(StringAnsi name, void* handle, CoreCLRClass* owningClass, CoreCLRClass* attributeClass)
        :_name(name), _handle(handle), _owningClass(owningClass), _attributeClass(attributeClass)
    {
    }

    void* GetHandle()
    {
        return _handle;
    }

    CoreCLRClass* GetClass()
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
    CoreCLRClass* klass;
    if (classHandles.TryGet(type, klass))
        return klass;
    return nullptr;
}

CoreCLRClass* GetOrCreateClass(void* type)
{
    CoreCLRClass* klass;
    if (!classHandles.TryGet(type, klass))
    {
        ManagedClass classInfo;
        void* assemblyHandle;
        CoreCLR::CallStaticMethodInternal<void, void*, void*>(TEXT("GetManagedClassFromType"), type, &classInfo, &assemblyHandle);
        CoreCLRAssembly* image = GetAssembly(assemblyHandle);
        klass = New<CoreCLRClass>(classInfo.typeHandle, StringAnsi(classInfo.name), StringAnsi(classInfo.fullname), StringAnsi(classInfo.namespace_), classInfo.typeAttributes, image);
        if (image != nullptr)
            image->AddClass(klass);

        if (type != classInfo.typeHandle)
            CoreCLR::CallStaticMethodInternal<void, void*, void*>(TEXT("GetManagedClassFromType"), type, &classInfo);
        classHandles.Add(classInfo.typeHandle, klass);

        CoreCLR::Free((void*)classInfo.name);
        CoreCLR::Free((void*)classInfo.fullname);
        CoreCLR::Free((void*)classInfo.namespace_);
    }
    ASSERT(klass != nullptr);
    return klass;
}

gchandle CoreCLR::NewGCHandle(void* obj, bool pinned)
{
    return (gchandle)CoreCLR::CallStaticMethodInternal<void*, void*, bool>(TEXT("NewGCHandle"), obj, pinned);
}

gchandle CoreCLR::NewGCHandleWeakref(void* obj, bool track_resurrection)
{
    return (gchandle)CoreCLR::CallStaticMethodInternal<void*, void*, bool>(TEXT("NewGCHandleWeakref"), obj, track_resurrection);
}

void* CoreCLR::GetGCHandleTarget(const gchandle& gchandle)
{
    return (void*)gchandle;
}

void CoreCLR::FreeGCHandle(const gchandle& gchandle)
{
    CoreCLR::CallStaticMethodInternal<void, void*>(TEXT("FreeGCHandle"), (void*)gchandle);
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
    return CoreCLR::CallStaticMethodInternal<void*, void*, void*>(TEXT("GetCustomAttribute"), ((CoreCLRClass*)klass)->GetTypeHandle(), ((CoreCLRClass*)attribClass)->GetTypeHandle());
}
Array<void*> CoreCLR::GetCustomAttributes(void* klass)
{
    Array<CoreCLRCustomAttribute*> attrib = ((CoreCLRClass*)klass)->GetCustomAttributes();

    Array<void*> attributes;
    attributes.Resize(attrib.Count(), false);
    for (int i = 0; i < attrib.Count(); i++)
        attributes.Add(attrib[i]->GetHandle());

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
    // Ignored, prevents the linker from removing unused functions
}

/*
 * objects.h
*/

MONO_API mono_unichar2* mono_string_chars(MonoString* s)
{
    _MonoString* str = (_MonoString*)CoreCLR::CallStaticMethodInternal<void*, void*>(TEXT("GetStringPointer"), s);
    return str->chars;
}

MONO_API int mono_string_length(MonoString* s)
{
    _MonoString* str = (_MonoString*)CoreCLR::CallStaticMethodInternal<void*, void*>(TEXT("GetStringPointer"), s);
    return str->length;
}

MONO_API MONO_RT_EXTERNAL_ONLY MonoObject* mono_object_new(MonoDomain* domain, MonoClass* klass)
{
    return (MonoObject*)CoreCLR::CallStaticMethodInternal<void*, void*>(TEXT("NewObject"), ((CoreCLRClass*)klass)->GetTypeHandle());
}

MONO_API MONO_RT_EXTERNAL_ONLY MonoArray* mono_array_new(MonoDomain* domain, MonoClass* eclass, uintptr_t n)
{
    return (MonoArray*)CoreCLR::CallStaticMethodInternal<void*, void*, long long>(TEXT("NewArray"), ((CoreCLRClass*)eclass)->GetTypeHandle(), n);
}

MONO_API char* mono_array_addr_with_size(MonoArray* array, int size, uintptr_t idx)
{
    return (char*)CoreCLR::CallStaticMethodInternal<void*, void*, int, int>(TEXT("GetArrayPointerToElement"), array, size, (int)idx);
}

MONO_API uintptr_t mono_array_length(MonoArray* array)
{
    return CoreCLR::CallStaticMethodInternal<int, void*>(TEXT("GetArrayLength"), array);
}

MONO_API MonoString* mono_string_empty(MonoDomain* domain)
{
    return (MonoString*)CoreCLR::CallStaticMethodInternal<void*>(TEXT("GetStringEmpty"));
}

MONO_API MONO_RT_EXTERNAL_ONLY MonoString* mono_string_new_utf16(MonoDomain* domain, const mono_unichar2* text, int32_t len)
{
    return (MonoString*)CoreCLR::CallStaticMethodInternal<void*, const mono_unichar2*, int>(TEXT("NewStringUTF16"), text, len);
}

MONO_API MONO_RT_EXTERNAL_ONLY MonoString* mono_string_new(MonoDomain* domain, const char* text)
{
    return (MonoString*)CoreCLR::CallStaticMethodInternal<void*, const char*>(TEXT("NewString"), text);
}

MONO_API MONO_RT_EXTERNAL_ONLY MonoString* mono_string_new_len(MonoDomain* domain, const char* text, unsigned int length)
{
    return (MonoString*)CoreCLR::CallStaticMethodInternal<void*, const char*, int>(TEXT("NewStringLength"), text, length);
}

MONO_API MONO_RT_EXTERNAL_ONLY char* mono_string_to_utf8(MonoString* string_obj)
{
    Char* strw = string_obj != nullptr ? (Char*)mono_string_chars(string_obj) : nullptr;
    auto len = string_obj != nullptr ? mono_string_length(string_obj) : 0;
    ASSERT(len >= 0)
    char* stra = (char*)CoreCLR::Allocate(sizeof(char) * (len + 1));
    StringUtils::ConvertUTF162UTF8(strw, stra, len, len);
    stra[len] = 0;

    return stra;
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
    return (MonoObject*)CoreCLR::CallStaticMethodInternal<void*, void*, void*>(TEXT("BoxValue"), ((CoreCLRClass*)klass)->GetTypeHandle(), val);
}

MONO_API void mono_value_copy(void* dest, /*const*/ void* src, MonoClass* klass)
{
    CoreCLRClass* mci = (CoreCLRClass*)klass;
    Platform::MemoryCopy(dest, src, mci->GetSize());
}

MONO_API MONO_RT_EXTERNAL_ONLY MonoClass* mono_object_get_class(MonoObject* obj)
{
    void* classHandle = CoreCLR::CallStaticMethodInternal<void*, void*>(TEXT("GetObjectType"), obj);

    CoreCLRClass* mi = GetOrCreateClass((void*)classHandle);

    ASSERT(mi != nullptr)
    return (MonoClass*)mi;
}

MONO_API void* mono_object_unbox(MonoObject* obj)
{
    return CoreCLR::CallStaticMethodInternal<void*, void*>(TEXT("UnboxValue"), obj);
}

MONO_API MONO_RT_EXTERNAL_ONLY void mono_raise_exception(MonoException* ex)
{
    CoreCLR::CallStaticMethodInternal<void*, void*>(TEXT("RaiseException"), ex);
}

MONO_API MONO_RT_EXTERNAL_ONLY void mono_runtime_object_init(MonoObject* this_obj)
{
    CoreCLR::CallStaticMethodInternal<void, void*>(TEXT("ObjectInit"), this_obj);
}

MONO_API MonoMethod* mono_object_get_virtual_method(MonoObject* obj, MonoMethod* method)
{
    return method;
}

MONO_API MONO_RT_EXTERNAL_ONLY MonoObject* mono_runtime_invoke(MonoMethod* method, void* obj, void** params, MonoObject** exc)
{
    CoreCLRMethod* mi = (CoreCLRMethod*)method;
    void* methodPtr = mi->GetMethodHandle();
    ASSERT(methodPtr != nullptr)

    static void* InvokeMethodPtr = CoreCLR::GetStaticMethodPointer(TEXT("InvokeMethod"));
    return (MonoObject*)CoreCLR::CallStaticMethodInternalPointer<void*, void*, void*, void*, void*>(InvokeMethodPtr, obj, methodPtr, params, exc);
}

MONO_API MONO_RT_EXTERNAL_ONLY void* mono_method_get_unmanaged_thunk(MonoMethod* method)
{
    CoreCLRMethod* mi = (CoreCLRMethod*)method;
    void* methodPtr = mi->GetMethodHandle();
    ASSERT(methodPtr != nullptr)

    return CoreCLR::CallStaticMethodInternal<void*, void*>(TEXT("GetMethodUnmanagedFunctionPointer"), methodPtr);
}

MONO_API void mono_field_set_value(MonoObject* obj, MonoClassField* field, void* value)
{
    CoreCLR::CallStaticMethodInternal<void, void*, void*, void*>(TEXT("FieldSetValue"), obj, ((CoreCLRField*)field)->GetHandle(), value);
}

MONO_API void mono_field_get_value(MonoObject* obj, MonoClassField* field, void* value)
{
    CoreCLR::CallStaticMethodInternal<void, void*, void*, void*>(TEXT("FieldGetValue"), obj, ((CoreCLRField*)field)->GetHandle(), value);
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
    CoreCLR::CallStaticMethodInternal<void, void*, void*, void*>(TEXT("SetArrayValueReference"), arr, slot_ptr, value);
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
    void* assemblyHandle = CoreCLR::CallStaticMethodInternal<void*, const char*, const char**, const char**>(TEXT("LoadAssemblyFromPath"), path, &name, &fullname);
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
        void* assemblyHandle = CoreCLR::CallStaticMethodInternal<void*, const char*, const char**, const char**>(TEXT("GetAssemblyByName"), "System.Private.CoreLib", &name, &fullname);
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
 * jit.h
*/

MONO_API char* mono_get_runtime_build_info(void)
{
    return CoreCLR::CallStaticMethodInternal<char*>(TEXT("GetRuntimeInformation"));
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
    CoreCLR::CallStaticMethodInternal<void, void*>(TEXT("CloseAssembly"), ((CoreCLRAssembly*)assembly)->GetHandle());

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
 * mono-debug.h
*/

MONO_API void mono_debug_open_image_from_memory(MonoImage* image, const mono_byte* raw_contents, int size)
{
    // Ignored
}

/*
 * reflection.h
*/

MONO_API MONO_RT_EXTERNAL_ONLY MonoReflectionAssembly* mono_assembly_get_object(MonoDomain* domain, MonoAssembly* assembly)
{
    return (MonoReflectionAssembly*)CoreCLR::CallStaticMethodInternal<void*, const char*>(TEXT("GetAssemblyObject"), ((CoreCLRAssembly*)assembly)->GetFullname().Get());
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
    CoreCLRClass* mi = (CoreCLRClass*)klass;
    MonoCustomAttrInfo* info = (MonoCustomAttrInfo*)New<Array<CoreCLRCustomAttribute*>>(mi->GetCustomAttributes());
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
    Array<CoreCLRCustomAttribute*>* mcai = (Array<CoreCLRCustomAttribute*>*)ainfo;
    Delete(mcai);
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

    CoreCLRAssembly* mi = (CoreCLRAssembly*)image;
    for (auto klass : mi->GetClasses())
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
    StringAnsi name2(name);
    CoreCLRClass* mi = (CoreCLRClass*)klass;
    for (auto field : mi->GetFields())
    {
        if (field->GetName() == name2)
            return (MonoClassField*)field;
    }
    return nullptr;
}

MONO_API MonoProperty* mono_class_get_property_from_name(MonoClass* klass, const char* name)
{
    StringAnsi name2(name);
    CoreCLRClass* mi = (CoreCLRClass*)klass;
    for (auto prop : mi->GetProperties())
    {
        if (prop->GetName() == name2)
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
    return CoreCLR::CallStaticMethodInternal<int, void*, uint32*>(TEXT("NativeSizeOf"), ((CoreCLRClass*)klass)->GetTypeHandle(), align);
}

MONO_API MonoClass* mono_class_from_mono_type(MonoType* type)
{
    CoreCLRClass* klass = GetOrCreateClass((void*)type);
    return (MonoClass*)klass;
}

MONO_API mono_bool mono_class_is_subclass_of(MonoClass* klass, MonoClass* klassc, mono_bool check_interfaces)
{
    return CoreCLR::CallStaticMethodInternal<bool, void*, void*, bool>(TEXT("TypeIsSubclassOf"), ((CoreCLRClass*)klass)->GetTypeHandle(), ((CoreCLRClass*)klassc)->GetTypeHandle(), check_interfaces);
}

MONO_API char* mono_type_get_name(MonoType* type)
{
    CoreCLRClass* mci = (CoreCLRClass*)mono_type_get_class(type);
    return StringAnsi(mci->GetFullname()).Get();
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
    return (mono_bool)CoreCLR::CallStaticMethodInternal<bool, void*>(TEXT("TypeIsValueType"), ((CoreCLRClass*)klass)->GetTypeHandle());
}

MONO_API MONO_RT_EXTERNAL_ONLY mono_bool mono_class_is_enum(MonoClass* klass)
{
    return (mono_bool)CoreCLR::CallStaticMethodInternal<bool, void*>(TEXT("TypeIsEnum"), ((CoreCLRClass*)klass)->GetTypeHandle());
}

MONO_API MONO_RT_EXTERNAL_ONLY MonoClass* mono_class_get_parent(MonoClass* klass)
{
    void* parentHandle = CoreCLR::CallStaticMethodInternal<void*, void*>(TEXT("GetClassParent"), ((CoreCLRClass*)klass)->GetTypeHandle());
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
    return CoreCLR::CallStaticMethodInternal<int, void*>(TEXT("GetTypeMonoTypeEnum"), type);
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
    CoreCLRMethod* mi = (CoreCLRMethod*)sig;
    return (MonoType*)mi->GetReturnType();
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
    CoreCLRMethod* mi = (CoreCLRMethod*)sig;
    return mi->GetNumParameters();
}

MONO_API mono_bool mono_signature_param_is_out(MonoMethodSignature* sig, int param_num)
{
    CoreCLRMethod* mi = (CoreCLRMethod*)sig;
    return CoreCLR::CallStaticMethodInternal<bool, void*, int>(TEXT("GetMethodParameterIsOut"), mi->GetMethodHandle(), param_num);
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
    return (MonoException*)CoreCLR::CallStaticMethodInternal<void*>(TEXT("GetNullReferenceException"));
}

MONO_API MonoException* mono_get_exception_not_supported(const char* msg)
{
    return (MonoException*)CoreCLR::CallStaticMethodInternal<void*>(TEXT("GetNotSupportedException"));
}

MONO_API MONO_RT_EXTERNAL_ONLY MonoException* mono_get_exception_argument_null(const char* arg)
{
    return (MonoException*)CoreCLR::CallStaticMethodInternal<void*>(TEXT("GetArgumentNullException"));
}

MONO_API MonoException* mono_get_exception_argument(const char* arg, const char* msg)
{
    return (MonoException*)CoreCLR::CallStaticMethodInternal<void*>(TEXT("GetArgumentException"));
}

MONO_API MONO_RT_EXTERNAL_ONLY MonoException* mono_get_exception_argument_out_of_range(const char* arg)
{
    return (MonoException*)CoreCLR::CallStaticMethodInternal<void*>(TEXT("GetArgumentOutOfRangeException"));
}

/*
 * image.h
*/

MONO_API MONO_RT_EXTERNAL_ONLY MonoImage* mono_image_open_from_data_with_name(char* data, uint32 data_len, mono_bool need_copy, MonoImageOpenStatus* status, mono_bool refonly, const char* path)
{
    const char* name;
    const char* fullname;
    void* assemblyHandle = CoreCLR::CallStaticMethodInternal<void*, char*, int, const char*, const char**, const char**>(TEXT("LoadAssemblyImage"), data, data_len, path, &name, &fullname);
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
