// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

namespace FlaxEngine.Networking
{
    public unsafe partial struct NetworkMessage
    {
        public void WriteBytes(byte* bytes, int length)
        {
            // TODO
        }

        public void ReadBytes(byte* buffer, int length)
        {
            // TODO
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
