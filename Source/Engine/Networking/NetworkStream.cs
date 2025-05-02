// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Text;

namespace FlaxEngine.Networking
{
    unsafe partial class NetworkStream
    {
        /// <summary>
        /// Writes raw bytes into the message.
        /// </summary>
        /// <param name="bytes">The bytes that will be written.</param>
        /// <param name="length">The amount of bytes to write from the bytes pointer.</param>
        public void WriteBytes(byte* bytes, int length)
        {
            WriteData(new IntPtr(bytes), length);
        }

        /// <summary>
        /// Reads raw bytes from the message into the given byte array.
        /// </summary>
        /// <param name="buffer">The buffer pointer that will be used to store the bytes. Should be of the same length as length or longer.
        /// </param>
        /// <param name="length">The minimal amount of bytes that the buffer contains.</param>
        public void ReadBytes(byte* buffer, int length)
        {
            ReadData(new IntPtr(buffer), length);
        }

        /// <summary>
        /// Writes raw bytes into the message.
        /// </summary>
        /// <param name="bytes">The bytes that will be written.</param>
        /// <param name="length">The amount of bytes to write from the bytes array.</param>
        public void WriteBytes(byte[] bytes, int length)
        {
            fixed (byte* bytesPtr = bytes)
                WriteData(new IntPtr(bytesPtr), length);
        }

        /// <summary>
        /// Reads raw bytes from the message into the given byte array.
        /// </summary>
        /// <param name="buffer">The buffer that will be used to store the bytes. Should be of the same length as length or longer.</param>
        /// <param name="length">The minimal amount of bytes that the buffer contains.</param>
        public void ReadBytes(byte[] buffer, int length)
        {
            fixed (byte* bufferPtr = buffer)
                ReadData(new IntPtr(bufferPtr), length);
        }

        /// <summary>
        /// Writes data of type <see cref="Int64"/> into the message.
        /// </summary>
        public void WriteInt64(long value)
        {
            WriteBytes((byte*)&value, sizeof(long));
        }

        /// <summary>
        /// Reads and returns data of type <see cref="Int64"/> from the message.
        /// </summary>
        public long ReadInt64()
        {
            long value = 0;
            ReadBytes((byte*)&value, sizeof(long));
            return value;
        }

        /// <summary>
        /// Writes data of type <see cref="Int32"/> into the message.
        /// </summary>
        public void WriteInt32(int value)
        {
            WriteBytes((byte*)&value, sizeof(int));
        }

        /// <summary>
        /// Reads and returns data of type <see cref="Int32"/> from the message.
        /// </summary>
        public int ReadInt32()
        {
            int value = 0;
            ReadBytes((byte*)&value, sizeof(int));
            return value;
        }

        /// <summary>
        /// Writes data of type <see cref="Int16"/> into the message.
        /// </summary>
        public void WriteInt16(short value)
        {
            WriteBytes((byte*)&value, sizeof(short));
        }

        /// <summary>
        /// Reads and returns data of type <see cref="Int16"/> from the message.
        /// </summary>
        public short ReadInt16()
        {
            short value = 0;
            ReadBytes((byte*)&value, sizeof(short));
            return value;
        }

        /// <summary>
        /// Writes data of type <see cref="SByte"/> into the message.
        /// </summary>
        public void WriteSByte(sbyte value)
        {
            WriteBytes((byte*)&value, sizeof(sbyte));
        }

        /// <summary>
        /// Reads and returns data of type <see cref="SByte"/> from the message.
        /// </summary>
        public sbyte ReadSByte()
        {
            sbyte value = 0;
            ReadBytes((byte*)&value, sizeof(sbyte));
            return value;
        }

        /// <summary>
        /// Writes data of type <see cref="UInt64"/> into the message.
        /// </summary>
        public void WriteUInt64(ulong value)
        {
            WriteBytes((byte*)&value, sizeof(ulong));
        }

        /// <summary>
        /// Reads and returns data of type <see cref="UInt64"/> from the message.
        /// </summary>
        public ulong ReadUInt64()
        {
            ulong value = 0;
            ReadBytes((byte*)&value, sizeof(ulong));
            return value;
        }

        /// <summary>
        /// Writes data of type <see cref="UInt32"/> into the message.
        /// </summary>
        public void WriteUInt32(uint value)
        {
            WriteBytes((byte*)&value, sizeof(uint));
        }

