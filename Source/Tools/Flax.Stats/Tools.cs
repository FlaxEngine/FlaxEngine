// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

using System;
using System.IO;
using System.Text;

namespace Flax.Stats
{
    /// <summary>
    /// Contains some useful functions
    /// </summary>
    public static class Tools
    {
        /// <summary>
        /// Converts size of the file (in bytes) to the best fitting string
        /// </summary>
        /// <param name="bytes">Size of the file in bytes</param>
        /// <returns>The best fitting string of the file size</returns>
        public static string BytesToText(long bytes)
        {
            string[] s = { "B", "KB", "MB", "GB", "TB" };
            int i = 0;
            float dblSByte = bytes;
            for (; (int)(bytes / 1024.0f) > 0; i++, bytes /= 1024)
                dblSByte = bytes / 1024.0f;
            return string.Format("{0:0.00} {1}", dblSByte, s[i]);
        }

        /// <summary>
        /// Returns date and time as 'good' for filenames string
        /// </summary>
        /// <returns>Date and Time</returns>
        public static string ToFilenameString(this DateTime dt)
        {
            StringBuilder sb = new StringBuilder();
            sb.Append(dt.ToShortDateString().Replace(' ', '_').Replace('-', '_').Replace('/', '_').Replace('\\', '_'));
            sb.Append('_');
            sb.Append(dt.ToLongTimeString().Replace(':', '_').Replace(' ', '_'));
            return sb.ToString();
        }

        #region Direct Write

        /// <summary>
        /// Write new line to the stream
        /// </summary>
        /// <param name="fs">Stream</param>
        public static void WriteLine(this Stream fs)
        {
            fs.WriteByte((byte)'\n');
        }

        /// <summary>
        /// Write text to the stream without '\0' char
        /// </summary>
        /// <param name="fs">File stream</param>
        /// <param name="data">Data to write</param>
        public static void WriteText(this Stream fs, string data)
        {
            // Write all bytes
            for (int i = 0; i < data.Length; i++)
            {
                // Write single byte
                fs.WriteByte((byte)data[i]);
            }
        }

        /// <summary>
        /// Write char to the stream
        /// </summary>
        /// <param name="fs">File stream</param>
        /// <param name="data">Data to write</param>
        public static void Write(this Stream fs, char data)
        {
            // Write single byte
            fs.WriteByte((byte)data);
        }

        /// <summary>
        /// Write string in UTF-8 encoding to the stream
        /// </summary>
        /// <param name="fs">File stream</param>
        /// <param name="data">Data to write</param>
        public static void WriteUTF8(this Stream fs, string data)
        {
            // Check if string is null or empty
            if (string.IsNullOrEmpty(data))
            {
                // Write 0
                fs.Write(0);
            }
            else
            {
                // Get bytes
                byte[] bytes = Encoding.UTF8.GetBytes(data);

                // Write length
                fs.Write(bytes.Length);

                // Write all bytes
                fs.Write(bytes, 0, bytes.Length);
            }
        }

        /// <summary>
        /// Write string in UTF-8 encoding to the stream and offset data
        /// </summary>
        /// <param name="fs">File stream</param>
        /// <param name="data">Data to write</param>
        /// <param name="offset">Offset to apply when writing data</param>
        public static void WriteUTF8(this Stream fs, string data, byte offset)
        {
            // Check if string is null or empty
            if (string.IsNullOrEmpty(data))
            {
                // Write 0
                fs.Write(0);
            }
            else
            {
                // Get bytes
                byte[] bytes = Encoding.UTF8.GetBytes(data);

                // Write length
                int len = bytes.Length;
                fs.Write(len);

                // Offset data
                for (int i = 0; i < len; i++)
                {
                    bytes[i] += offset;
                }

                // Write all bytes
                fs.Write(bytes, 0, len);
            }
        }

        /// <summary>
        /// Write bool to the stream
        /// </summary>
        /// <param name="fs">File stream</param>
        /// <param name="value">Value to write</param>
        public static void Write(this Stream fs, bool value)
        {
            // Write single byte
            fs.WriteByte((value) ? (byte)1 : (byte)0);
        }

