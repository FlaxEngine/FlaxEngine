// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using FlaxEngine.GUI;
using Newtonsoft.Json;

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

    /// <summary>
    /// Serialize SoftObjectReference as Guid in internal format.
    /// </summary>
    /// <seealso cref="Newtonsoft.Json.JsonConverter" />
    internal class MarginConverter : JsonConverter
    {
        /// <inheritdoc />
        public override void WriteJson(JsonWriter writer, object value, Newtonsoft.Json.JsonSerializer serializer)
        {
            var valueMargin = (Margin)value;

            writer.WriteStartObject();
            {
                writer.WritePropertyName("Left");
                writer.WriteValue(valueMargin.Left);
                writer.WritePropertyName("Right");
                writer.WriteValue(valueMargin.Right);
                writer.WritePropertyName("Top");
                writer.WriteValue(valueMargin.Top);
                writer.WritePropertyName("Bottom");
                writer.WriteValue(valueMargin.Bottom);
            }
            writer.WriteEndObject();
        }

        /// <inheritdoc />
        public override void WriteJsonDiff(JsonWriter writer, object value, object other, Newtonsoft.Json.JsonSerializer serializer)
        {
            var valueMargin = (Margin)value;
            var otherMargin = (Margin)other;
            writer.WriteStartObject();
            if (!Mathf.NearEqual(valueMargin.Left, otherMargin.Left))
            {
                writer.WritePropertyName("Left");
                writer.WriteValue(valueMargin.Left);
            }
            if (!Mathf.NearEqual(valueMargin.Right, otherMargin.Right))
            {
                writer.WritePropertyName("Right");
                writer.WriteValue(valueMargin.Right);
            }
            if (!Mathf.NearEqual(valueMargin.Top, otherMargin.Top))
            {
                writer.WritePropertyName("Top");
                writer.WriteValue(valueMargin.Top);
            }
            if (!Mathf.NearEqual(valueMargin.Bottom, otherMargin.Bottom))
            {
                writer.WritePropertyName("Bottom");
                writer.WriteValue(valueMargin.Bottom);
            }
            writer.WriteEndObject();
        }

        /// <inheritdoc />
        public override object ReadJson(JsonReader reader, Type objectType, object existingValue, Newtonsoft.Json.JsonSerializer serializer)
        {
            var value = (Margin?)existingValue ?? new Margin();
            if (reader.TokenType == JsonToken.StartObject)
            {
                while (reader.Read())
                {
                    switch (reader.TokenType)
                    {
                    case JsonToken.PropertyName:
                    {
                        var propertyName = (string)reader.Value;
                        var propertyValue = (float)reader.ReadAsDouble();
                        switch (propertyName)
                        {
                        case "Left":
                            value.Left = propertyValue;
                            break;
                        case "Right":
                            value.Right = propertyValue;
                            break;
                        case "Top":
                            value.Top = propertyValue;
                            break;
                        case "Bottom":
                            value.Bottom = propertyValue;
                            break;
                        }
                        break;
                    }
                    case JsonToken.Comment: break;
                    default: return value;
                    }
                }
            }
            return value;
        }

        /// <inheritdoc />
        public override bool CanConvert(Type objectType)
        {
            return objectType == typeof(Margin);
        }

        /// <inheritdoc />
        public override bool CanRead => true;

        /// <inheritdoc />
        public override bool CanWrite => true;

        /// <inheritdoc />
        public override bool CanWriteDiff => true;
    }

    /// <summary>
    /// Serialize LocalizedString as inlined text is not using localization (Id member is empty).
    /// </summary>
    /// <seealso cref="Newtonsoft.Json.JsonConverter" />
    internal class LocalizedStringConverter : JsonConverter
    {
        /// <inheritdoc />
        public override void WriteJson(JsonWriter writer, object value, Newtonsoft.Json.JsonSerializer serializer)
        {
            var str = (LocalizedString)value;
            if (string.IsNullOrEmpty(str.Id))
            {
                writer.WriteValue(str.Value);
            }
            else
            {
                writer.WriteStartObject();
                writer.WritePropertyName("Id");
                writer.WriteValue(str.Id);
                writer.WritePropertyName("Value");
                writer.WriteValue(str.Value);
                writer.WriteEndObject();
            }
        }

        /// <inheritdoc />
        public override object ReadJson(JsonReader reader, Type objectType, object existingValue, Newtonsoft.Json.JsonSerializer serializer)
        {
            var str = existingValue as LocalizedString ?? new LocalizedString();
            if (reader.TokenType == JsonToken.String)
            {
                str.Id = null;
                str.Value = (string)reader.Value;
            }
            else if (reader.TokenType == JsonToken.StartObject)
            {
                while (reader.Read())
                {
                    switch (reader.TokenType)
                    {
                    case JsonToken.PropertyName:
                    {
                        var propertyName = (string)reader.Value;
                        switch (propertyName)
                        {
                        case "Id":
                            str.Id = reader.ReadAsString();
                            break;
                        case "Value":
                            str.Value = reader.ReadAsString();
                            break;
                        }
                        break;
                    }
                    case JsonToken.Comment: break;
                    default: return str;
                    }
                }
            }
            return str;
        }

        /// <inheritdoc />
        public override bool CanConvert(Type objectType)
        {
            return objectType == typeof(LocalizedString);
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
}
