// Copyright (c) Wojciech Figat. All rights reserved.

#include "CommandLine.h"
#include "Engine/Core/Utilities.h"
#include "Engine/Core/Collections/Array.h"

static Array<CommandLine::Arg> Args;
CommandLine::OptionsData CommandLine::Options;

bool InitCommandLine(const Char* cmdLine)
{
    auto& options = CommandLine::Options;

#if FLAX_TESTS
    // Configure engine for test running environment
    options.Headless = true;
    options.Null = true;
    options.Mute = true;
    options.Std = true;
#endif

    // Parse command line
    int32 length = StringUtils::Length(cmdLine);
    if (length == 0)
        return false;
    if (CommandLine::Parse(StringView(cmdLine, length), Args))
        return true;

    // Parse common switches
    bool valueBool;
    StringView valueString;
#define PARSE_BOOL_SWITCH(arg, field) if (CommandLine::Get(TEXT(arg), valueBool)) options.field = valueBool
#define PARSE_ARG_SWITCH(arg, field) if (CommandLine::Get(TEXT(arg), valueString)) options.field = valueString
    PARSE_BOOL_SWITCH("windowed", Windowed);
    PARSE_BOOL_SWITCH("fullscreen", Fullscreen);
    PARSE_BOOL_SWITCH("vsync", VSync);
    PARSE_BOOL_SWITCH("novsync", NoVSync);
    PARSE_BOOL_SWITCH("nolog", NoLog);
    PARSE_BOOL_SWITCH("std", Std);
#if PLATFORM_HAS_HEADLESS_MODE
    PARSE_BOOL_SWITCH("headless", Headless);
#endif
    PARSE_BOOL_SWITCH("d3d12", D3D12);
    PARSE_BOOL_SWITCH("d3d11", D3D11);
    PARSE_BOOL_SWITCH("d3d10", D3D10);
    PARSE_BOOL_SWITCH("null", Null);
    PARSE_BOOL_SWITCH("vulkan", Vulkan);
    PARSE_BOOL_SWITCH("nvidia", NVIDIA);
    PARSE_BOOL_SWITCH("amd", AMD);
    PARSE_BOOL_SWITCH("intel", Intel);
    PARSE_BOOL_SWITCH("mute", Mute);
    PARSE_BOOL_SWITCH("lowdpi", LowDPI);
#if PLATFORM_LINUX && PLATFORM_SDL
    PARSE_BOOL_SWITCH("wayland", Wayland);
    PARSE_BOOL_SWITCH("x11", X11);
#endif
#if USE_EDITOR
    PARSE_BOOL_SWITCH("clearcache", ClearCache);
    PARSE_BOOL_SWITCH("clearcooker", ClearCookerCache);
    PARSE_ARG_SWITCH("project", Project);
    PARSE_BOOL_SWITCH("lastproject", LastProject);
    PARSE_BOOL_SWITCH("new", NewProject);
    PARSE_BOOL_SWITCH("genprojectfiles", GenProjectFiles);
    PARSE_ARG_SWITCH("build", Build);
    PARSE_BOOL_SWITCH("skipcompile", SkipCompile);
    PARSE_BOOL_SWITCH("shaderdebug", ShaderDebug);
    PARSE_BOOL_SWITCH("exit", Exit);
    PARSE_ARG_SWITCH("play", Play);
#endif
#if USE_EDITOR || !BUILD_RELEASE
    PARSE_BOOL_SWITCH("shaderprofile", ShaderProfile);
    PARSE_BOOL_SWITCH("gpudebug", GPUDebug);
#endif
#undef PARSE_BOOL_SWITCH
#undef PARSE_ARG_SWITCH

    return false;
}

bool CommandLine::Get(const StringView& arg)
{
    StringView value;
    return Get(arg, value);
}

bool CommandLine::Get(const StringView& arg, StringView& value)
{
    for (const Arg& a : Args)
    {
        if (a.Name.Length() == arg.Length() && StringUtils::CompareIgnoreCase(*a.Name, *arg, arg.Length()) == 0)
        {
            value = a.Value;
            return true;
        }
    }
    return false;
}

