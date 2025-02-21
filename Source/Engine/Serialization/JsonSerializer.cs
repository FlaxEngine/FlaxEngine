// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Collections;
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
    sealed class StringWriterWithEncoding : StringWriter
    {
        public override Encoding Encoding { get; }

        public StringWriterWithEncoding(System.Text.StringBuilder sb, IFormatProvider formatProvider, Encoding encoding)
        : base(sb, formatProvider)
        {
            Encoding = encoding;
        }
    }

    partial class JsonSerializer
    {
        internal class SerializerCache
        {
            public bool IsManagedOnly;
            public Newtonsoft.Json.JsonSerializer JsonSerializer;
            public StringBuilder StringBuilder;
            public StringWriter StringWriter;
            public JsonTextWriter JsonWriter;
            public JsonSerializerInternalWriter SerializerWriter;
            public UnmanagedMemoryStream MemoryStream;
            public StreamReader Reader;
            public bool IsWriting;
            public bool IsReading;
#if FLAX_EDITOR
            public uint CacheVersion;
#endif

            public unsafe SerializerCache(bool isManagedOnly)
            {
                IsManagedOnly = isManagedOnly;
                StringBuilder = new StringBuilder(256);
                StringWriter = new StringWriterWithEncoding(StringBuilder, CultureInfo.InvariantCulture, Encoding.UTF8);
                MemoryStream = new UnmanagedMemoryStream((byte*)0, 0);

#if FLAX_EDITOR
                lock (CurrentCacheSyncRoot)
#endif
                {
                    BuildSerializer();
                    BuildRead();
                    BuildWrite();
#if FLAX_EDITOR
                    CacheVersion = Json.JsonSerializer.CurrentCacheVersion;
#endif
                }
            }

            public void ReadBegin()
            {
                CheckCacheVersionRebuild();

                // TODO: Reset reading state (eg if previous deserialization got exception)
                if (IsReading)
                    BuildRead();

                IsWriting = false;
                IsReading = true;
            }

            public void ReadEnd()
            {
                IsReading = false;
            }

            public void WriteBegin()
            {
                CheckCacheVersionRebuild();

                // Reset writing state (eg if previous serialization got exception)
                if (IsWriting)
                    BuildWrite();

                StringBuilder.Clear();
                IsWriting = true;
                IsReading = false;
            }

            public void WriteEnd()
            {
                IsWriting = false;
            }

            /// <summary>Check that the cache is up to date, rebuild it if it isn't</summary>
            private void CheckCacheVersionRebuild()
            {
#if FLAX_EDITOR
                var version = Json.JsonSerializer.CurrentCacheVersion;
                if (CacheVersion == version)
                    return;

                lock (CurrentCacheSyncRoot)
                {
                    version = Json.JsonSerializer.CurrentCacheVersion;
                    if (CacheVersion == version)
                        return;

                    BuildSerializer();
                    BuildRead();
                    BuildWrite();

                    CacheVersion = version;
                }
#endif
            }

            /// <summary>Builds the serializer</summary>
            private void BuildSerializer()
            {
                JsonSerializer = Newtonsoft.Json.JsonSerializer.CreateDefault(IsManagedOnly ? SettingsManagedOnly : Settings);
                JsonSerializer.Formatting = Formatting.Indented;
                JsonSerializer.ReferenceLoopHandling = ReferenceLoopHandling.Serialize;
            }

            /// <summary>Builds the reader state</summary>
            private void BuildRead()
            {
                Reader = new StreamReader(MemoryStream, Encoding.UTF8, false);
            }

            /// <summary>Builds the writer state</summary>
            private void BuildWrite()
            {
                SerializerWriter = new JsonSerializerInternalWriter(JsonSerializer);
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
        internal static ExtendedSerializationBinder SerializationBinder;
        internal static FlaxObjectConverter ObjectConverter;
        internal static ThreadLocal<SerializerCache> Current = new ThreadLocal<SerializerCache>();
        internal static ThreadLocal<SerializerCache> Cache = new ThreadLocal<SerializerCache>(() => new SerializerCache(false));
        internal static ThreadLocal<SerializerCache> CacheManagedOnly = new ThreadLocal<SerializerCache>(() => new SerializerCache(true));
        internal static ThreadLocal<IntPtr> CachedGuidBuffer = new ThreadLocal<IntPtr>(() => Marshal.AllocHGlobal(32 * sizeof(char)), true);
        internal static string CachedGuidDigits = "0123456789abcdef";
#if FLAX_EDITOR
        /// <summary>The version of the cache, used to check that a cache is not out of date</summary>
        internal static uint CurrentCacheVersion = 0;

        /// <summary>Used to synchronize cache operations such as <see cref="SerializerCache"/> rebuild, and <see cref="ResetCache"/></summary>
        internal static readonly object CurrentCacheSyncRoot = new();
#endif

        internal static JsonSerializerSettings CreateDefaultSettings(bool isManagedOnly)
        {
            Newtonsoft.Json.Utilities.MiscellaneousUtils.ValueEquals = ValueEquals;
            if (SerializationBinder is null)
                SerializationBinder = new();
            var settings = new JsonSerializerSettings
            {
                ContractResolver = new ExtendedDefaultContractResolver(isManagedOnly),
                ReferenceLoopHandling = ReferenceLoopHandling.Ignore,
                TypeNameHandling = TypeNameHandling.Auto,
                NullValueHandling = NullValueHandling.Include,
                ObjectCreationHandling = ObjectCreationHandling.Auto,
                SerializationBinder = SerializationBinder,
            };
            if (ObjectConverter == null)
                ObjectConverter = new FlaxObjectConverter();
            settings.Converters.Add(ObjectConverter);
            settings.Converters.Add(new SceneReferenceConverter());
            settings.Converters.Add(new SoftObjectReferenceConverter());
            settings.Converters.Add(new SoftTypeReferenceConverter());
            settings.Converters.Add(new BehaviorKnowledgeSelectorAnyConverter());
            settings.Converters.Add(new MarginConverter());
            settings.Converters.Add(new VersionConverter());
            settings.Converters.Add(new LocalizedStringConverter());
            settings.Converters.Add(new TagConverter());
            //settings.Converters.Add(new GuidConverter());
            return settings;
        }

#if FLAX_EDITOR
        /// <summary>Resets the serialization cache.</summary>
        internal static void ResetCache()
        {
            lock (CurrentCacheSyncRoot)
            {
                unchecked
                {
                    CurrentCacheVersion++;
                }

                Newtonsoft.Json.JsonSerializer.ClearCache();
                SerializationBinder.ResetCache();
            }
        }
#endif

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
#if false
            // Use default value comparision used by C# json serialization library
            return Newtonsoft.Json.Utilities.MiscellaneousUtils.ValueEquals(objA, objB);
#else
            // Based on Newtonsoft.Json MiscellaneousUtils.ValueEquals but with customization for prefab object references diff
            if (objA == objB)
                return true;
            if (objA == null || objB == null)
                return false;
            
            // Special case when saving reference to prefab object and the objects are different but the point to the same prefab object
            // In that case, skip saving reference as it's defined in prefab (will be populated via IdsMapping during deserialization)
            if (objA is SceneObject sceneA && objB is SceneObject sceneB && sceneA.HasPrefabLink && sceneB.HasPrefabLink)
                return sceneA.PrefabObjectID == sceneB.PrefabObjectID;

            // Comparing an Int32 and Int64 both of the same value returns false, make types the same then compare
            if (objA.GetType() != objB.GetType())
            {
                bool IsInteger(object value)
                {
                    var type = value.GetType();
                    return type == typeof(SByte) ||
                           type == typeof(Byte) ||
                           type == typeof(Int16) ||
                           type == typeof(UInt16) ||
                           type == typeof(Int32) ||
                           type == typeof(UInt32) ||
                           type == typeof(Int64) ||
                           type == typeof(SByte) ||
                           type == typeof(UInt64);
                }
                if (IsInteger(objA) && IsInteger(objB))
                    return Convert.ToDecimal(objA, CultureInfo.CurrentCulture).Equals(Convert.ToDecimal(objB, CultureInfo.CurrentCulture));
                if ((objA is double || objA is float || objA is decimal) && (objB is double || objB is float || objB is decimal))
                    return Mathd.NearEqual(Convert.ToDouble(objA, CultureInfo.CurrentCulture), Convert.ToDouble(objB, CultureInfo.CurrentCulture));
                return false;
            }

            // Diff on collections
            if (objA is IList aList && objB is IList bList)
            {
                if (aList.Count != bList.Count)
                    return false;
            }
            if (objA is IEnumerable aEnumerable && objB is IEnumerable bEnumerable)
            {
                var aEnumerator = aEnumerable.GetEnumerator();
                var bEnumerator = bEnumerable.GetEnumerator();
                using var aEnumerator1 = aEnumerator as IDisposable;
                using var bEnumerator1 = bEnumerator as IDisposable;
                while (aEnumerator.MoveNext())
                {
                    if (!bEnumerator.MoveNext() || !ValueEquals(aEnumerator.Current, bEnumerator.Current))
                        return false;
                }
                return !bEnumerator.MoveNext();
            }

            return objA.Equals(objB);
#endif
        }

        /// <summary>
        /// Serializes the specified object.
        /// </summary>
        /// <param name="obj">The object.</param>
        /// <param name="isManagedOnly">True if serialize only C# members and skip C++ members (marked with <see cref="UnmanagedAttribute"/>).</param>
        /// <returns>The output json string.</returns>
        public static string Serialize(object obj, bool isManagedOnly = false)
        {
            return Serialize(obj, obj.GetType(), isManagedOnly);
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

            cache.WriteBegin();
            cache.SerializerWriter.Serialize(cache.JsonWriter, obj, type);
            cache.WriteEnd();

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

            cache.WriteBegin();
            cache.SerializerWriter.SerializeDiff(cache.JsonWriter, obj, type, other);
            cache.WriteEnd();

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
            cache.ReadBegin();
            using (JsonReader reader = new JsonTextReader(new StringReader(json)))
            {
                cache.JsonSerializer.Populate(reader, input);
                if (cache.JsonSerializer.CheckAdditionalContent)
                {
                    while (reader.Read())
                    {
                        if (reader.TokenType != JsonToken.Comment)
                            throw new Exception("Additional text found in JSON string after finishing deserializing object.");
                    }
                }
            }
            cache.ReadEnd();
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
            cache.ReadBegin();
            using (JsonReader reader = new JsonTextReader(new StringReader(json)))
            {
                result = cache.JsonSerializer.Deserialize(reader, objectType);
                if (cache.JsonSerializer.CheckAdditionalContent)
                {
                    while (reader.Read())
                    {
                        if (reader.TokenType != JsonToken.Comment)
                            throw new Exception("Additional text found in JSON string after finishing deserializing object.");
                    }
                }
            }
            cache.ReadEnd();
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
            cache.ReadBegin();
            using (JsonReader reader = new JsonTextReader(new StringReader(json)))
            {
                result = cache.JsonSerializer.Deserialize(reader);

                if (cache.JsonSerializer.CheckAdditionalContent)
                {
                    while (reader.Read())
                    {
                        if (reader.TokenType != JsonToken.Comment)
                            throw new Exception("Additional text found in JSON string after finishing deserializing object.");
                    }
                }
            }
            cache.ReadEnd();
            return result;
        }

        /// <summary>
        /// Deserializes the specified object (from the input json data).
        /// </summary>
        /// <param name="input">The object.</param>
        /// <param name="jsonBuffer">The input json data buffer (raw, fixed memory buffer).</param>
        /// <param name="jsonLength">The input json data buffer length (characters count).</param>
        public static unsafe void Deserialize(object input, byte* jsonBuffer, int jsonLength)
        {
            var cache = Cache.Value;
            cache.ReadBegin();

            /*// Debug json string reading
            cache.MemoryStream.Initialize(jsonBuffer, jsonLength);
            cache.Reader.DiscardBufferedData();
            string json = cache.Reader.ReadToEnd();*/

            cache.MemoryStream.Initialize(jsonBuffer, jsonLength);
            cache.Reader.DiscardBufferedData();
            var jsonReader = new JsonTextReader(cache.Reader);
            if (*jsonBuffer != (byte)'{' && input is LocalizedString asLocalizedString)
            {
                // Hack for objects that are serialized into sth different thant "{..}" (eg. LocalizedString can be saved as plain string if not using localization)
                asLocalizedString.Id = null;
                asLocalizedString.Value = jsonReader.ReadAsString();
            }
            else
            {
                cache.JsonSerializer.Populate(jsonReader, input);
            }
            if (cache.JsonSerializer.CheckAdditionalContent)
            {
                while (jsonReader.Read())
                {
                    if (jsonReader.TokenType != JsonToken.Comment)
                        throw new Exception("Additional text found in JSON string after finishing deserializing object.");
                }
            }

            cache.ReadEnd();
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
        /// <returns>The identifier.</returns>
        public static Guid ParseID(string str)
        {
            ParseID(str, out var id);
            return id;
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
