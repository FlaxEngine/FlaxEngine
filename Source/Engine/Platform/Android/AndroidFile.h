// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_ANDROID

#include "Engine/Platform/Unix/UnixFile.h"

typedef struct AAsset AAsset;

/// <summary>
/// Android platform file object implementation.
/// </summary>
class FLAXENGINE_API AndroidFile : public UnixFile
{
public:

    /// <summary>
    /// Initializes a new instance of the <see cref="AndroidFile"/> class.
    /// </summary>
    /// <param name="handle">The handle.</param>
    AndroidFile(int32 handle);

    /// <summary>
    /// Finalizes an instance of the <see cref="AndroidFile"/> class.
    /// </summary>
    ~AndroidFile();

public:

    /// <summary>
    /// Creates or opens a file.
    /// </summary>
    /// <param name="path">The name of the file to be created or opened.</param>
    /// <param name="mode">An action to take on a file that exists or does not exist.</param>
    /// <param name="access">The requested access to the file.</param>
    /// <param name="share">The requested sharing mode of the file.</param>
    /// <returns>Opened file handle or null if cannot.</returns>
    static AndroidFile* Open(const StringView& path, FileMode mode, FileAccess access = FileAccess::ReadWrite, FileShare share = FileShare::None);
};

/// <summary>
/// Android platform asset file object implementation (accessed via AAssetManager API).
/// </summary>
class AndroidAssetFile : public AndroidFile
{
private:

    AAsset* _asset;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="AndroidAssetFile"/> class.
    /// </summary>
    /// <param name="asset">The Android asset.</param>
    AndroidAssetFile(AAsset* asset);

    /// <summary>
    /// Finalizes an instance of the <see cref="AndroidAssetFile"/> class.
    /// </summary>
    ~AndroidAssetFile();

public:

    // [AndroidFile]
    bool Read(void* buffer, uint32 bytesToRead, uint32* bytesRead = nullptr) override;
    bool Write(const void* buffer, uint32 bytesToWrite, uint32* bytesWritten = nullptr) override;
    void Close() override;
    uint32 GetSize() const override;
    DateTime GetLastWriteTime() const override;
    uint32 GetPosition() const override;
    void SetPosition(uint32 seek) override;
    bool IsOpened() const override;
};

#endif
