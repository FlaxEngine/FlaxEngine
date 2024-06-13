// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "LogContext.h"
#include "Engine/Core/Types/Guid.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Threading/ThreadLocal.h"

struct LogContextThreadData
{
    LogContextData* Ptr;
    uint32 Count;
    uint32 Capacity;

    void Push(LogContextData&& item)
    {
        if (Count == Capacity)
        {
            Capacity = Capacity == 0 ? 32 : Capacity * 2;
            auto ptr = (LogContextData*)Allocator::Allocate(Capacity * sizeof(LogContextData));
            Platform::MemoryCopy(ptr, Ptr, Count * sizeof(LogContextData));
            Allocator::Free(Ptr);
            Ptr = ptr;
        }
        Ptr[Count++] = MoveTemp(item);
    }

    void Pop()
    {
        Count--;
    }

    LogContextData Peek()
    {
        return Count > 0 ? Ptr[Count - 1] : LogContextData();
    }
};

ThreadLocal<LogContextThreadData> GlobalLogContexts;

String LogContext::GetInfo()
{
    LogContextData context = LogContext::Get();
    if (context.ObjectID != Guid::Empty)
        return String::Format(TEXT("(Loading source was {0})"), context.ObjectID);
    return String::Empty;
}

void LogContext::Push(const Guid& id)
{
    LogContextData context;
    context.ObjectID = id;
    auto& stack = GlobalLogContexts.Get();
    stack.Push(MoveTemp(context));
}

void LogContext::Pop()
{
    auto& stack = GlobalLogContexts.Get();
    stack.Pop();
}

LogContextData LogContext::Get()
{
    auto& stack = GlobalLogContexts.Get();
    return stack.Peek();
}
