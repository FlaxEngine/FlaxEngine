// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "Exception.h"

Log::Exception::~Exception()
{
    // Always write exception to the log
    Logger::Write(_level, ToString());
}