        /// <summary>
        /// Reads and returns data of type <see cref="UInt32"/> from the message.
        /// </summary>
        public uint ReadUInt32()
        {
            uint value = 0;
            ReadBytes((byte*)&value, sizeof(uint));
            return value;
        }

        /// <summary>
        /// Writes data of type <see cref="UInt16"/> into the message.
        /// </summary>
        public void WriteUInt16(ushort value)
        {
            WriteBytes((byte*)&value, sizeof(ushort));
        }

        /// <summary>
        /// Reads and returns data of type <see cref="UInt16"/> from the message.
        /// </summary>
        public ushort ReadUInt16()
        {
            ushort value = 0;
            ReadBytes((byte*)&value, sizeof(ushort));
            return value;
        }

        /// <summary>
        /// Writes data of type <see cref="Byte"/> into the message.
        /// </summary>
        public void WriteByte(byte value)
        {
            WriteBytes(&value, sizeof(byte));
        }

        /// <summary>
        /// Reads and returns data of type <see cref="Byte"/> from the message.
        /// </summary>
        public byte ReadByte()
        {
            byte value = 0;
            ReadBytes(&value, sizeof(byte));
            return value;
        }

        /// <summary>
        /// Writes data of type <see cref="Single"/> into the message.
        /// </summary>
        public void WriteSingle(float value)
        {
            WriteBytes((byte*)&value, sizeof(float));
        }

        /// <summary>
        /// Reads and returns data of type <see cref="Single"/> from the message.
        /// </summary>
        public float ReadSingle()
        {
            float value = 0.0f;
            ReadBytes((byte*)&value, sizeof(float));
            return value;
        }

        /// <summary>
        /// Writes data of type <see cref="Double"/> into the message.
        /// </summary>
        public void WriteDouble(double value)
        {
            WriteBytes((byte*)&value, sizeof(double));
        }

        /// <summary>
        /// Reads and returns data of type <see cref="Double"/> from the message.
        /// </summary>
        public double ReadDouble()
        {
            double value = 0.0;
            ReadBytes((byte*)&value, sizeof(double));
            return value;
        }

        /// <summary>
        /// Writes data of type <see cref="string"/> into the message. UTF-16 encoded.
        /// </summary>
        public void WriteString(string value)
        {
            // Note: Make sure that this is consistent with the C++ message API!
            if (value == null)
            {
                WriteInt32(0);
                return;
            }
            var stringLength = value.Length;
            WriteInt32(stringLength);
            fixed (char* ptr = value)
                WriteData(new IntPtr(ptr), value.Length * 2);
        }

        /// <summary>
        /// Reads and returns data of type <see cref="string"/> from the message. UTF-16 encoded.
        /// </summary>
        public string ReadString()
        {
            // Note: Make sure that this is consistent with the C++ message API!
            var stringLength = ReadInt32();
            if (stringLength == 0)
                return string.Empty;
            var dataLength = stringLength * sizeof(char);
            var bytes = stackalloc char[stringLength];
            ReadBytes((byte*)bytes, dataLength);
            return new string(bytes, 0, stringLength);
        }

        /// <summary>
        /// Writes data of type <see cref="Guid"/> into the message.
        /// </summary>
        public void WriteGuid(Guid value)
        {
            WriteBytes((byte*)&value, sizeof(Guid));
        }

        /// <summary>
        /// Reads and returns data of type <see cref="Guid"/> from the message.
        /// </summary>
        public Guid ReadGuid()
        {
            var guidData = stackalloc Guid[1];
            ReadBytes((byte*)guidData, sizeof(Guid));
            return guidData[0];
        }

        /// <summary>
        /// Writes data of type <see cref="Vector2"/> into the message.
        /// </summary>
        public void WriteVector2(Vector2 value)
        {
            WriteSingle((float)value.X);
            WriteSingle((float)value.Y);
        }

        /// <summary>
        /// Reads and returns data of type <see cref="Vector2"/> from the message.
        /// </summary>
        public Vector2 ReadVector2()
        {
            return new Vector2(ReadSingle(), ReadSingle());
        }

        /// <summary>
        /// Writes data of type <see cref="Vector3"/> into the message.
        /// </summary>
        public void WriteVector3(Vector3 value)
        {
            WriteSingle((float)value.X);
            WriteSingle((float)value.Y);
            WriteSingle((float)value.Z);
        }

