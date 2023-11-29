// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#include "CommandLine.h"
#include "Engine/Core/Collections/Array.h"
#include <iostream>

#include "Engine/Core/Types/StringBuilder.h"

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

#if PLATFORM_LINUX | PLATFORM_MAC

// a map as OptionsData would make sense
void CommandLine::SetValue(const char *name, const Char *value)
{
    if (strcmp("-windowed", name) == 0)
        Options.Windowed = true;
    else if (strcmp("-fullscreen", name) == 0)
        Options.Fullscreen = true;
    else if (strcmp("-vsync", name) == 0)
        Options.VSync = true;
    else if (strcmp("-novsync", name) == 0)
        Options.NoVSync = true;
    else if (strcmp("-nolog", name) == 0)
        Options.NoLog = true;
    else if (strcmp("-std", name) == 0)
        Options.Std = true;
#if !BUILD_RELEASE
    else if (strcmp("-debug", name) == 0)
        Options.DebuggerAddress = String(value);
    else if (strcmp("-debugwait", name) == 0)
        Options.WaitForDebugger = true;
#endif
#if PLATFORM_HAS_HEADLESS_MODE
    else if (strcmp("-headless", name) == 0)
        Options.Headless = true;
#endif
    else if (strcmp("-d3d12", name) == 0)
        Options.D3D12 = true;
    else if (strcmp("-d3d11", name) == 0)
        Options.D3D11 = true;
    else if (strcmp("-d3d10", name) == 0)
        Options.D3D10 = true;
    else if (strcmp("-null", name) == 0)
        Options.Null = true;
    else if (strcmp("-vulkan", name) == 0)
        Options.Vulkan = true;
    else if (strcmp("-nvidia", name) == 0)
        Options.NVIDIA = true;
    else if (strcmp("-amd", name) == 0)
        Options.AMD = true;
    else if (strcmp("-intel", name) == 0)
        Options.Intel = true;
    else if (strcmp("-monolog", name) == 0)
        Options.MonoLog = true;
    else if (strcmp("-mute", name) == 0)
        Options.Mute = true;
    else if (strcmp("-lowdpi", name) == 0)
        Options.LowDPI = true;
#if USE_EDITOR
    else if (strcmp("-clearcache", name) == 0)
        Options.ClearCache = true;
    else if (strcmp("-clearcooker", name) == 0)
        Options.ClearCookerCache = true;
    else if (strcmp("-project", name) == 0)
        Options.Project = String(value);
    else if (strcmp("-new", name) == 0)
        Options.NewProject = true;
    else if (strcmp("-genprojectfiles", name) == 0)
        Options.GenProjectFiles = true;
    else if (strcmp("-build", name) == 0)
        Options.Build = String(value);
    else if (strcmp("-skipcompile", name) == 0)
        Options.SkipCompile = true;
    else if (strcmp("-shaderdebug", name) == 0)
        Options.ShaderDebug = true;
    else if (strcmp("-play", name) == 0)
        Options.Play = String(value);
#endif
}

bool CommandLine::Parse(int argc, const char** argv)
{
    enum ArgType { Flag, Value, OptionalValue, Invalid };
    struct OptArg
    {
        const char *name;
        ArgType type;
    };
    OptArg optArgs[] =
    {
        { "-windowed", Flag },
        { "-fullscreen", Flag },
        { "-vsync", Flag },
        { "-novsync", Flag },
        { "-nolog", Flag },
        { "-std", Flag },
#if !BUILD_RELEASE
        { "-debug", Value },
        { "-debugwait", Flag },
#endif
#if PLATFORM_HAS_HEADLESS_MODE
        { "-headless", Flag },
#endif
        { "-d3d12", Flag },
        { "-d3d11", Flag },
        { "-d3d10", Flag },
        { "-null", Flag },
        { "-vulkan", Flag },
        { "-nvidia", Flag },
        { "-amd", Flag },
        { "-intel", Flag },
        { "-monolog", Flag },
        { "-mute", Flag },
        { "-lowdpi", Flag },
#if USE_EDITOR
        { "-clearcache", Flag },
        { "-clearcooker", Flag },
        { "-project", Value },
        { "-new", Flag },
        { "-genprojectfiles", Flag },
        { "-build", Value },
        { "-skipcompile", Flag },
        { "-shaderdebug", Flag },
        { "-play", OptionalValue }
#endif
    };

    StringBuilder cmdString;
    for (int i = 1; i < argc; i++)
    {
        cmdString.Append(argv[i]);

        if (i + 1 != argc)
            cmdString.Append(TEXT(' '));
    }
    cmdString.Append(TEXT('\0'));
    Options.CmdLine = *cmdString;

    int argvP = 1;
    while (argvP < argc)
    {
        OptArg optarg = { "", Invalid };
        const Char *optValue;
        for (int i = 0; i < sizeof(optArgs); i++)
        {
            if (strcmp(argv[argvP], optArgs[i].name) == 0)
            {
                optarg = optArgs[i];
                break;
            }
        }
        if (optarg.type == Invalid)
        {
            std::cerr << "Unknown command line option: " << argv[argvP] << std::endl;
            break;
        }
        if (optarg.type == Flag)
        {
            // set boolean flag, the value is ignored
            SetValue(optarg.name, nullptr);
        }
        else
        {
            if (argvP < argc-1)
            {
                argvP++;
                auto sb = StringBuilder();
                sb.Append(argv[argvP]);
                optValue = *sb;
            }
            else
            {
                if (optarg.type == OptionalValue)
                    optValue = TEXT("");
                else
                {
                    std::cerr << "Command line option " << optarg.name << "requires an argument" << std::endl;
                    break;
                }
            }
            SetValue(optarg.name, optValue);
        }
        argvP++;
    }
#else
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
			Platform::MemoryCopy(pos, pos + len, (end - pos - len) * 2); \
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
			Platform::MemoryCopy(pos, pos + len, (end - pos - len) * 2); \
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
			    Platform::MemoryCopy(pos, pos + len, (end - pos - len) * 2); \
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
    PARSE_ARG_OPT_SWITCH("-play ", Play);

#endif

#endif
    return false;
}

