// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_WIN32

#include "Engine/Platform/Base/FileBase.h"

/// <summary>
/// Win32 platform file object implementation
/// </summary>
class FLAXENGINE_API Win32File : public FileBase
{
private:

    void* _handle;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="Win32File"/> class.
    /// </summary>
    /// <param name="handle">The handle.</param>
    Win32File(void* handle)
        : _handle(handle)
    {
    }

    /// <summary>
    /// Finalizes an instance of the <see cref="Win32File"/> class.
    /// </summary>
    ~Win32File();

public:

    /// <summary>
    /// Creates or opens a file.
    /// </summary>
    /// <param name="path">The name of the file to be created or opened.</param>
    /// <param name="mode">An action to take on a file that exists or does not exist.</param>
    /// <param name="access">The requested access to the file.</param>
    /// <param name="share">The requested sharing mode of the file.</param>
    /// <returns>Opened file handle or null if cannot.</returns>
    static Win32File* Open(const StringView& path, FileMode mode, FileAccess access = FileAccess::ReadWrite, FileShare share = FileShare::None);

public:

    // [FileBase]
    bool Read(void* buffer, uint32 bytesToRead, uint32* bytesRead = nullptr) override;
    bool Write(const void* buffer, uint32 bytesToWrite, uint32* bytesWritten = nullptr) override;
    void Close() final override;
    uint32 GetSize() const override;
    DateTime GetLastWriteTime() const override;
    uint32 GetPosition() const override;
    void SetPosition(uint32 seek) override;
    bool IsOpened() const override;
};

#endif
