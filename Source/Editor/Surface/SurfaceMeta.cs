// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.Loader;
using System.Runtime.Serialization.Formatters.Binary;
using System.Text;
using FlaxEngine;

namespace FlaxEditor.Surface
{
    /// <summary>
    /// Metadata container.
    /// </summary>
    [HideInEditor]
    public class SurfaceMeta
    {
        /// <summary>
        /// Metadata entry
        /// </summary>
        public struct Entry
        {
            /// <summary>
            /// The type identifier.
            /// </summary>
            public int TypeID;

            /// <summary>
            /// The data.
            /// </summary>
            public byte[] Data;
        }

        /// <summary>
        /// All meta entries
        /// </summary>
        public readonly List<Entry> Entries = new List<Entry>();

        /// <summary>
        /// The attribute meta type identifier. Uses byte[] as storage for Attribute[] serialized with BinaryFormatter (deprecated in .NET 5).
        /// [Deprecated on 8.12.2023, expires on 8.12.2025]
        /// </summary>
        public const int OldAttributeMetaTypeID = 12;

        /// <summary>
        /// The attribute meta type identifier. Uses byte[] as storage for Attribute[] serialized with JsonSerializer.
        /// </summary>
        public const int AttributeMetaTypeID = 13;

        /// <summary>
        /// Gets the attributes collection from the data.
        /// </summary>
        /// <param name="data">The graph metadata serialized with JsonSerializer.</param>
        /// <param name="oldData">The graph metadata serialized with BinaryFormatter.</param>
        /// <returns>The attributes collection.</returns>
        public static Attribute[] GetAttributes(byte[] data, byte[] oldData)
        {
            if (data != null && data.Length != 0)
            {
                try
                {
                    var json = Encoding.Unicode.GetString(data);
                    return FlaxEngine.Json.JsonSerializer.Deserialize<Attribute[]>(json);
                }
                catch (Exception ex)
                {
                    Editor.LogError("Failed to deserialize Visject attributes array.");
                    Editor.LogWarning(ex);
                }
            }
            if (oldData != null && oldData.Length != 0)
            {
                // [Deprecated on 8.12.2023, expires on 8.12.2025]
                using (var stream = new MemoryStream(oldData))
                {
                    try
                    {
                        // Ensure we are in the correct load context (https://github.com/dotnet/runtime/issues/42041)
                        using var ctx = AssemblyLoadContext.EnterContextualReflection(typeof(Editor).Assembly);
#pragma warning disable SYSLIB0011
                        var formatter = new BinaryFormatter();
                        return (Attribute[])formatter.Deserialize(stream);
#pragma warning restore SYSLIB0011
                    }
                    catch (Exception ex)
                    {
                        Editor.LogError("Failed to deserialize Visject attributes array.");
                        Editor.LogWarning(ex);
                    }
                }
            }
            return Utils.GetEmptyArray<Attribute>();
        }

        /// <summary>
        /// Serializes surface attributes into byte[] data using the current format.
        /// </summary>
        /// <param name="attributes">The input attributes.</param>
        /// <returns>The result array with bytes. Can be empty but not null.</returns>
        internal static byte[] GetAttributesData(Attribute[] attributes)
        {
            if (attributes != null && attributes.Length != 0)
            {
                var json = FlaxEngine.Json.JsonSerializer.Serialize(attributes);
                return Encoding.Unicode.GetBytes(json);
            }
            return Utils.GetEmptyArray<byte>();
        }

        /// <summary>
        /// Determines whether the specified attribute was defined for this member.
        /// </summary>
        /// <param name="parameter">The graph parameter.</param>
        /// <param name="attributeType">The attribute type.</param>
        /// <returns><c>true</c> if the specified member has attribute; otherwise, <c>false</c>.</returns>
        public static bool HasAttribute(GraphParameter parameter, Type attributeType)
        {
            return GetAttributes(parameter).Any(x => x.GetType() == attributeType);
        }

