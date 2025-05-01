// Copyright (c) Wojciech Figat. All rights reserved.

#include "FileWriteStream.h"
#include "Engine/Core/Log.h"
#include "Engine/Platform/File.h"

FileWriteStream* FileWriteStream::Open(const StringView& path)
{
    auto file = File::Open(path, FileMode::CreateAlways, FileAccess::Write, FileShare::Read);
    if (file == nullptr)
    {
        LOG(Warning, "Cannot open file '{0}'", path);
        return nullptr;
    }
    return New<FileWriteStream>(file);
}

void FileWriteStream::Unlink()
{
    _file = nullptr;
}

FileWriteStream::FileWriteStream(File* file)
    : _file(file)
    , _virtualPosInBuffer(0)
{
    ASSERT_LOW_LAYER(_file);
}

FileWriteStream::~FileWriteStream()
{
    // Ensure to be file flushed, closed and deleted
    if (_file)
    {
        Flush();
        Delete(_file);
    }
}

void FileWriteStream::Flush()
{
    // Check if need to flush
    if (_virtualPosInBuffer > 0)
    {
        // Update buffer
        uint32 bytesWritten;
        _hasError |= _file->Write(_buffer, _virtualPosInBuffer, &bytesWritten) != 0;
        _virtualPosInBuffer = 0;
    }
}

void FileWriteStream::Close()
{
    if (_file)
    {
        Flush();
        _file->Close();
    }
}

uint32 FileWriteStream::GetLength()
{
    Flush();
    return _file->GetSize();
}

uint32 FileWriteStream::GetPosition()
{
    Flush();
    return _file->GetPosition();
}

void FileWriteStream::SetPosition(uint32 seek)
{
    Flush();
    _file->SetPosition(seek);
}

void FileWriteStream::WriteBytes(const void* data, uint32 bytes)
{
    const uint32 bufferBytesLeft = FILESTREAM_BUFFER_SIZE - _virtualPosInBuffer;
    if (bytes <= bufferBytesLeft)
    {
        Platform::MemoryCopy(_buffer + _virtualPosInBuffer, data, bytes);
        _virtualPosInBuffer += bytes;
    }
    else
    {
        uint32 bytesWritten;

        // Flush already written bytes and write more it the buffer (reduce amount of flushes)
        if (_virtualPosInBuffer > 0)
        {
            Platform::MemoryCopy(_buffer + _virtualPosInBuffer, data, bufferBytesLeft);
            data = (byte*)data + bufferBytesLeft;
            bytes -= bufferBytesLeft;
            _virtualPosInBuffer = 0;
            _hasError |= _file->Write(_buffer, FILESTREAM_BUFFER_SIZE, &bytesWritten) != 0;
        }

        // Write as much as can using whole buffer
        while (bytes >= FILESTREAM_BUFFER_SIZE)
        {
            Platform::MemoryCopy(_buffer, data, FILESTREAM_BUFFER_SIZE);
            data = (byte*)data + FILESTREAM_BUFFER_SIZE;
            bytes -= FILESTREAM_BUFFER_SIZE;
            _hasError |= _file->Write(_buffer, FILESTREAM_BUFFER_SIZE, &bytesWritten) != 0;
        }

        // Write the rest of the buffer but without flushing its data
        if (bytes > 0)
        {
            Platform::MemoryCopy(_buffer, data, bytes);
            _virtualPosInBuffer = bytes;
        }
    }
}
