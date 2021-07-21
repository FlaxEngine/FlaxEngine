// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "Log.h"
#include "Engine/Engine/CommandLine.h"
#include "Engine/Core/Types/DateTime.h"
#include "Engine/Core/Collections/Sorting.h"
#include "Engine/Engine/Time.h"
#include "Engine/Engine/Globals.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Platform/CriticalSection.h"
#include "Engine/Serialization/FileWriteStream.h"
#include "Engine/Serialization/MemoryWriteStream.h"
#include "Engine/Debug/Exceptions/Exceptions.h"
#include <iostream>

#define LOG_ENABLE_FILE (!PLATFORM_SWITCH)

namespace
{
    bool LogAfterInit = false, IsDuringLog = false;
    int LogTotalErrorsCnt = 0;
    FileWriteStream* LogFile = nullptr;
    CriticalSection LogLocker;
    DateTime LogStartTime;
}

String Log::Logger::LogFilePath;
Delegate<LogType, const StringView&> Log::Logger::OnMessage;
Delegate<LogType, const StringView&> Log::Logger::OnError;

bool Log::Logger::Init()
{
    // Skip if disabled
    if (!IsLogEnabled())
        return false;

    // Create logs directory (if is missing)
#if USE_EDITOR
    const String logsDirectory = Globals::ProjectFolder / TEXT("Logs");
#else
    const String logsDirectory = Globals::ProductLocalFolder / TEXT("Logs");
#endif
    if (FileSystem::CreateDirectory(logsDirectory))
    {
        return true;
    }

#if USE_EDITOR
    // Remove old log files
    int32 filesDeleted = 0;
    Array<String> oldLogs;
    if (!FileSystem::DirectoryGetFiles(oldLogs, logsDirectory, TEXT("*.txt"), DirectorySearchOption::TopDirectoryOnly))
    {
        // Check if there are any files to delete
        const int32 maxLogFiles = 20;
        int32 remaining = oldLogs.Count() - maxLogFiles + 1;
        if (remaining > 0)
        {
            Sorting::QuickSort(oldLogs.Get(), oldLogs.Count());

            // Delete the oldest logs
            int32 i = 0;
            while (remaining > 0)
            {
                FileSystem::DeleteFile(oldLogs[i++]);
                filesDeleted++;
                remaining--;
            }
        }
    }
#endif

    // Create log file path
    LogStartTime = Time::StartupTime;
    const String filename = TEXT("Log_") + LogStartTime.ToFileNameString() + TEXT(".txt");
    LogFilePath = logsDirectory / filename;

    // Open file
    LogFile = FileWriteStream::Open(LogFilePath);
    if (LogFile == nullptr)
    {
        return true;
    }
    LogTotalErrorsCnt = 0;
    LogAfterInit = true;

    // Write BOM (UTF-16 (LE); BOM: FF FE)
    byte bom[] = { 0xFF, 0xFE };
    LogFile->Write(bom, 2);

    // Write startup info
    WriteFloor();
    Write(String::Format(TEXT("           Start of the log, {0}"), LogStartTime.ToString()));
#if USE_EDITOR
    if (filesDeleted > 0)
        Write(String::Format(TEXT("                  Deleted {0} old log files"), filesDeleted));
#endif
    WriteFloor();

    return false;
}

void Log::Logger::Write(const StringView& msg)
{
    const auto ptr = msg.Get();
    const auto length = msg.Length();

    LogLocker.Lock();
    if (IsDuringLog)
    {
        LogLocker.Unlock();
        return;
    }
    IsDuringLog = true;

    // Send message to standard process output
    if (CommandLine::Options.Std)
    {
#if PLATFORM_TEXT_IS_CHAR16
        StringAnsi ansi(msg);
        ansi += PLATFORM_LINE_TERMINATOR;
        printf("%s", ansi.Get());
#else
        std::wcout.write(ptr, length);
        std::wcout.write(TEXT(PLATFORM_LINE_TERMINATOR), ARRAY_COUNT(PLATFORM_LINE_TERMINATOR) - 1);
#endif
    }

    // Send message to platform logging
    Platform::Log(msg);

    // Write message to log file
    if (LogAfterInit)
    {
        LogFile->Write(ptr, length);
        LogFile->Write(TEXT(PLATFORM_LINE_TERMINATOR), ARRAY_COUNT(PLATFORM_LINE_TERMINATOR) - 1);
#if LOG_ENABLE_AUTO_FLUSH
        LogFile->Flush();
#endif
    }

    IsDuringLog = false;
    LogLocker.Unlock();
}

