// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "CoroutineHandle.h"
#include "CoroutineExecutor.h"

bool CoroutineHandle::HasFinished() const
{
    if (Executor == nullptr)
    {
        return true;
    }

    return Executor->HasFinished(*this);
}

bool CoroutineHandle::IsPaused() const
{
    if (Executor == nullptr)
    {
        return false;
    }

    return Executor->IsPaused(*this);
}

bool CoroutineHandle::Cancel()
{
    if (Executor == nullptr)
    {
        return false;
    }

    return Executor->Cancel(*this);
}

bool CoroutineHandle::Pause()
{
    if (Executor == nullptr)
    {
        return false;
    }

    return Executor->Pause(*this);
}

bool CoroutineHandle::Resume()
{
    if (Executor == nullptr)
    {
        return false;
    }

    return Executor->Resume(*this);
}
