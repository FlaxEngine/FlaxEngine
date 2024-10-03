// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "Engine/Scripting/Types.h"
#if !USE_CSHARP
#include "Engine/Core/Types/Span.h"
#include "Engine/Scripting/ManagedCLR/MCore.h"
#include "Engine/Scripting/ManagedCLR/MDomain.h"
#include "Engine/Scripting/ManagedCLR/MAssembly.h"
#include "Engine/Scripting/ManagedCLR/MClass.h"
#include "Engine/Scripting/ManagedCLR/MDomain.h"
#include "Engine/Scripting/ManagedCLR/MEvent.h"
#include "Engine/Scripting/ManagedCLR/MException.h"
#include "Engine/Scripting/ManagedCLR/MField.h"
#include "Engine/Scripting/ManagedCLR/MMethod.h"
#include "Engine/Scripting/ManagedCLR/MProperty.h"

extern MDomain* MRootDomain;
extern Array<MDomain*, FixedAllocation<4>> MDomains;

MDomain* MCore::CreateDomain(const StringAnsi& domainName)
{
    for (int32 i = 0; i < MDomains.Count(); i++)
    {
        if (MDomains[i]->GetName() == domainName)
            return MDomains[i];
    }
    auto domain = New<MDomain>(domainName);
    MDomains.Add(domain);
    return domain;
}

void MCore::UnloadDomain(const StringAnsi& domainName)
{
    int32 i = 0;
    for (; i < MDomains.Count(); i++)
    {
        if (MDomains[i]->GetName() == domainName)
            break;
    }
    if (i == MDomains.Count())
        return;

    auto domain = MDomains[i];
    Delete(domain);
    MDomains.RemoveAtKeepOrder(i);
}

bool MCore::LoadEngine()
{
    MRootDomain = New<MDomain>("Root");
    MDomains.Add(MRootDomain);
    return false;
}

void MCore::UnloadEngine()
{
    MDomains.ClearDelete();
    MRootDomain = nullptr;
}

#if USE_EDITOR

void MCore::ReloadScriptingAssemblyLoadContext()
{
}

#endif

MObject* MCore::Object::Box(void* value, const MClass* klass)
{
    return nullptr;
}

void* MCore::Object::Unbox(MObject* obj)
{
    return nullptr;
}

MObject* MCore::Object::New(const MClass* klass)
{
    return nullptr;
}

void MCore::Object::Init(MObject* obj)
{
}

MClass* MCore::Object::GetClass(MObject* obj)
{
    return nullptr;
}

MString* MCore::Object::ToString(MObject* obj)
{
    return nullptr;
}

int32 MCore::Object::GetHashCode(MObject* obj)
{
    return 0;
}

MString* MCore::String::GetEmpty(MDomain* domain)
{
    return nullptr;
}

MString* MCore::String::New(const char* str, int32 length, MDomain* domain)
{
    return nullptr;
}

MString* MCore::String::New(const Char* str, int32 length, MDomain* domain)
{
    return nullptr;
}

StringView MCore::String::GetChars(MString* obj)
{
    return StringView::Empty;
}

MArray* MCore::Array::New(const MClass* elementKlass, int32 length)
{
    return nullptr;
}

MClass* MCore::Array::GetClass(MClass* elementKlass)
{
    return nullptr;
}

MClass* MCore::Array::GetArrayClass(const MArray* obj)
{
    return nullptr;
}

int32 MCore::Array::GetLength(const MArray* obj)
{
    return 0;
}

void* MCore::Array::GetAddress(const MArray* obj)
{
    return nullptr;
}

MArray* MCore::Array::Unbox(MObject* obj)
{
    return nullptr;
}

MGCHandle MCore::GCHandle::New(MObject* obj, bool pinned)
{
    return (MGCHandle)(uintptr)obj;
}

MGCHandle MCore::GCHandle::NewWeak(MObject* obj, bool trackResurrection)
{
    return (MGCHandle)(uintptr)obj;
}

MObject* MCore::GCHandle::GetTarget(const MGCHandle& handle)
{
    return (MObject*)(uintptr)handle;
}

