// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#include "FileReadStream.h"
#include "Engine/Core/Log.h"
#include "Engine/Platform/File.h"

FileReadStream* FileReadStream::Open(const StringView& path)
{
    const auto file = File::Open(path, FileMode::OpenExisting, FileAccess::Read, FileShare::Read);
    if (file == nullptr)
    {
        LOG(Warning, "Cannot open file '{0}'", path);
        return nullptr;
    }
    return New<FileReadStream>(file);
}

void FileReadStream::Unlink()
{
    _file = nullptr;
}

FileReadStream::FileReadStream(File* file)
    : _file(file)
    , _virtualPosInBuffer(0)
    , _bufferSize(0)
{
    ASSERT_LOW_LAYER(_file);
}

FileReadStream::~FileReadStream()
{
    // Ensure to be file closed and deleted
    if (_file)
        Delete(_file);
}

void FileReadStream::Flush()
{
    // Check if need to flush
    /*if (_virtualPosInBuffer > 0)
    {
        // Update buffer
        uint32 bytesRead;
        _hasError |= _file->Read(_buffer, FILESTREAM_BUFFER_SIZE, &bytesRead) != 0;
        _virtualPosInBuffer = 0;
    }*/
}

void FileReadStream::Close()
{
    if (_file)
    {
        _file->Close();
    }
}

uint32 FileReadStream::GetLength()
{
    return _file->GetSize();
}

uint32 FileReadStream::GetPosition()
{
    return _file->GetPosition() - _bufferSize + _virtualPosInBuffer;
}

void FileReadStream::SetPosition(uint32 seek)
{
    // Seek
    _file->SetPosition(seek);

    // Update buffer
    _hasError |= _file->Read(_buffer, FILESTREAM_BUFFER_SIZE, &_bufferSize) != 0;
    _virtualPosInBuffer = 0;
}

void FileReadStream::ReadBytes(void* data, uint32 bytes)
{
    // Skip if nothing to read
    if (bytes == 0)
    {
        return;
    }

    // Ensure to flush the buffer if it's empty
    if (_bufferSize == 0)
    {
        CHECK(_virtualPosInBuffer == 0);
        _hasError |= _file->Read(_buffer, FILESTREAM_BUFFER_SIZE, &_bufferSize) != 0;
    }

    // Check if buffer has enough data for this read
    const uint32 bufferBytesLeft = static_cast<uint32>(_bufferSize - _virtualPosInBuffer);
    if (bytes <= bufferBytesLeft)
    {
        Platform::MemoryCopy(data, _buffer + _virtualPosInBuffer, bytes);
        _virtualPosInBuffer += bytes;
    }
    else
    {
        // Flush already buffered bytes and read more it the buffer (reduce amount of flushes)
        if (_virtualPosInBuffer > 0)
        {
            Platform::MemoryCopy(data, _buffer + _virtualPosInBuffer, bufferBytesLeft);
            data = (byte*)data + bufferBytesLeft;
            bytes -= bufferBytesLeft;
            _virtualPosInBuffer = 0;
            _hasError |= _file->Read(_buffer, FILESTREAM_BUFFER_SIZE, &_bufferSize) != 0;
        }

        // Read as much as can using whole buffer
        while (bytes >= FILESTREAM_BUFFER_SIZE)
        {
            Platform::MemoryCopy(data, _buffer, FILESTREAM_BUFFER_SIZE);
            data = (byte*)data + FILESTREAM_BUFFER_SIZE;
            bytes -= FILESTREAM_BUFFER_SIZE;
            _hasError |= _file->Read(_buffer, FILESTREAM_BUFFER_SIZE, &_bufferSize) != 0;
        }

        // Read the rest of the buffer but without flushing its data
        if (bytes > 0)
        {
            Platform::MemoryCopy(data, _buffer, bytes);
            _virtualPosInBuffer = bytes;
        }
    }
}
