// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using Newtonsoft.Json;
using Newtonsoft.Json.Converters;
using Newtonsoft.Json.Serialization;

namespace FlaxEngine.Json.JsonCustomSerializers
{
    internal class ExtendedDefaultContractResolver : DefaultContractResolver
    {
        private readonly Type _flaxType = typeof(Object);
        private static readonly JsonConverter InterfaceObjectReferenceConverterInstance = new InterfaceObjectReferenceConverter();

        private readonly Type[] AttributesIgnoreList =
        {
            typeof(NonSerializedAttribute),
            typeof(NoSerializeAttribute)
        };

        private readonly Type[] AttributesIgnoreListManaged =
        {
            typeof(UnmanagedAttribute),
            typeof(NonSerializedAttribute),
            typeof(NoSerializeAttribute)
        };

        private readonly Type[] _attributesIgnoreList;

        public ExtendedDefaultContractResolver(bool isManagedOnly)
        {
            _attributesIgnoreList = isManagedOnly ? AttributesIgnoreListManaged : AttributesIgnoreList;
        }

        private static bool HasObjectInterfaceReferenceAttribute(IEnumerable<Attribute> attributes)
        {
            return attributes.Any(x => x is ScriptingObjectInterfaceReferenceAttribute || x is SoftObjectInterfaceReferenceAttribute);
        }

        private static Type GetCollectionItemType(Type type)
        {
            if (type.IsArray)
                return type.GetElementType();
            if (!type.IsGenericType || type == typeof(string))
                return null;

            var types = type.GetInterfaces().Concat(new[] { type });
            var dictionaryType = types.FirstOrDefault(x => x.IsGenericType && x.GetGenericTypeDefinition() == typeof(IDictionary<,>));
            if (dictionaryType != null)
                return dictionaryType.GetGenericArguments()[1];
            var enumerableType = types.FirstOrDefault(x => x.IsGenericType && x.GetGenericTypeDefinition() == typeof(IEnumerable<>));
            return enumerableType?.GetGenericArguments()[0];
        }

        private static void SetupInterfaceObjectReferenceItems(JsonContainerContract contract, Type itemType)
        {
            if (itemType?.IsInterface == true)
            {
                contract.ItemReferenceLoopHandling = ReferenceLoopHandling.Serialize;
                contract.ItemConverter = InterfaceObjectReferenceConverterInstance;
            }
        }

        private void SetupObjectReferenceProperty(JsonProperty jsonProperty, Type type, IEnumerable<Attribute> attributes)
        {
            var hasObjectInterfaceReferenceAttribute = HasObjectInterfaceReferenceAttribute(attributes);
            if (_flaxType.IsAssignableFrom(type) || (type.IsInterface && hasObjectInterfaceReferenceAttribute))
            {
                jsonProperty.ReferenceLoopHandling = ReferenceLoopHandling.Serialize;
                jsonProperty.Converter = JsonSerializer.ObjectConverter;
            }
            if (hasObjectInterfaceReferenceAttribute && GetCollectionItemType(type)?.IsInterface == true)
            {
                jsonProperty.ItemReferenceLoopHandling = ReferenceLoopHandling.Serialize;
                jsonProperty.ItemConverter = JsonSerializer.ObjectConverter;
            }
        }

        private sealed class InterfaceObjectReferenceConverter : JsonConverter
        {
            public override unsafe void WriteJson(JsonWriter writer, object value, Newtonsoft.Json.JsonSerializer serializer)
            {
                if (value is Object obj)
                {
                    var id = obj.ID;
                    writer.WriteValue(JsonSerializer.GetStringID(&id));
                }
                else if (value == null)
                {
                    writer.WriteNull();
                }
                else
                {
                    serializer.Serialize(writer, value, value.GetType());
                }
            }

            public override object ReadJson(JsonReader reader, Type objectType, object existingValue, Newtonsoft.Json.JsonSerializer serializer)
            {
                if (reader.TokenType == JsonToken.String && JsonSerializer.TryParseID((string)reader.Value, out var id))
                {
                    return Object.Find(ref id, objectType, true);
                }
                if (reader.TokenType == JsonToken.Null)
                    return null;
                // objectType is the same interface item type that selected this converter. Passing it back to
                // Newtonsoft can cause this converter to be chosen again and recurse until the stack overflows.
                return Newtonsoft.Json.Linq.JToken.Load(reader).ToObject<object>(serializer);
            }

