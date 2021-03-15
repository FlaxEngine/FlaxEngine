// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine.Networking
{
    public unsafe partial struct NetworkMessage
    {
        public void WriteBytes(byte* bytes, int length)
        {
            Utils.MemoryCopy(new IntPtr(bytes), new IntPtr(Buffer), length);
        }

        public void ReadBytes(byte* buffer, int length)
        {
            Utils.MemoryCopy(new IntPtr(Buffer), new IntPtr(buffer), length);
        }

        public void WriteUInt32(uint value)
        {
            WriteBytes((byte*)&value, sizeof(uint));
        }

        public uint ReadUInt32()
        {
            uint value = 0;
            ReadBytes((byte*)&value, sizeof(uint));
            return value;
        }
    }
}