        /// <summary>
        /// Write short to the stream
        /// </summary>
        /// <param name="fs">File stream</param>
        /// <param name="value">Value to write</param>
        public static void Write(this Stream fs, short value)
        {
            // Convert to bytes
            byte[] bytes = BitConverter.GetBytes(value);

            // Write bytes
            fs.Write(bytes, 0, 2);
        }

        /// <summary>
        /// Write ushort to the stream
        /// </summary>
        /// <param name="fs">File stream</param>
        /// <param name="value">Value to write</param>
        public static void Write(this Stream fs, ushort value)
        {
            // Convert to bytes
            byte[] bytes = BitConverter.GetBytes(value);

            // Write bytes
            fs.Write(bytes, 0, 2);
        }

        /// <summary>
        /// Write int to the stream
        /// </summary>
        /// <param name="fs">File stream</param>
        /// <param name="value">Value to write</param>
        public static void Write(this Stream fs, int value)
        {
            // Convert to bytes
            byte[] bytes = BitConverter.GetBytes(value);

            // Write bytes
            fs.Write(bytes, 0, 4);
        }

        /// <summary>
        /// Write uint to the stream
        /// </summary>
        /// <param name="fs">File stream</param>
        /// <param name="value">Value to write</param>
        public static void Write(this Stream fs, uint value)
        {
            // Convert to bytes
            byte[] bytes = BitConverter.GetBytes(value);

            // Write bytes
            fs.Write(bytes, 0, 4);
        }

        /// <summary>
        /// Write long to the stream
        /// </summary>
        /// <param name="fs">File stream</param>
        /// <param name="value">Value to write</param>
        public static void Write(this Stream fs, long value)
        {
            // Convert to bytes
            byte[] bytes = BitConverter.GetBytes(value);

            // Write bytes
            fs.Write(bytes, 0, 8);
        }

        /// <summary>
        /// Write float to the stream
        /// </summary>
        /// <param name="fs">File stream</param>
        /// <param name="value">Value to write</param>
        public static void Write(this Stream fs, float value)
        {
            // Convert to bytes
            byte[] bytes = BitConverter.GetBytes(value);

            // Write bytes
            fs.Write(bytes, 0, 4);
        }

        /// <summary>
        /// Write float array to the stream
        /// </summary>
        /// <param name="fs">File stream</param>
        /// <param name="val">Value to write</param>
        public static void Write(this Stream fs, float[] val)
        {
            // Write all floats
            for (int i = 0; i < val.Length; i++)
            {
                // Convert to bytes
                byte[] bytes = BitConverter.GetBytes(val[i]);

                // Write bytes
                fs.Write(bytes, 0, 4);
            }
        }

        /// <summary>
        /// Write double to the stream
        /// </summary>
        /// <param name="fs">File stream</param>
        /// <param name="val">Value to write</param>
        public static void Write(this Stream fs, double val)
        {
            // Convert to bytes
            byte[] bytes = BitConverter.GetBytes(val);

            // Write bytes
            fs.Write(bytes, 0, 8);
        }

        /// <summary>
        /// Write DateTime to the stream
        /// </summary>
        /// <param name="fs">File stream</param>
        /// <param name="val">Value to write</param>
        public static void Write(this Stream fs, DateTime val)
        {
            // Convert to bytes
            byte[] bytes = BitConverter.GetBytes(val.ToBinary());

            // Write bytes
            fs.Write(bytes, 0, 8);
        }

        /// <summary>
        /// Write Guid to the stream
        /// </summary>
        /// <param name="fs">File stream</param>
        /// <param name="val">Value to write</param>
        public static void Write(this Stream fs, Guid val)
        {
            // Convert to bytes
            byte[] bytes = val.ToByteArray();

            // Write bytes
            fs.Write(bytes, 0, 16);
        }

        /// <summary>
        /// Write array of Guids to the stream
        /// </summary>
        /// <param name="fs">File stream</param>
        /// <param name="val">Value to write</param>
        public static void Write(this Stream fs, Guid[] val)
        {
            // Check size
            if (val == null || val.Length < 1)
            {
                // No Guids
                fs.Write(0);
            }
            else
            {
                // Write length
                fs.Write(val.Length);

                // Write all Guids
                for (int i = 0; i < val.Length; i++)
                {
                    // Convert to bytes
                    byte[] bytes = val[i].ToByteArray();

                    // Write bytes
                    fs.Write(bytes, 0, 16);
                }
            }
        }

