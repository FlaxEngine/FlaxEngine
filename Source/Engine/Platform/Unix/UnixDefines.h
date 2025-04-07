// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_UNIX

// Platform description
#define PLATFORM_LINE_TERMINATOR "\n"
#define PLATFORM_TEXT_IS_CHAR16 1

#define LOG_UNIX_LAST_ERROR LOG(Warning, "Unix::errno(): {0}", String(strerror(errno)))

#endif