bool CommandLine::Get(const StringView& arg, bool& value)
{
    StringView str;
    if (Get(arg, str))
    {
        value = true; // Assume true when arg is set
        if (str.HasChars())
            StringUtils::Parse(str.Get(), str.Length(), &value);
        return true;
    }
    return false;
}

bool CommandLine::Get(const StringView& arg, int32& value)
{
    StringView str;
    if (Get(arg, str))
    {
        value = 0;
        if (str.HasChars())
            StringUtils::Parse(str.Get(), str.Length(), &value);
        return true;
    }
    return false;
}

bool CommandLine::Parse(const StringView& commandLine, Array<Arg>& args)
{
    int32 length = commandLine.Length();
    bool addedEmptyArg = false;
    for (int32 i = 0; i < length;)
    {
        // Skip white space
        while (i < length && StringUtils::IsWhitespace(commandLine[i]))
            i++;

        // Read option prefix
        if (i == length)
            break;
        bool wholeQuote = commandLine[i] == '\"';
        if (wholeQuote)
            i++;
        if (i == length)
            break;
        if (commandLine[i] == '-')
            i++;
        else if (commandLine[i] == '/')
            i++;

        // Skip white space
        while (i < length && StringUtils::IsWhitespace(commandLine[i]))
            i++;

        // Read option name
        int32 nameStart = i;
        while (i < length && commandLine[i] != '-' && commandLine[i] != '=' && !StringUtils::IsWhitespace(commandLine[i]))
            i++;
        if (wholeQuote)
            i--;
        int32 nameEnd = i;
        StringView name = commandLine.Substring(nameStart, nameEnd - nameStart);

        // If previous argument was empty and this one is a quote then assume it's a value for the previous argument (without '=')
        if (wholeQuote && addedEmptyArg)
        {
            args.Last().Value = name;
            i++;
            continue;
        }

        // Skip white space
        while (i < length && StringUtils::IsWhitespace(commandLine[i]))
            i++;

        // Check if has no value
        if (i >= length - 1 || commandLine[i] != '=')
        {
            args.Add({ name, StringView::Empty });
            addedEmptyArg = true;
            if (wholeQuote)
                i++;
            if (i < length && commandLine[i] != '\"')
                i++;
            continue;
        }
        addedEmptyArg = false;

        // Read value
        i++;
        int32 valueStart, valueEnd;
        if (length > i + 1 && commandLine[i] == '\\' && commandLine[i + 1] == '\"')
        {
            valueStart = i + 2;
            i++;
            while (i + 1 < length && commandLine[i] != '\\' && commandLine[i + 1] != '\"')
                i++;
            valueEnd = i;
            i += 2;
            if (wholeQuote)
            {
                while (i < length && commandLine[i] != '\"')
                    i++;
                i++;
            }
        }
        else if (commandLine[i] == '\"' || commandLine[i] == '\'')
        {
            Char quoteChar = commandLine[i];
            valueStart = i + 1;
            i++;
            while (i < length && commandLine[i] != quoteChar)
                i++;
            valueEnd = i;
            i++;
            if (wholeQuote)
            {
                while (i < length && commandLine[i] != '\"')
                    i++;
                i++;
            }
        }
        else if (wholeQuote)
        {
            valueStart = i;
            while (i < length && commandLine[i] != '\"')
                i++;
            valueEnd = i;
            i++;
        }
        else
        {
            valueStart = i;
            while (i < length && commandLine[i] != ' ')
                i++;
            valueEnd = i;
        }
        StringView value = commandLine.Substring(valueStart, valueEnd - valueStart);
        value = value.Trim();
        if (value.StartsWith(TEXT("\\\"")) && value.EndsWith(TEXT("\\\"")))
            value = value.Substring(2, value.Length() - 4);
        args.Add({ name, value });
    }

    return false;
}