        /// <summary>
        /// Write TimeSpan to the stream
        /// </summary>
        /// <param name="fs">File stream</param>
        /// <param name="val">Value to write</param>
        public static void Write(this Stream fs, TimeSpan val)
        {
            // Convert to bytes
            byte[] bytes = BitConverter.GetBytes(val.Ticks);

            // Write bytes
            fs.Write(bytes, 0, 8);
        }

        #endregion

        #region Direct Read

        /// <summary>
        /// Read string from the file
        /// </summary>
        /// <param name="fs">File stream</param>
        /// <param name="length">Length of the text</param>
        /// <returns>String</returns>
        public static string ReadText(this Stream fs, int length)
        {
            // Read all bytes
            string val = string.Empty;
            while (fs.CanRead && length > 0)
            {
                // Read byte
                int b = fs.ReadByte();

                // Validate
                if (b > 0)
                {
                    // Add char to string
                    val += (char)b;
                }
                else
                {
                    break;
                }
                length--;
            }

            // Return value
            return val;
        }

        /// <summary>
        /// Read string(UTF-8) from the file
        /// </summary>
        /// <param name="fs">File stream</param>
        /// <returns>String</returns>
        public static string ReadStringUTF8(this Stream fs)
        {
            // Read length
            int len = fs.ReadInt();

            // Check if no string
            if (len == 0)
            {
                // Empty string
                return string.Empty;
            }
            else
            {
                // Read all bytes
                byte[] bytes = new byte[len];
                fs.Read(bytes, 0, bytes.Length);

                // Convert
                return Encoding.UTF8.GetString(bytes);
            }
        }

        /// <summary>
        /// Read string(UTF-8) from the file and restore offset
        /// </summary>
        /// <param name="fs">File stream</param>
        /// <param name="offset">Offset to restore</param>
        /// <returns>String</returns>
        public static string ReadStringUTF8(this Stream fs, byte offset)
        {
            // Read length
            int len = fs.ReadInt();

            // Check if no string
            if (len <= 0)
            {
                // Empty string
                return string.Empty;
            }
            else
            {
                // Read all bytes
                byte[] bytes = new byte[len];
                fs.Read(bytes, 0, bytes.Length);

                // Restore offset
                for (int i = 0; i < len; i++)
                {
                    bytes[i] -= offset;
                }

                // Convert
                return Encoding.UTF8.GetString(bytes);
            }
        }

        /// <summary>
        /// Read bool from the file
        /// </summary>
        /// <param name="fs">File stream</param>
        /// <returns>bool</returns>
        public static bool ReadBool(this Stream fs)
        {
            // Read byte
            return fs.ReadByte() == 1;
        }

        /// <summary>
        /// Read short from the file
        /// </summary>
        /// <param name="fs">File stream</param>
        /// <returns>short</returns>
        public static short ReadShort(this Stream fs)
        {
            // Read 2 bytes
            byte[] bytes = new byte[2];
            fs.Read(bytes, 0, 2);

            // Return bytes converted
            return BitConverter.ToInt16(bytes, 0);
        }

        /// <summary>
        /// Read ushort from the file
        /// </summary>
        /// <param name="fs">File stream</param>
        /// <returns>ushort</returns>
        public static ushort ReadUShort(this Stream fs)
        {
            // Read 2 bytes
            byte[] bytes = new byte[2];
            fs.Read(bytes, 0, 2);

            // Return bytes converted
            return BitConverter.ToUInt16(bytes, 0);
        }

        /// <summary>
        /// Read int from the file
        /// </summary>
        /// <param name="fs">File stream</param>
        /// <returns>int</returns>
        public static int ReadInt(this Stream fs)
        {
            // Read 4 bytes
            byte[] bytes = new byte[4];
            fs.Read(bytes, 0, 4);

            // Return bytes converted
            return BitConverter.ToInt32(bytes, 0);
        }

        /// <summary>
        /// Read uint from the file
        /// </summary>
        /// <param name="fs">File stream</param>
        /// <returns>uint</returns>
        public static uint ReadUInt(this Stream fs)
        {
            // Read 4 bytes
            byte[] bytes = new byte[4];
            fs.Read(bytes, 0, 4);

            // Return bytes converted
            return BitConverter.ToUInt32(bytes, 0);
        }