void Log::Logger::Write(const Exception& exception)
{
    Write(exception.GetLevel(), exception.ToString());
}

void Log::Logger::Dispose()
{
    LogLocker.Lock();

    // Write ending info
    WriteFloor();
    Write(String::Format(TEXT(" Total errors: {0}\n Closing file"), LogTotalErrorsCnt, DateTime::Now().ToString()));
    WriteFloor();

    // Close
    if (LogAfterInit)
    {
        LogAfterInit = false;
        LogFile->Close();
        Delete(LogFile);
        LogFile = nullptr;
    }

    LogLocker.Unlock();
}

bool Log::Logger::IsLogEnabled()
{
#if LOG_ENABLE && LOG_ENABLE_FILE
    return !CommandLine::Options.NoLog.HasValue();
#else
	return false;
#endif
}

void Log::Logger::Flush()
{
    LogLocker.Lock();
    if (LogFile)
        LogFile->Flush();
    LogLocker.Unlock();
}

void Log::Logger::WriteFloor()
{
    Write(TEXT("================================================================"));
}

void Log::Logger::ProcessLogMessage(LogType type, const StringView& msg, fmt_flax::memory_buffer& w)
{
    const TimeSpan time = DateTime::Now() - LogStartTime;

    fmt_flax::format(w, TEXT("[ {0} ]: [{1}] "), *time.ToString('a'), ToString(type));

    // On Windows convert all '\n' into '\r\n'
#if PLATFORM_WINDOWS
    const int32 msgLength = msg.Length();
    if (msgLength > 1)
    {
        MemoryWriteStream msgStream(msgLength * sizeof(Char));
        msgStream.WriteChar(msg[0]);
        for (int32 i = 1; i < msgLength; i++)
        {
            if (msg[i - 1] != '\r' && msg[i] == '\n')
                msgStream.WriteChar(TEXT('\r'));
            msgStream.WriteChar(msg[i]);
        }
        msgStream.WriteChar(msg[msgLength]);
        msgStream.WriteChar(TEXT('\0'));
        fmt_flax::format(w, TEXT("{}"), (const Char*)msgStream.GetHandle());
        //w.append(msgStream.GetHandle(), msgStream.GetHandle() + msgStream.GetPosition());
    }
    else
    {
        //w.append(msg.Get(), msg.Get() + msg.Length());
        fmt_flax::format(w, TEXT("{}"), (const Char*)msg.Get());
    }
#else
    fmt_flax::format(w, TEXT("{}"), (const Char*)msg.Get());
#endif
}

void Log::Logger::Write(LogType type, const StringView& msg)
{
    const bool isError = IsError(type);

    // Create message for the log file
    fmt_flax::memory_buffer w;
    ProcessLogMessage(type, msg, w);

    // Log formatted message
    Write(StringView(w.data(), (int32)w.size()));

    // Fire events
    OnMessage(type, msg);
    if (isError)
    {
        LogTotalErrorsCnt++;
        OnError(type, msg);
    }

    // Check if need to show message box with that log message
    if (type == LogType::Fatal)
    {
        Flush();

        // Process message further
        if (type == LogType::Fatal)
            Platform::Fatal(msg);
        else if (type == LogType::Error)
            Platform::Error(msg);
        else
            Platform::Info(msg);
    }
}

const Char* ToString(LogType e)
{
    const Char* result;
    switch (e)
    {
    case LogType::Info:
        result = TEXT("Info");
        break;
    case LogType::Warning:
        result = TEXT("Warning");
        break;
    case LogType::Error:
        result = TEXT("Error");
        break;
    case LogType::Fatal:
        result = TEXT("Fatal");
        break;
    default:
        result = TEXT("");
    }
    return result;
}