            public override bool CanConvert(Type objectType)
            {
                return objectType.IsInterface;
            }
        }

        /// <inheritdoc />
        protected override JsonContract CreateContract(Type objectType)
        {
            var contract = base.CreateContract(objectType);

            // Override contract for Flax objects
            if (_flaxType.IsAssignableFrom(objectType))
            {
                ((JsonObjectContract)contract).ItemReferenceLoopHandling = ReferenceLoopHandling.Serialize;
            }

            // Check if use enum serialization as string
            var type = Nullable.GetUnderlyingType(objectType) ?? objectType;
            if (type.IsEnum && type.GetCustomAttribute<EnumStringAttribute>() != null)
            {
                contract.Converter = new StringEnumConverter();
            }

            return contract;
        }

        /// <inheritdoc />
        protected override JsonArrayContract CreateArrayContract(Type objectType)
        {
            var contract = base.CreateArrayContract(objectType);

            SetupInterfaceObjectReferenceItems(contract, contract.CollectionItemType);

            return contract;
        }

        /// <inheritdoc />
        protected override JsonDictionaryContract CreateDictionaryContract(Type objectType)
        {
            var contract = base.CreateDictionaryContract(objectType);

            SetupInterfaceObjectReferenceItems(contract, contract.DictionaryValueType);

            // Override contract to save enums keys as integer
            var keyType = contract.DictionaryKeyType;
            if ((keyType?.IsEnum ?? false) && keyType.GetCustomAttribute<EnumStringAttribute>() == null)
            {
                contract.DictionaryKeyResolver = name =>
                {
                    try
                    {
                        var e = Enum.Parse(keyType, name);
                        name = Convert.ToInt32(e).ToString();
                    }
                    catch (Exception ex)
                    {
                        Debug.Logger.LogHandler.LogWrite(LogType.Warning, $"Failed to parse enum '{name}' as {keyType.Name}: {ex.Message}");
                    }
                    return name;
                };
            }

            return contract;
        }

        protected override IList<JsonProperty> CreateProperties(Type type, MemberSerialization memberSerialization)
        {
            var fields = type.GetFields(BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance);
            var properties = type.GetProperties(BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance);

            var result = new List<JsonProperty>(fields.Length + properties.Length);

            for (int i = 0; i < fields.Length; i++)
            {
                var f = fields[i];
                var attributes = f.GetCustomAttributes();

                // Serialize non-public fields only with a proper attribute
                if (!f.IsPublic && !attributes.Any(x => x is SerializeAttribute))
                    continue;

                // Check if has attribute to skip serialization
                bool noSerialize = false;
                foreach (var attribute in attributes)
                {
                    if (_attributesIgnoreList.Contains(attribute.GetType()))
                    {
                        noSerialize = true;
                        break;
                    }
                }

                if (noSerialize)
                    continue;

                var jsonProperty = CreateProperty(f, memberSerialization);
                jsonProperty.Writable = true;
                jsonProperty.Readable = true;

                SetupObjectReferenceProperty(jsonProperty, f.FieldType, attributes);

                result.Add(jsonProperty);
            }

            for (int i = 0; i < properties.Length; i++)
            {
                var p = properties[i];

                // Serialize only properties with read/write
                if (!(p.CanRead && p.CanWrite && p.GetIndexParameters().GetLength(0) == 0))
                    continue;

                var attributes = p.GetCustomAttributes();

                // Serialize non-public properties only with a proper attribute
                if ((!p.GetMethod.IsPublic || !p.SetMethod.IsPublic) && !attributes.Any(x => x is SerializeAttribute))
                    continue;

                // Check if has attribute to skip serialization
                bool noSerialize = false;
                foreach (var attribute in attributes)
                {
                    if (_attributesIgnoreList.Contains(attribute.GetType()))
                    {
                        noSerialize = true;
                        break;
                    }
                }

                if (noSerialize)
                    continue;

                var isObsolete = attributes.Any(x => x is ObsoleteAttribute);

                var jsonProperty = CreateProperty(p, memberSerialization);
                jsonProperty.Writable = true;
                jsonProperty.Readable = !isObsolete;

                SetupObjectReferenceProperty(jsonProperty, p.PropertyType, attributes);

                result.Add(jsonProperty);
            }

            return result;
        }
    }
}
