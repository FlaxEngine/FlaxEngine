// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/String.h"
#include "Engine/Platform/FileSystem.h"

// Small utility that uses temporary file to properly handle non-ANSI paths for 3rd party libs.
struct AnsiPathTempFile
{
    StringAnsi Path;
    String TempPath;
    bool Temp;

    AnsiPathTempFile(const String& path)
    {
        if (path.IsANSI() == false)
        {
            // Use temporary file
            FileSystem::GetTempFilePath(TempPath);
            if (TempPath.IsANSI() && !FileSystem::CopyFile(TempPath, path))
            {
                Path = TempPath.ToStringAnsi();
                return;
            }
            TempPath.Clear();
        }
        Path = path.ToStringAnsi();
    }

    ~AnsiPathTempFile()
    {
        // Cleanup temporary file after use
        if (TempPath.HasChars())
            FileSystem::DeleteFile(TempPath);
    }
};
