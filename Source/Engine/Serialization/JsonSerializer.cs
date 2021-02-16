// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Globalization;
using System.IO;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using FlaxEngine.Json.JsonCustomSerializers;
using FlaxEngine.Utilities;
using Newtonsoft.Json;
using Newtonsoft.Json.Converters;
using Newtonsoft.Json.Serialization;

namespace FlaxEngine.Json
{
    /// <summary>
    /// Serialize references to the FlaxEngine.Object as Guid.
    /// </summary>
    /// <seealso cref="Newtonsoft.Json.JsonConverter" />
    internal class FlaxObjectConverter : JsonConverter
    {
        /// <inheritdoc />
        public override unsafe void WriteJson(JsonWriter writer, object value, Newtonsoft.Json.JsonSerializer serializer)
        {
            Guid id = Guid.Empty;
            if (value is Object obj)
                id = obj.ID;
            writer.WriteValue(JsonSerializer.GetStringID(&id));
        }

        /// <inheritdoc />
        public override object ReadJson(JsonReader reader, Type objectType, object existingValue, Newtonsoft.Json.JsonSerializer serializer)
        {
            if (reader.TokenType == JsonToken.String)
            {
                JsonSerializer.ParseID((string)reader.Value, out var id);
                return Object.Find(ref id, objectType);
            }
            return null;
        }

        /// <inheritdoc />
        public override bool CanConvert(Type objectType)
        {
            // Skip serialization as reference id for the root object serialization (eg. Script)
            var cache = JsonSerializer.Current.Value;
            if (cache != null && cache.IsDuringSerialization && cache.SerializerWriter.SerializeStackSize == 0)
            {
                return false;
            }
            return typeof(Object).IsAssignableFrom(objectType);
        }
    }

    /// <summary>
    /// Serialize SceneReference as Guid in internal format.
    /// </summary>
    /// <seealso cref="Newtonsoft.Json.JsonConverter" />
    internal class SceneReferenceConverter : JsonConverter
    {
        /// <inheritdoc />
        public override unsafe void WriteJson(JsonWriter writer, object value, Newtonsoft.Json.JsonSerializer serializer)
        {
            Guid id = ((SceneReference)value).ID;
            writer.WriteValue(JsonSerializer.GetStringID(&id));
        }

        /// <inheritdoc />
        public override object ReadJson(JsonReader reader, Type objectType, object existingValue, Newtonsoft.Json.JsonSerializer serializer)
        {
            SceneReference result = new SceneReference();

            if (reader.TokenType == JsonToken.String)
            {
                JsonSerializer.ParseID((string)reader.Value, out result.ID);
            }

            return result;
        }

        /// <inheritdoc />
        public override bool CanConvert(Type objectType)
        {
            return objectType == typeof(SceneReference);
        }
    }

    /// <summary>
    /// Serialize SoftObjectReference as Guid in internal format.
    /// </summary>
    /// <seealso cref="Newtonsoft.Json.JsonConverter" />
    internal class SoftObjectReferenceConverter : JsonConverter
    {
        /// <inheritdoc />
        public override unsafe void WriteJson(JsonWriter writer, object value, Newtonsoft.Json.JsonSerializer serializer)
        {
            var id = ((SoftObjectReference)value).ID;
            writer.WriteValue(JsonSerializer.GetStringID(&id));
        }

        /// <inheritdoc />
        public override object ReadJson(JsonReader reader, Type objectType, object existingValue, Newtonsoft.Json.JsonSerializer serializer)
        {
            var result = new SoftObjectReference();
            if (reader.TokenType == JsonToken.String)
            {
                JsonSerializer.ParseID((string)reader.Value, out var id);
                result.ID = id;
            }
            return result;
        }

        /// <inheritdoc />
        public override bool CanConvert(Type objectType)
        {
            return objectType == typeof(SoftObjectReference);
        }
    }

