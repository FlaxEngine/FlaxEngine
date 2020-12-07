// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#include "MException.h"
#include "ManagedCLR/MUtils.h"
#include <ThirdParty/mono-2.0/mono/metadata/object.h>

MException::MException(MonoObject* exception)
    : InnerException(nullptr)
{
    ASSERT(exception);

    MonoClass* exceptionClass = mono_object_get_class(exception);
    MonoProperty* exceptionMsgProp = mono_class_get_property_from_name(exceptionClass, "Message");
    MonoMethod* exceptionMsgGetter = mono_property_get_get_method(exceptionMsgProp);
    MonoString* exceptionMsg = (MonoString*)mono_runtime_invoke(exceptionMsgGetter, exception, nullptr, nullptr);
    Message = MUtils::ToString(exceptionMsg);

    MonoProperty* exceptionStackProp = mono_class_get_property_from_name(exceptionClass, "StackTrace");
    MonoMethod* exceptionStackGetter = mono_property_get_get_method(exceptionStackProp);
    MonoString* exceptionStackTrace = (MonoString*)mono_runtime_invoke(exceptionStackGetter, exception, nullptr, nullptr);
    StackTrace = MUtils::ToString(exceptionStackTrace);

    MonoProperty* innerExceptionProp = mono_class_get_property_from_name(exceptionClass, "InnerException");
    MonoMethod* innerExceptionGetter = mono_property_get_get_method(innerExceptionProp);
    MonoObject* innerException = (MonoObject*)mono_runtime_invoke(innerExceptionGetter, exception, nullptr, nullptr);
    if (innerException)
        InnerException = New<MException>(innerException);
}

MException::~MException()
{
    if (InnerException)
        Delete(InnerException);
}
