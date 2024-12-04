// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "CommandLine.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Utilities.h"
#include <iostream>

CommandLine::OptionsData CommandLine::Options;

bool ParseArg(Char* ptr, Char*& start, Char*& end)
{
    // Skip leading whitespaces
    while (*ptr && StringUtils::IsWhitespace(*ptr))
        ptr++;

    bool isInComma = false;
    bool isUglyQuote = false;
    start = ptr;
    while (*ptr)
    {
        Char c = *ptr;
        if (StringUtils::IsWhitespace(c) && !isInComma)
        {
            // End
            end = ptr;
            return false;
        }
        else if (c == '\"' || c == '\'')
        {
            if (isInComma)
            {
                // End comma
                end = ptr;
                if (isUglyQuote)
                {
                    ptr += 2;
                    end -= 2;
                }
                return false;
            }
            else
            {
                // Special case (eg. Visual Studio Code adds soo many quotes to the args with spaces)
                isUglyQuote = StringUtils::Compare(ptr, TEXT("\"\\\\\""), 4) == 0;
                if (isUglyQuote)
                {
                    ptr += 3;
                }

                // Start comma
                isInComma = true;
                start = ptr + 1;
            }
        }

        ptr++;
    }
    return true;
}

bool CommandLine::Parse(const Char* cmdLine)
{
    Options.CmdLine = cmdLine;

    int32 length = StringUtils::Length(cmdLine);
    if (length == 0)
        return false;
    Array<Char> buffer;
    buffer.Resize(length + 2);
    Platform::MemoryCopy(buffer.Get(), cmdLine, sizeof(Char) * length);
    buffer[length++] = ' ';
    buffer[length++] = 0;
    Char* end = buffer.Get() + length;

    Char* pos;
    Char* argStart;
    Char* argEnd;
    int32 len;

#define PARSE_BOOL_SWITCH(text, field) \
		pos = (Char*)StringUtils::FindIgnoreCase(buffer.Get(), TEXT(text)); \
		if (pos) \
		{ \
			len = ARRAY_COUNT(text) - 1; \
			Utilities::UnsafeMemoryCopy(pos, pos + len, (end - pos - len) * 2); \
			*(end - len) = 0; \
			end -= len; \
			Options.field = true; \
		}
#define PARSE_ARG_SWITCH(text, field) \
		pos = (Char*)StringUtils::FindIgnoreCase(buffer.Get(), TEXT(text)); \
		if (pos) \
		{ \
			len = ARRAY_COUNT(text) - 1; \
			if (ParseArg(pos + len, argStart, argEnd)) \
			{ \
				std::cout << "Failed to parse argument." << std::endl; \
				return true; \
			} \
			Options.field = String(argStart, static_cast<int32>(argEnd - argStart)); \
			len = static_cast<int32>((argEnd - pos) + 1); \
			Utilities::UnsafeMemoryCopy(pos, pos + len, (end - pos - len) * 2); \
			*(end - len) = 0; \
			end -= len; \
		}
    
#define PARSE_ARG_OPT_SWITCH(text, field) \
		pos = (Char*)StringUtils::FindIgnoreCase(buffer.Get(), TEXT(text)); \
		if (pos) \
		{ \
			len = ARRAY_COUNT(text) - 1; \
			if (ParseArg(pos + len, argStart, argEnd)) \
				Options.field = String::Empty; \
			else \
            { \
			    Options.field = String(argStart, static_cast<int32>(argEnd - argStart)); \
			    len = static_cast<int32>((argEnd - pos) + 1); \
			    Utilities::UnsafeMemoryCopy(pos, pos + len, (end - pos - len) * 2); \
			    *(end - len) = 0; \
			    end -= len; \
			} \
		}

    PARSE_BOOL_SWITCH("-windowed ", Windowed);
    PARSE_BOOL_SWITCH("-fullscreen ", Fullscreen);
    PARSE_BOOL_SWITCH("-vsync ", VSync);
    PARSE_BOOL_SWITCH("-novsync ", NoVSync);
    PARSE_BOOL_SWITCH("-nolog ", NoLog);
    PARSE_BOOL_SWITCH("-std ", Std);
#if !BUILD_RELEASE
    PARSE_ARG_SWITCH("-debug ", DebuggerAddress);
    PARSE_BOOL_SWITCH("-debugwait ", WaitForDebugger);
#endif
#if PLATFORM_HAS_HEADLESS_MODE
    PARSE_BOOL_SWITCH("-headless ", Headless);
#endif
    PARSE_BOOL_SWITCH("-d3d12 ", D3D12);
    PARSE_BOOL_SWITCH("-d3d11 ", D3D11);
    PARSE_BOOL_SWITCH("-d3d10 ", D3D10);
    PARSE_BOOL_SWITCH("-null ", Null);
    PARSE_BOOL_SWITCH("-vulkan ", Vulkan);
    PARSE_BOOL_SWITCH("-nvidia ", NVIDIA);
    PARSE_BOOL_SWITCH("-amd ", AMD);
    PARSE_BOOL_SWITCH("-intel ", Intel);
    PARSE_BOOL_SWITCH("-monolog ", MonoLog);
    PARSE_BOOL_SWITCH("-mute ", Mute);
    PARSE_BOOL_SWITCH("-lowdpi ", LowDPI);
#if USE_EDITOR
    PARSE_BOOL_SWITCH("-clearcache ", ClearCache);
    PARSE_BOOL_SWITCH("-clearcooker ", ClearCookerCache);
    PARSE_ARG_SWITCH("-project ", Project);
    PARSE_BOOL_SWITCH("-new ", NewProject);
    PARSE_BOOL_SWITCH("-genprojectfiles ", GenProjectFiles);
    PARSE_ARG_SWITCH("-build ", Build);
    PARSE_BOOL_SWITCH("-skipcompile ", SkipCompile);
    PARSE_BOOL_SWITCH("-shaderdebug ", ShaderDebug);
    PARSE_BOOL_SWITCH("-exit ", Exit);
    PARSE_ARG_OPT_SWITCH("-play ", Play);
#endif
#if USE_EDITOR || !BUILD_RELEASE
    PARSE_BOOL_SWITCH("-shaderprofile ", ShaderProfile);
#endif

    return false;
}