    /*
    /// <summary>
    /// Serialize Guid values using `N` format
    /// </summary>
    /// <seealso cref="Newtonsoft.Json.JsonConverter" />
    internal class GuidConverter : JsonConverter
    {
        /// <inheritdoc />
        public override void WriteJson(JsonWriter writer, object value, Newtonsoft.Json.JsonSerializer serializer)
        {
            Guid id = (Guid)value;
            writer.WriteValue(id.ToString("N"));
        }

        /// <inheritdoc />
        public override object ReadJson(JsonReader reader, Type objectType, object existingValue, Newtonsoft.Json.JsonSerializer serializer)
        {
            if (reader.TokenType == JsonToken.String)
            {
                var id = Guid.Parse((string)reader.Value);
                return id;
            }

            return Guid.Empty;
        }

        /// <inheritdoc />
        public override bool CanConvert(Type objectType)
        {
            return objectType == typeof(Guid);
        }
    }
    */

    /// <summary>
    /// Objects serialization tool (json format).
    /// </summary>
    public static class JsonSerializer
    {
        internal class SerializerCache
        {
            public Newtonsoft.Json.JsonSerializer JsonSerializer;
            public StringBuilder StringBuilder;
            public StringWriter StringWriter;
            public JsonTextWriter JsonWriter;
            public JsonSerializerInternalWriter SerializerWriter;
            public UnmanagedStringReader StringReader;
            public bool IsDuringSerialization;

            public SerializerCache(JsonSerializerSettings settings)
            {
                JsonSerializer = Newtonsoft.Json.JsonSerializer.CreateDefault(settings);
                JsonSerializer.Formatting = Formatting.Indented;
                StringBuilder = new StringBuilder(256);
                StringWriter = new StringWriter(StringBuilder, CultureInfo.InvariantCulture);
                SerializerWriter = new JsonSerializerInternalWriter(JsonSerializer);
                StringReader = new UnmanagedStringReader();
                JsonWriter = new JsonTextWriter(StringWriter)
                {
                    IndentChar = '\t',
                    Indentation = 1,
                    Formatting = JsonSerializer.Formatting,
                    DateFormatHandling = JsonSerializer.DateFormatHandling,
                    DateTimeZoneHandling = JsonSerializer.DateTimeZoneHandling,
                    FloatFormatHandling = JsonSerializer.FloatFormatHandling,
                    StringEscapeHandling = JsonSerializer.StringEscapeHandling,
                    Culture = JsonSerializer.Culture,
                    DateFormatString = JsonSerializer.DateFormatString,
                };
            }
        }

        internal static JsonSerializerSettings Settings = CreateDefaultSettings(false);
        internal static JsonSerializerSettings SettingsManagedOnly = CreateDefaultSettings(true);
        internal static FlaxObjectConverter ObjectConverter;
        internal static ThreadLocal<SerializerCache> Current = new ThreadLocal<SerializerCache>();
        internal static ThreadLocal<SerializerCache> Cache = new ThreadLocal<SerializerCache>(() => new SerializerCache(Settings));
        internal static ThreadLocal<SerializerCache> CacheManagedOnly = new ThreadLocal<SerializerCache>(() => new SerializerCache(SettingsManagedOnly));
        internal static ThreadLocal<IntPtr> CachedGuidBuffer = new ThreadLocal<IntPtr>(() => Marshal.AllocHGlobal(32 * sizeof(char)), true);
        internal static string CachedGuidDigits = "0123456789abcdef";

        internal static JsonSerializerSettings CreateDefaultSettings(bool isManagedOnly)
        {
            //Newtonsoft.Json.Utilities.MiscellaneousUtils.ValueEquals = ValueEquals;

            var settings = new JsonSerializerSettings
            {
                ContractResolver = new ExtendedDefaultContractResolver(isManagedOnly),
                ReferenceLoopHandling = ReferenceLoopHandling.Ignore,
                TypeNameHandling = TypeNameHandling.Auto,
                NullValueHandling = NullValueHandling.Ignore,
                ObjectCreationHandling = ObjectCreationHandling.Auto,
            };
            if (ObjectConverter == null)
                ObjectConverter = new FlaxObjectConverter();
            settings.Converters.Add(ObjectConverter);
            settings.Converters.Add(new SceneReferenceConverter());
            settings.Converters.Add(new SoftObjectReferenceConverter());
            settings.Converters.Add(new VersionConverter());
            //settings.Converters.Add(new GuidConverter());
            return settings;
        }

