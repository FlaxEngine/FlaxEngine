// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.Serialization.Formatters.Binary;
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
        /// The attribute meta type identifier.
        /// </summary>
        public const int AttributeMetaTypeID = 12;

        /// <summary>
        /// Gets the attributes collection from the data.
        /// </summary>
        /// <param name="data">The graph metadata.</param>
        /// <returns>The attributes collection.</returns>
        public static Attribute[] GetAttributes(byte[] data)
        {
            if (data != null && data.Length != 0)
            {
                using (var stream = new MemoryStream(data))
                {
                    try
                    {
                        var formatter = new BinaryFormatter();
                        return (Attribute[])formatter.Deserialize(stream);
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
            return GetAttributes(data);
        }

        /// <summary>
        /// Gets the attributes collection.
        /// </summary>
        /// <returns>The attributes collection.</returns>
        public Attribute[] GetAttributes()
        {
            for (int i = 0; i < Entries.Count; i++)
            {
                if (Entries[i].TypeID == AttributeMetaTypeID)
                    return GetAttributes(Entries[i].Data);
            }
            return Utils.GetEmptyArray<Attribute>();
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
            }
            else
            {
                for (int i = 0; i < attributes.Length; i++)
                {
                    if (attributes[i] == null)
                        throw new NullReferenceException("One of the Visject attributes is null.");
                }
                using (var stream = new MemoryStream())
                {
                    var formatter = new BinaryFormatter();
                    formatter.Serialize(stream, attributes);
                    AddEntry(AttributeMetaTypeID, stream.ToArray());
                }
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
            stream.Write(Entries.Count);

            for (int i = 0; i < Entries.Count; i++)
            {
                Entry e = Entries[i];

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
            for (int i = 0; i < Entries.Count; i++)
            {
                if (Entries[i].TypeID == typeID)
                {
                    return Entries[i];
                }
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