        /// <summary>
        /// Read ulong from the file
        /// </summary>
        /// <param name="fs">File stream</param>
        /// <returns>ulong</returns>
        public static ulong ReadULong(this Stream fs)
        {
            // Read 8 bytes
            byte[] bytes = new byte[8];
            fs.Read(bytes, 0, 8);

            // Return bytes converted
            return BitConverter.ToUInt64(bytes, 0);
        }

        /// <summary>
        /// Read long from the file
        /// </summary>
        /// <param name="fs">File stream</param>
        /// <returns>long</returns>
        public static long ReadLong(this Stream fs)
        {
            // Read 8 bytes
            byte[] bytes = new byte[8];
            fs.Read(bytes, 0, 8);

            // Return bytes converted
            return BitConverter.ToInt64(bytes, 0);
        }

        /// <summary>
        /// Read float from the file
        /// </summary>
        /// <param name="fs">File stream</param>
        /// <returns>float</returns>
        public static unsafe float ReadFloat(this Stream fs)
        {
            // Read 4 bytes
            byte[] bytes = new byte[4];
            fs.Read(bytes, 0, 4);

            // Convert into float
            fixed (byte* p = bytes)
            {
                return *((float*)p);
            }
        }

        /// <summary>
        /// Read array of floats from the file
        /// </summary>
        /// <param name="fs">File stream</param>
        /// <param name="size">Array size</param>
        /// <returns>float array</returns>
        public static unsafe float[] ReadFloats(this Stream fs, uint size)
        {
            // Create arrays
            float[] val = new float[size];
            byte[] bytes = new byte[4];

            // Read all floats
            for (int i = 0; i < size; i++)
            {
                // Read 4 bytes
                fs.Read(bytes, 0, 4);

                // Convert into float
                fixed (byte* p = bytes)
                {
                    val[i] = *((float*)p);
                }
            }

            // Return
            return val;
        }

        /// <summary>
        /// Read double from the file
        /// </summary>
        /// <param name="fs">File stream</param>
        /// <returns>double</returns>
        public static double ReadDouble(this Stream fs)
        {
            // Read 8 bytes
            byte[] bytes = new byte[8];
            fs.Read(bytes, 0, 8);

            // Return bytes converted
            return BitConverter.ToDouble(bytes, 0);
        }

        /// <summary>
        /// Read DateTime from the file
        /// </summary>
        /// <param name="fs">File stream</param>
        /// <returns>DateTime</returns>
        public static DateTime ReadDateTime(this Stream fs)
        {
            // Read 8 bytes
            byte[] bytes = new byte[8];
            fs.Read(bytes, 0, 8);

            // Return bytes converted
            return DateTime.FromBinary(BitConverter.ToInt64(bytes, 0));
        }

        /// <summary>
        /// Read Guid from the file
        /// </summary>
        /// <param name="fs">File stream</param>
        /// <returns>Guid</returns>
        public static Guid ReadGuid(this Stream fs)
        {
            // Read 16 bytes
            byte[] bytes = new byte[16];
            fs.Read(bytes, 0, 16);

            // Return created Guid
            return new Guid(bytes);
        }

        /// <summary>
        /// Read array of Guids from the file
        /// </summary>
        /// <param name="fs">File stream</param>
        /// <returns>Guids</returns>
        public static Guid[] ReadGuids(this Stream fs)
        {
            // Read size
            int len = fs.ReadInt();

            // Check if can read more
            Guid[] res;
            if (len > 0)
            {
                // Create array
                res = new Guid[len];

                // Read all
                for (int i = 0; i < len; i++)
                {
                    // Read 16 bytes
                    byte[] bytes = new byte[16];
                    fs.Read(bytes, 0, 16);

                    // Create Guid
                    res[i] = new Guid(bytes);
                }
            }
            else
            {
                // No Guids
                res = new Guid[0];
            }

            // Return
            return res;
        }

        /// <summary>
        /// Read TimeSpan from the file
        /// </summary>
        /// <param name="fs">File stream</param>
        /// <returns>TimeSpan</returns>
        public static TimeSpan ReadTimeSpan(this Stream fs)
        {
            // Read 8 bytes
            byte[] bytes = new byte[8];
            fs.Read(bytes, 0, 8);

            // Return bytes converted
            return new TimeSpan(BitConverter.ToInt64(bytes, 0));
        }

        #endregion
    }
}
