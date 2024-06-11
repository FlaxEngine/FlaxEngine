#include "LogContexts.h"
#include "Engine/Core/Types/Guid.h"
#include "Engine/Core/Types/String.h"

ThreadLocal<LogContextStack> LogContexts::CurrentLogContext;

String LogContextFormatter::Format()
{
    LogContextData context = LogContexts::Get();
    if (context.ObjectID != Guid::Empty)
        return String::Format(TEXT("(Loading source was {0})"), context.ObjectID);

    return String("");
}

void LogContexts::Set(const Guid& id)
{
    LogContextData context = LogContextData();
    context.ObjectID = id;

    LogContextStack contextStack = LogContexts::CurrentLogContext.Get();
    contextStack.Stack.Push(context);

    LogContexts::CurrentLogContext.Set(contextStack);
}

void LogContexts::Clear()
{
    LogContextStack contextStack = LogContexts::CurrentLogContext.Get();
    contextStack.Stack.Pop();

    LogContexts::CurrentLogContext.Set(contextStack);
}

LogContextData LogContexts::Get()
{
    LogContextStack contextStack = LogContexts::CurrentLogContext.Get();
    if (contextStack.Stack.Count() == 0)
        return LogContextData();

    return contextStack.Stack.Last();
}
