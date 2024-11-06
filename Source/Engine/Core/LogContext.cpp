// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "LogContext.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Types/Guid.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Core/Types/StringBuilder.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Scripting/Scripting.h"
#include "Engine/Scripting/Script.h"
#include "Engine/Content/Asset.h"
#include "Engine/Content/Content.h"
#include "Engine/Level/Actor.h"
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

    LogContextData Peek() const
    {
        return Count > 0 ? Ptr[Count - 1] : LogContextData();
    }
};

ThreadLocal<LogContextThreadData> GlobalLogContexts;

void LogContext::Print(LogType verbosity)
{
    auto& stack = GlobalLogContexts.Get();
    if (stack.Count == 0)
        return;
    const StringView indentation(TEXT("    "));
    StringBuilder msg;
    for (int32 index = (int32)stack.Count - 1; index >= 0; index--)
    {
        LogContextData& context = stack.Ptr[index];

        // Skip duplicates
        if (index < (int32)stack.Count - 1 && stack.Ptr[stack.Count - 1] == context)
            continue;

        // Build call hierarchy via indentation
        msg.Clear();
        for (uint32 i = index; i < stack.Count; i++)
            msg.Append(indentation);

        if (context.ObjectID != Guid::Empty)
        {
            // Object reference context
            msg.Append(TEXT(" Referenced by "));
            if (ScriptingObject* object = Scripting::TryFindObject(context.ObjectID))
            {
                const StringAnsiView typeName(object->GetType().Fullname);
                if (Asset* asset = ScriptingObject::Cast<Asset>(object))
                {
                    msg.AppendFormat(TEXT("asset '{}' ({}, {})"), asset->GetPath(), asset->GetTypeName(), context.ObjectID);
                }
                else if (Actor* actor = ScriptingObject::Cast<Actor>(object))
                {
                    msg.AppendFormat(TEXT("actor '{}' ({}, {})"), actor->GetNamePath(), String(typeName), context.ObjectID);
                }
                else if (Script* script = ScriptingObject::Cast<Script>(object))
                {
                    msg.AppendFormat(TEXT("script '{}' ({}, {})"), script->GetNamePath(), String(typeName), context.ObjectID);
                }
                else
                {
                    msg.AppendFormat(TEXT("object {} ({})"), String(typeName), context.ObjectID);
                }
            }
            else if (Asset* asset = Content::GetAsset(context.ObjectID))
            {
                msg.AppendFormat(TEXT("asset '{}' ({}, {})"), asset->GetPath(), asset->GetTypeName(), context.ObjectID);
            }
            else
            {
                msg.AppendFormat(TEXT("object {}"), context.ObjectID);
            }
        }

        // Print message
        Log::Logger::Write(verbosity, msg.ToStringView());
    }
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
