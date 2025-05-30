// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Core.h"

template<typename FuncType>
struct ScopeExit
{
    explicit ScopeExit(FuncType&& func)
        : _func((FuncType&&)func)
    {
    }

    ~ScopeExit()
    {
        _func();
    }

private:
    FuncType _func;
};

namespace THelpers
{
    struct ScopeExitInternal
    {
        template<typename FuncType>
        ScopeExit<FuncType> operator*(FuncType&& func)
        {
            return ScopeExit<FuncType>((FuncType&&)func);
        }
    };
}

#define SCOPE_EXIT const auto CONCAT_MACROS(__scopeExit, __LINE__) = THelpers::ScopeExitInternal() * [&]()