        /// <summary>
        /// Gets the attributes collection from the graph parameter.
        /// </summary>
        /// <param name="parameter">The graph parameter.</param>
        /// <returns>The attributes collection.</returns>
        public static Attribute[] GetAttributes(GraphParameter parameter)
        {
            var data = parameter.GetMetaData(AttributeMetaTypeID);
            var dataOld = parameter.GetMetaData(OldAttributeMetaTypeID);
            return GetAttributes(data, dataOld);
        }

        /// <summary>
        /// Gets the attributes collection.
        /// </summary>
        /// <returns>The attributes collection.</returns>
        public Attribute[] GetAttributes()
        {
            return GetAttributes(GetEntry(AttributeMetaTypeID).Data, GetEntry(OldAttributeMetaTypeID).Data);
        }

        /// <summary>
        /// Sets the attributes collection.
        /// </summary>
        /// <param name="attributes">The attributes to set.</param>
        public void SetAttributes(Attribute[] attributes)
        {
            if (attributes == null || attributes.Length == 0)
            {
                RemoveEntry(AttributeMetaTypeID);
                RemoveEntry(OldAttributeMetaTypeID);
            }
            else
            {
                AddEntry(AttributeMetaTypeID, GetAttributesData(attributes));
                RemoveEntry(OldAttributeMetaTypeID);
            }
        }

        /// <summary>
        /// Load from the stream
        /// </summary>
        /// <param name="stream">Stream</param>
        public void Load(BinaryReader stream)
        {
            Entries.Clear();
            Entries.TrimExcess();

            // Version 1
            {
                int entries = stream.ReadInt32();

                Entries.Capacity = entries;

                for (int i = 0; i < entries; i++)
                {
                    Entry e = new Entry
                    {
                        TypeID = stream.ReadInt32()
                    };
                    stream.ReadInt64(); // don't use CreationTime

                    uint dataSize = stream.ReadUInt32();
                    e.Data = new byte[dataSize];
                    stream.Read(e.Data, 0, (int)dataSize);

                    Entries.Add(e);
                }
            }
        }

        /// <summary>
        /// Save to the stream
        /// </summary>
        /// <param name="stream">Stream</param>
        /// <returns>True if cannot save data</returns>
        public void Save(BinaryWriter stream)
        {
            var entries = Entries;
            stream.Write(entries.Count);
            for (int i = 0; i < entries.Count; i++)
            {
                Entry e = entries[i];

                stream.Write(e.TypeID);
                stream.Write((long)0);

                uint dataSize = e.Data != null ? (uint)e.Data.Length : 0;
                stream.Write(dataSize);
                if (dataSize > 0)
                {
                    stream.Write(e.Data);
                }
            }
        }

        /// <summary>
        /// Releases meta data.
        /// </summary>
        public void Release()
        {
            Entries.Clear();
            Entries.TrimExcess();
        }

        /// <summary>
        /// Gets the entry.
        /// </summary>
        /// <param name="typeID">Entry type ID</param>
        /// <returns>Entry</returns>
        public Entry GetEntry(int typeID)
        {
            var entries = Entries;
            for (int i = 0; i < entries.Count; i++)
            {
                if (entries[i].TypeID == typeID)
                    return entries[i];
            }
            return new Entry();
        }

        /// <summary>
        /// Adds new entry.
        /// </summary>
        /// <param name="typeID">Type ID</param>
        /// <param name="data">Bytes to set</param>
        public void AddEntry(int typeID, byte[] data)
        {
            var e = new Entry
            {
                TypeID = typeID,
                Data = data
            };
            for (int i = 0; i < Entries.Count; i++)
            {
                if (Entries[i].TypeID == typeID)
                {
                    Entries[i] = e;
                    return;
                }
            }
            Entries.Add(e);
        }

        /// <summary>
        /// Removes the entry.
        /// </summary>
        /// <param name="typeID">Type ID</param>
        public void RemoveEntry(int typeID)
        {
            for (int i = 0; i < Entries.Count; i++)
            {
                if (Entries[i].TypeID == typeID)
                {
                    Entries.RemoveAt(i);
                    break;
                }
            }
        }
    }
}