        internal static void Dispose()
        {
            CachedGuidBuffer.Values.ForEach(Marshal.FreeHGlobal);
            Current.Dispose();
            Cache.Dispose();
            CacheManagedOnly.Dispose();
        }

        /// <summary>
        /// The default implementation of the values comparision function used by the serialization system.
        /// </summary>
        /// <param name="objA">The object a.</param>
        /// <param name="objB">The object b.</param>
        /// <returns>True if both objects are equal, otherwise false.</returns>
        public static bool ValueEquals(object objA, object objB)
        {
            // If referenced object has the same linkage to the prefab object as the default value used in SerializeDiff, then mark it as equal
            /*if (objA is ISceneObject sceneObjA && objB is ISceneObject sceneObjB && (sceneObjA.HasPrefabLink || sceneObjB.HasPrefabLink))
            {
                return sceneObjA.PrefabObjectID == sceneObjB.PrefabObjectID;
            }*/

            return Newtonsoft.Json.Utilities.MiscellaneousUtils.DefaultValueEquals(objA, objB);
        }

        /// <summary>
        /// Serializes the specified object.
        /// </summary>
        /// <param name="obj">The object.</param>
        /// <param name="isManagedOnly">True if serialize only C# members and skip C++ members (marked with <see cref="UnmanagedAttribute"/>).</param>
        /// <returns>The output json string.</returns>
        public static string Serialize(object obj, bool isManagedOnly = false)
        {
            Type type = obj.GetType();
            var cache = isManagedOnly ? CacheManagedOnly.Value : Cache.Value;
            Current.Value = cache;

            cache.StringBuilder.Clear();
            cache.IsDuringSerialization = true;
            cache.SerializerWriter.Serialize(cache.JsonWriter, obj, type);

            return cache.StringBuilder.ToString();
        }

        /// <summary>
        /// Serializes the specified object.
        /// </summary>
        /// <param name="obj">The object.</param>
        /// <param name="type">The object type. Can be typeof(object) to handle generic object serialization.</param>
        /// <param name="isManagedOnly">True if serialize only C# members and skip C++ members (marked with <see cref="UnmanagedAttribute"/>).</param>
        /// <returns>The output json string.</returns>
        public static string Serialize(object obj, Type type, bool isManagedOnly = false)
        {
            var cache = isManagedOnly ? CacheManagedOnly.Value : Cache.Value;
            Current.Value = cache;

            cache.StringBuilder.Clear();
            cache.IsDuringSerialization = true;
            cache.SerializerWriter.Serialize(cache.JsonWriter, obj, type);

            return cache.StringBuilder.ToString();
        }

        /// <summary>
        /// Serializes the specified object difference to the other object of the same type. Used to serialize modified properties of the object during prefab instance serialization.
        /// </summary>
        /// <param name="obj">The object.</param>
        /// <param name="other">The reference object.</param>
        /// <param name="isManagedOnly">True if serialize only C# members and skip C++ members (marked with <see cref="UnmanagedAttribute"/>).</param>
        /// <returns>The output json string.</returns>
        public static string SerializeDiff(object obj, object other, bool isManagedOnly = false)
        {
            Type type = obj.GetType();
            var cache = isManagedOnly ? CacheManagedOnly.Value : Cache.Value;
            Current.Value = cache;

            cache.StringBuilder.Clear();
            cache.IsDuringSerialization = true;
            cache.SerializerWriter.SerializeDiff(cache.JsonWriter, obj, type, other);

            return cache.StringBuilder.ToString();
        }