void MCore::GCHandle::Free(const MGCHandle& handle)
{
}

void MCore::GC::Collect()
{
}

void MCore::GC::Collect(int32 generation)
{
}

void MCore::GC::Collect(int32 generation, MGCCollectionMode collectionMode, bool blocking, bool compacting)
{
}

int32 MCore::GC::MaxGeneration()
{
    return 0;
}

void MCore::GC::WaitForPendingFinalizers()
{
}

void MCore::GC::WriteRef(void* ptr, MObject* ref)
{
}

void MCore::GC::WriteValue(void* dst, void* src, int32 count, const MClass* klass)
{
}

void MCore::GC::WriteArrayRef(MArray* dst, MObject* ref, int32 index)
{
}

void MCore::GC::WriteArrayRef(MArray* dst, Span<MObject*> refs)
{
}

void MCore::Thread::Attach()
{
}

void MCore::Thread::Exit()
{
}

bool MCore::Thread::IsAttached()
{
    return true;
}

void MCore::Exception::Throw(MObject* exception)
{
}

MObject* MCore::Exception::GetNullReference()
{
    return nullptr;
}

MObject* MCore::Exception::Get(const char* msg)
{
    return nullptr;
}

MObject* MCore::Exception::GetArgument(const char* arg, const char* msg)
{
    return nullptr;
}

MObject* MCore::Exception::GetArgumentNull(const char* arg)
{
    return nullptr;
}

MObject* MCore::Exception::GetArgumentOutOfRange(const char* arg)
{
    return nullptr;
}

MObject* MCore::Exception::GetNotSupported(const char* msg)
{
    return nullptr;
}

::String MCore::Type::ToString(MType* type)
{
    return ::String::Empty;
}

MClass* MCore::Type::GetClass(MType* type)
{
    return nullptr;
}

MType* MCore::Type::GetElementType(MType* type)
{
    return nullptr;
}

int32 MCore::Type::GetSize(MType* type)
{
    return 0;
}

MTypes MCore::Type::GetType(MType* type)
{
    return MTypes::End;
}

bool MCore::Type::IsPointer(MType* type)
{
    return false;
}

bool MCore::Type::IsReference(MType* type)
{
    return false;
}

const MAssembly::ClassesDictionary& MAssembly::GetClasses() const
{
    _hasCachedClasses = true;
    return _classes;
}

bool MAssembly::LoadCorlib()
{
    return false;
}

bool MAssembly::LoadImage(const String& assemblyPath, const StringView& nativePath)
{
    _hasCachedClasses = false;
    _assemblyPath = assemblyPath;
    return false;
}

bool MAssembly::UnloadImage(bool isReloading)
{
    return false;
}

bool MAssembly::ResolveMissingFile(String& assemblyPath) const
{
    return true;
}

MClass::~MClass()
{
    _fields.ClearDelete();
    _properties.ClearDelete();
    _methods.ClearDelete();
    _events.ClearDelete();
}

MClass* MClass::GetBaseClass() const
{
    return nullptr;
}

bool MClass::IsSubClassOf(const MClass* klass, bool checkInterfaces) const
{
    return false;
}

bool MClass::HasInterface(const MClass* klass) const
{
    return false;
}

bool MClass::IsInstanceOfType(MObject* object) const
{
    return false;
}

uint32 MClass::GetInstanceSize() const
{
    return 0;
}

MMethod* MClass::GetMethod(const char* name, int32 numParams) const
{
    return nullptr;
}

const Array<MMethod*>& MClass::GetMethods() const
{
    _hasCachedMethods = true;
    return _methods;
}

MField* MClass::GetField(const char* name) const
{
    return nullptr;
}

const Array<MField*>& MClass::GetFields() const
{
    _hasCachedFields = true;
    return _fields;
}

const Array<MEvent*>& MClass::GetEvents() const
{
    _hasCachedEvents = true;
    return _events;
}

MProperty* MClass::GetProperty(const char* name) const
{
    return nullptr;
}

