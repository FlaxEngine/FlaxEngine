// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "CoroutineHandle.h"
#include "CoroutineExecutor.h"

bool CoroutineHandle::Cancel()
{
    CHECK_RETURN(Executor != nullptr, false);
    return Executor->Cancel(*this);
}

bool CoroutineHandle::Pause()
{
    CHECK_RETURN(Executor != nullptr, false);
    return Executor->Pause(*this);
}

bool CoroutineHandle::Resume()
{
    CHECK_RETURN(Executor != nullptr, false);
    return Executor->Resume(*this);
}
