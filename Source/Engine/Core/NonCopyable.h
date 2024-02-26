// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

struct NonCopyable
{
    NonCopyable() = default;
    NonCopyable(const NonCopyable&) = delete;
    NonCopyable& operator=(const NonCopyable&) = delete;
};