const Array<MProperty*>& MClass::GetProperties() const
{
    _hasCachedProperties = true;
    return _properties;
}

bool MClass::HasAttribute(const MClass* klass) const
{
    return false;
}

bool MClass::HasAttribute() const
{
    return false;
}

MObject* MClass::GetAttribute(const MClass* klass) const
{
    return nullptr;
}

const Array<MObject*>& MClass::GetAttributes() const
{
    _hasCachedAttributes = true;
    return _attributes;
}

bool MDomain::SetCurrentDomain(bool force)
{
    extern MDomain* MActiveDomain;
    MActiveDomain = this;
    return true;
}

void MDomain::Dispatch() const
{
}

MMethod* MEvent::GetAddMethod() const
{
    return _addMethod;
}

MMethod* MEvent::GetRemoveMethod() const
{
    return _removeMethod;
}

bool MEvent::HasAttribute(const MClass* klass) const
{
    return false;
}

bool MEvent::HasAttribute() const
{
    return false;
}

MObject* MEvent::GetAttribute(const MClass* klass) const
{
    return nullptr;
}

const Array<MObject*>& MEvent::GetAttributes() const
{
    return _attributes;
}

MException::MException(MObject* exception)
    : InnerException(nullptr)
{
}

MException::~MException()
{
}

MType* MField::GetType() const
{
    return nullptr;
}

int32 MField::GetOffset() const
{
    return 0;
}

void MField::GetValue(MObject* instance, void* result) const
{
}

MObject* MField::GetValueBoxed(MObject* instance) const
{
    return nullptr;
}

void MField::SetValue(MObject* instance, void* value) const
{
}

bool MField::HasAttribute(const MClass* klass) const
{
    return false;
}

bool MField::HasAttribute() const
{
    return false;
}

MObject* MField::GetAttribute(const MClass* klass) const
{
    return nullptr;
}

const Array<MObject*>& MField::GetAttributes() const
{
    return _attributes;
}

MObject* MMethod::Invoke(void* instance, void** params, MObject** exception) const
{
    return nullptr;
}

MObject* MMethod::InvokeVirtual(MObject* instance, void** params, MObject** exception) const
{
    return nullptr;
}

MMethod* MMethod::InflateGeneric() const
{
    return nullptr;
}

MType* MMethod::GetReturnType() const
{
    return nullptr;
}

int32 MMethod::GetParametersCount() const
{
    return 0;
}

MType* MMethod::GetParameterType(int32 paramIdx) const
{
    return nullptr;
}

bool MMethod::GetParameterIsOut(int32 paramIdx) const
{
    return false;
}

bool MMethod::HasAttribute(const MClass* klass) const
{
    return false;
}

bool MMethod::HasAttribute() const
{
    return false;
}

MObject* MMethod::GetAttribute(const MClass* klass) const
{
    return nullptr;
}

const Array<MObject*>& MMethod::GetAttributes() const
{
    return _attributes;
}

MProperty::~MProperty()
{
    if (_getMethod)
        Delete(_getMethod);
    if (_setMethod)
        Delete(_setMethod);
}

MMethod* MProperty::GetGetMethod() const
{
    return _getMethod;
}

MMethod* MProperty::GetSetMethod() const
{
    return _setMethod;
}

MObject* MProperty::GetValue(MObject* instance, MObject** exception) const
{
    return nullptr;
}

void MProperty::SetValue(MObject* instance, void* value, MObject** exception) const
{
}

bool MProperty::HasAttribute(const MClass* klass) const
{
    return false;
}

bool MProperty::HasAttribute() const
{
    return false;
}

MObject* MProperty::GetAttribute(const MClass* klass) const
{
    return nullptr;
}

const Array<MObject*>& MProperty::GetAttributes() const
{
    return _attributes;
}

void MCore::ScriptingObject::SetInternalValues(MClass* klass, MObject* object, void* unmanagedPtr, const Guid* id)
{
}

MObject* MCore::ScriptingObject::CreateScriptingObject(MClass* klass, void* unmanagedPtr, const Guid* id)
{
    return nullptr;
}

#endif