        /// <summary>
        /// Deserializes the specified object (from the input json data).
        /// </summary>
        /// <param name="input">The object.</param>
        /// <param name="json">The input json data.</param>
        public static void Deserialize(object input, string json)
        {
            var cache = Cache.Value;
            cache.IsDuringSerialization = false;
            Current.Value = cache;

            using (JsonReader reader = new JsonTextReader(new StringReader(json)))
            {
                cache.JsonSerializer.Populate(reader, input);

                if (!cache.JsonSerializer.CheckAdditionalContent)
                    return;
                while (reader.Read())
                {
                    if (reader.TokenType != JsonToken.Comment)
                        throw new FlaxException("Additional text found in JSON string after finishing deserializing object.");
                }
            }
        }

        /// <summary>
        /// Deserializes the specified .NET object type (from the input json data).
        /// </summary>
        /// <param name="json">The input json data.</param>
        /// <typeparam name="T">The type of the object to deserialize to.</typeparam>
        /// <returns>The deserialized object from the JSON string.</returns>
        public static T Deserialize<T>(string json)
        {
            return (T)Deserialize(json, typeof(T));
        }

        /// <summary>
        /// Deserializes the specified .NET object type (from the input json data).
        /// </summary>
        /// <param name="json">The input json data.</param>
        /// <param name="objectType">The <see cref="T:System.Type" /> of object being deserialized.</param>
        /// <returns>The deserialized object from the JSON string.</returns>
        public static object Deserialize(string json, Type objectType)
        {
            object result;
            var cache = Cache.Value;
            cache.IsDuringSerialization = false;
            Current.Value = cache;

            using (JsonReader reader = new JsonTextReader(new StringReader(json)))
            {
                result = cache.JsonSerializer.Deserialize(reader, objectType);

                if (!cache.JsonSerializer.CheckAdditionalContent)
                    return result;
                while (reader.Read())
                {
                    if (reader.TokenType != JsonToken.Comment)
                        throw new FlaxException("Additional text found in JSON string after finishing deserializing object.");
                }
            }

            return result;
        }

        /// <summary>
        /// Deserializes the .NET object (from the input json data).
        /// </summary>
        /// <param name="json">The input json data.</param>
        /// <returns>The deserialized object from the JSON string.</returns>
        public static object Deserialize(string json)
        {
            object result;
            var cache = Cache.Value;
            cache.IsDuringSerialization = false;
            Current.Value = cache;

            using (JsonReader reader = new JsonTextReader(new StringReader(json)))
            {
                result = cache.JsonSerializer.Deserialize(reader);

                if (!cache.JsonSerializer.CheckAdditionalContent)
                    return result;
                while (reader.Read())
                {
                    if (reader.TokenType != JsonToken.Comment)
                        throw new FlaxException("Additional text found in JSON string after finishing deserializing object.");
                }
            }

            return result;
        }

        /// <summary>
        /// Deserializes the specified object (from the input json data).
        /// </summary>
        /// <param name="input">The object.</param>
        /// <param name="jsonBuffer">The input json data buffer (raw, fixed memory buffer).</param>
        /// <param name="jsonLength">The input json data buffer length (characters count).</param>
        public static unsafe void Deserialize(object input, void* jsonBuffer, int jsonLength)
        {
            var cache = Cache.Value;
            cache.IsDuringSerialization = false;
            Current.Value = cache;

            cache.StringReader.Initialize(jsonBuffer, jsonLength);
            var jsonReader = new JsonTextReader(cache.StringReader);
            cache.JsonSerializer.Populate(jsonReader, input);

            if (!cache.JsonSerializer.CheckAdditionalContent)
                return;
            while (jsonReader.Read())
            {
                if (jsonReader.TokenType != JsonToken.Comment)
                    throw new FlaxException("Additional text found in JSON string after finishing deserializing object.");
            }
        }

        /// <summary>
        /// Guid type in Flax format (the same as C++ layer).
        /// </summary>
        internal struct GuidInterop
        {
            public uint A;
            public uint B;
            public uint C;
            public uint D;
        }

        /// <summary>
        /// Gets the string representation of the given object ID. It matches the internal serialization formatting rules.
        /// </summary>
        /// <param name="id">The object identifier.</param>
        /// <returns>The serialized ID.</returns>
        public static unsafe string GetStringID(Guid id)
        {
            return GetStringID(&id);
        }