        /// <summary>
        /// Reads and returns data of type <see cref="Vector3"/> from the message.
        /// </summary>
        public Vector3 ReadVector3()
        {
            return new Vector3(ReadSingle(), ReadSingle(), ReadSingle());
        }

        /// <summary>
        /// Writes data of type <see cref="Vector4"/> into the message.
        /// </summary>
        public void WriteVector4(Vector4 value)
        {
            WriteSingle((float)value.X);
            WriteSingle((float)value.Y);
            WriteSingle((float)value.Z);
            WriteSingle((float)value.W);
        }

        /// <summary>
        /// Reads and returns data of type <see cref="Vector4"/> from the message.
        /// </summary>
        public Vector4 ReadVector4()
        {
            return new Vector4(ReadSingle(), ReadSingle(), ReadSingle(), ReadSingle());
        }

        /// <summary>
        /// Writes data of type <see cref="Float2"/> into the message.
        /// </summary>
        public void WriteFloat2(Float2 value)
        {
            WriteSingle(value.X);
            WriteSingle(value.Y);
        }

        /// <summary>
        /// Reads and returns data of type <see cref="Float2"/> from the message.
        /// </summary>
        public Float2 ReadFloat2()
        {
            return new Float2(ReadSingle(), ReadSingle());
        }

        /// <summary>
        /// Writes data of type <see cref="Float3"/> into the message.
        /// </summary>
        public void WriteFloat3(Float3 value)
        {
            WriteSingle(value.X);
            WriteSingle(value.Y);
            WriteSingle(value.Z);
        }

        /// <summary>
        /// Reads and returns data of type <see cref="Float3"/> from the message.
        /// </summary>
        public Float3 ReadFloat3()
        {
            return new Float3(ReadSingle(), ReadSingle(), ReadSingle());
        }

        /// <summary>
        /// Writes data of type <see cref="Float4"/> into the message.
        /// </summary>
        public void WriteFloat4(Float4 value)
        {
            WriteSingle(value.X);
            WriteSingle(value.Y);
            WriteSingle(value.Z);
            WriteSingle(value.W);
        }

        /// <summary>
        /// Reads and returns data of type <see cref="Float4"/> from the message.
        /// </summary>
        public Float4 ReadFloat4()
        {
            return new Float4(ReadSingle(), ReadSingle(), ReadSingle(), ReadSingle());
        }

        /// <summary>
        /// Writes data of type <see cref="Quaternion"/> into the message.
        /// </summary>
        public void WriteQuaternion(Quaternion value)
        {
            WriteSingle(value.X);
            WriteSingle(value.Y);
            WriteSingle(value.Z);
            WriteSingle(value.W);
        }

        /// <summary>
        /// Reads and returns data of type <see cref="Quaternion"/> from the message.
        /// </summary>
        public Quaternion ReadQuaternion()
        {
            return new Quaternion(ReadSingle(), ReadSingle(), ReadSingle(), ReadSingle());
        }

        /// <summary>
        /// Writes data of type <see cref="Ray"/> into the message.
        /// </summary>
        public void WriteRay(Ray value)
        {
            WriteVector3(value.Position);
            WriteVector3(value.Direction);
        }

        /// <summary>
        /// Reads and returns data of type <see cref="Ray"/> from the message.
        /// </summary>
        public Ray ReadRay()
        {
            return new Ray(ReadVector3(), ReadVector3());
        }

        /// <summary>
        /// Writes data of type <see cref="Boolean"/> into the message.
        /// </summary>
        public void WriteBoolean(bool value)
        {
            WriteBytes((byte*)&value, sizeof(bool));
        }

        /// <summary>
        /// Reads and returns data of type <see cref="Boolean"/> from the message.
        /// </summary>
        public bool ReadBoolean()
        {
            bool value = default;
            ReadBytes((byte*)&value, sizeof(bool));
            return value;
        }

        /// <summary>
        /// Writes the <see cref="INetworkSerializable"/> object data to the stream. Object has to be allocated.
        /// </summary>
        /// <param name="obj">The serializable object.</param>
        public void Write(INetworkSerializable obj)
        {
            obj.Serialize(this);
        }

        /// <summary>
        /// Reads the <see cref="INetworkSerializable"/> object data from the stream. Object has to be allocated.
        /// </summary>
        /// <param name="obj">The serializable object.</param>
        public void Read(INetworkSerializable obj)
        {
            obj.Deserialize(this);
        }
    }
}