        /// <summary>
        /// Gets the string representation of the given object ID. It matches the internal serialization formatting rules.
        /// </summary>
        /// <param name="id">The object identifier.</param>
        /// <returns>The serialized ID.</returns>
        public static unsafe string GetStringID(Guid* id)
        {
            GuidInterop* g = (GuidInterop*)id;

            // Unoptimized version:
            //return string.Format("{0:x8}{1:x8}{2:x8}{3:x8}", g->A, g->B, g->C, g->D);

            // Optimized version:
            var buffer = (char*)CachedGuidBuffer.Value.ToPointer();
            for (int i = 0; i < 32; i++)
                buffer[i] = '0';
            var digits = CachedGuidDigits;
            uint n = g->A;
            char* p = buffer + 7;
            do
            {
                *p-- = digits[(int)(n & 0xf)];
            } while ((n >>= 4) != 0);
            n = g->B;
            p = buffer + 15;
            do
            {
                *p-- = digits[(int)(n & 0xf)];
            } while ((n >>= 4) != 0);
            n = g->C;
            p = buffer + 23;
            do
            {
                *p-- = digits[(int)(n & 0xf)];
            } while ((n >>= 4) != 0);
            n = g->D;
            p = buffer + 31;
            do
            {
                *p-- = digits[(int)(n & 0xf)];
            } while ((n >>= 4) != 0);
            return new string(buffer, 0, 32);
        }

        /// <summary>
        /// Gets the string representation of the given object. It matches the internal serialization formatting rules.
        /// </summary>
        /// <param name="obj">The object.</param>
        /// <returns>The serialized ID.</returns>
        public static unsafe string GetStringID(Object obj)
        {
            Guid id = Guid.Empty;
            if (obj != null)
                id = obj.ID;
            return GetStringID(&id);
        }

        /// <summary>
        /// Parses the given object identifier represented in the internal serialization format.
        /// </summary>
        /// <param name="str">The ID string.</param>
        /// <param name="id">The identifier.</param>
        public static unsafe void ParseID(string str, out Guid id)
        {
            GuidInterop g;

            // Broken after VS 15.5
            /*fixed (char* a = str)
            {
                char* b = a + 8;
                char* c = b + 8;
                char* d = c + 8;

                ParseHex(a, 8, out g.A);
                ParseHex(b, 8, out g.B);
                ParseHex(c, 8, out g.C);
                ParseHex(d, 8, out g.D);
            }*/

            // Temporary fix (not using raw char* pointer)
            ParseHex(str, 0, 8, out g.A);
            ParseHex(str, 8, 8, out g.B);
            ParseHex(str, 16, 8, out g.C);
            ParseHex(str, 24, 8, out g.D);

            id = *(Guid*)&g;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static unsafe void ParseHex(char* str, int length, out uint result)
        {
            uint sum = 0;
            char* p = str;
            char* end = str + length;

            if (*p == '0' && *(p + 1) == 'x')
                p += 2;

            while (p < end && *p != 0)
            {
                int c = *p - '0';

                if (c < 0 || c > 9)
                {
                    c = char.ToLower(*p) - 'a' + 10;
                    if (c < 10 || c > 15)
                    {
                        result = 0;
                        return;
                    }
                }

                sum = 16 * sum + (uint)c;

                p++;
            }

            result = sum;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static void ParseHex(string str, int start, int length, out uint result)
        {
            uint sum = 0;
            int p = start;
            int end = start + length;

            if (str.Length < end)
            {
                result = 0;
                return;
            }

            if (str[p] == '0' && str[p + 1] == 'x')
                p += 2;

            while (p < end && str[p] != 0)
            {
                int c = str[p] - '0';

                if (c < 0 || c > 9)
                {
                    c = char.ToLower(str[p]) - 'a' + 10;
                    if (c < 10 || c > 15)
                    {
                        result = 0;
                        return;
                    }
                }

                sum = 16 * sum + (uint)c;

                p++;
            }

            result = sum;
        }
    }
}
