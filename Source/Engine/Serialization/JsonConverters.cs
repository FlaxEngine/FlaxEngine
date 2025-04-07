// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEngine.GUI;
using Newtonsoft.Json;

namespace FlaxEngine.Json
{
    /// <summary>
    /// Serialize references to the <see cref="FlaxEngine.Object"/> as Guid.
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
            if (cache != null && cache.IsWriting && cache.SerializerWriter.SerializeStackSize == 0)
            {
                return false;
            }
            return typeof(Object).IsAssignableFrom(objectType);
        }
    }

    /// <summary>
    /// Serialize <see cref="SceneReference"/> as Guid in internal format.
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
    /// Serialize <see cref="SoftObjectReference"/> as Guid in internal format.
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
    /// Serialize <see cref="SoftTypeReference"/> as typename string in internal format.
    /// </summary>
    /// <seealso cref="Newtonsoft.Json.JsonConverter" />
    internal class SoftTypeReferenceConverter : JsonConverter
    {
        /// <inheritdoc />
        public override void WriteJson(JsonWriter writer, object value, Newtonsoft.Json.JsonSerializer serializer)
        {
            writer.WriteValue(((SoftTypeReference)value).TypeName);
        }

        /// <inheritdoc />
        public override object ReadJson(JsonReader reader, Type objectType, object existingValue, Newtonsoft.Json.JsonSerializer serializer)
        {
            var result = new SoftTypeReference();
            if (reader.TokenType == JsonToken.String)
                result.TypeName = (string)reader.Value;
            return result;
        }

        /// <inheritdoc />
        public override bool CanConvert(Type objectType)
        {
            return objectType == typeof(SoftTypeReference);
        }
    }

    /// <summary>
    /// Serialize <see cref="BehaviorKnowledgeSelectorAny"/> as path string in internal format.
    /// </summary>
    /// <seealso cref="Newtonsoft.Json.JsonConverter" />
    internal class BehaviorKnowledgeSelectorAnyConverter : JsonConverter
    {
        /// <inheritdoc />
        public override void WriteJson(JsonWriter writer, object value, Newtonsoft.Json.JsonSerializer serializer)
        {
            writer.WriteValue(((BehaviorKnowledgeSelectorAny)value).Path);
        }

        /// <inheritdoc />
        public override object ReadJson(JsonReader reader, Type objectType, object existingValue, Newtonsoft.Json.JsonSerializer serializer)
        {
            var result = new BehaviorKnowledgeSelectorAny();
            if (reader.TokenType == JsonToken.String)
                result.Path = (string)reader.Value;
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
                        case "Path":
                            result.Path = reader.ReadAsString();
                            break;
                        }
                        break;
                    }
                    case JsonToken.Comment: break;
                    case JsonToken.String: break;
                    default: return result;
                    }
                }
            }
            return result;
        }

        /// <inheritdoc />
        public override bool CanConvert(Type objectType)
        {
            return objectType == typeof(BehaviorKnowledgeSelectorAny);
        }
    }

    /// <summary>
    /// Serialize <see cref="Margin"/> as Guid in internal format.
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
#if FLAX_EDITOR
                if ((serializer.TypeNameHandling & TypeNameHandling.Objects) == TypeNameHandling.Objects)
                {
                    writer.WritePropertyName("$type");
                    writer.WriteValue("FlaxEngine.GUI.Margin, FlaxEngine.CSharp");
                }
#endif
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
                        reader.Read();
                        switch (reader.TokenType)
                        {
                        case JsonToken.Integer:
                        case JsonToken.Float:
                        {
                            var propertyValue = Convert.ToSingle(reader.Value);
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
                        }
                        break;
                    }
                    case JsonToken.Comment: break;
                    case JsonToken.String: break;
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
    /// Serialize <see cref="LocalizedString"/> as inlined text is not using localization (Id member is empty).
    /// </summary>
    /// <seealso cref="Newtonsoft.Json.JsonConverter" />
    internal class LocalizedStringConverter : JsonConverter
    {
        /// <inheritdoc />
        public override void WriteJson(JsonWriter writer, object value, Newtonsoft.Json.JsonSerializer serializer)
        {
#if FLAX_EDITOR
            bool writeTypename = (serializer.TypeNameHandling & TypeNameHandling.Objects) == TypeNameHandling.Objects;
#else
            bool writeTypename = false;
#endif
            var str = (LocalizedString)value;
            if (string.IsNullOrEmpty(str.Id) && !writeTypename)
            {
                writer.WriteValue(str.Value ?? string.Empty);
            }
            else
            {
                writer.WriteStartObject();
#if FLAX_EDITOR
                if (writeTypename)
                {
                    writer.WritePropertyName("$type");
                    writer.WriteValue("FlaxEngine.LocalizedString, FlaxEngine.CSharp");
                }
#endif
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
                    case JsonToken.String: break;
                    default: return str;
                    }
                }
            }
            else
                return null;
            if (existingValue == null && str.Id == null && str.Value == null)
                return null;
            return str;
        }

        /// <inheritdoc />
        public override bool CanConvert(Type objectType)
        {
            return objectType == typeof(LocalizedString);
        }
    }

    /// <summary>
    /// Serialize <see cref="Tag"/> as inlined text.
    /// </summary>
    /// <seealso cref="Newtonsoft.Json.JsonConverter" />
    internal class TagConverter : JsonConverter
    {
        /// <inheritdoc />
        public override void WriteJson(JsonWriter writer, object value, Newtonsoft.Json.JsonSerializer serializer)
        {
            var tag = (Tag)value;
            writer.WriteValue(tag.ToString());
        }

        /// <inheritdoc />
        public override object ReadJson(JsonReader reader, Type objectType, object existingValue, Newtonsoft.Json.JsonSerializer serializer)
        {
            if (reader.TokenType == JsonToken.String)
                return Tags.Get((string)reader.Value);
            return Tag.Default;
        }

        /// <inheritdoc />
        public override bool CanConvert(Type objectType)
        {
            return objectType == typeof(Tag);
        }
    }

    internal sealed class JsonAssetReferenceConverter : JsonConverter
    {
        /// <inheritdoc />
        public override unsafe void WriteJson(JsonWriter writer, object value, Newtonsoft.Json.JsonSerializer serializer)
        {
            var asset = (JsonAsset)value.GetType().GetField("Asset").GetValue(value);
            var id = asset?.ID ?? Guid.Empty;
            writer.WriteValue(JsonSerializer.GetStringID(&id));
        }

        /// <inheritdoc />
        public override object ReadJson(JsonReader reader, Type objectType, object existingValue, Newtonsoft.Json.JsonSerializer serializer)
        {
            var result = existingValue ?? Activator.CreateInstance(objectType);
            if (reader.TokenType == JsonToken.String)
            {
                JsonSerializer.ParseID((string)reader.Value, out var id);
                var asset = Content.LoadAsync<JsonAsset>(id);
                objectType.GetField("Asset").SetValue(result, asset);
            }
            else if (reader.TokenType == JsonToken.StartObject)
            {
                // [Deprecated on 26.07.2024, expires on 26.07.2026]
                while (reader.Read() && reader.TokenType != JsonToken.EndObject)
                {
                    switch (reader.TokenType)
                    {
                    case JsonToken.PropertyName:
                    {
                        var propertyName = (string)reader.Value;
                        reader.Read();
                        if (propertyName == "Asset" && reader.TokenType == JsonToken.String)
                        {
                            JsonSerializer.ParseID((string)reader.Value, out var id);
                            var asset = Content.LoadAsync<JsonAsset>(id);
                            objectType.GetField("Asset").SetValue(result, asset);
                        }

                        break;
                    }
                    }
                }
            }

            return result;
        }

        /// <inheritdoc />
        public override bool CanConvert(Type objectType)
        {
            return objectType.Name.StartsWith("JsonAssetReference", StringComparison.Ordinal);
        }
    }

    internal class ControlReferenceConverter : JsonConverter
    {
        /// <inheritdoc />
        public override unsafe void WriteJson(JsonWriter writer, object value, Newtonsoft.Json.JsonSerializer serializer)
        {
            var id = (value as IControlReference)?.UIControl?.ID ?? Guid.Empty;
            writer.WriteValue(JsonSerializer.GetStringID(&id));
        }

        /// <inheritdoc />
        public override void WriteJsonDiff(JsonWriter writer, object value, object other, Newtonsoft.Json.JsonSerializer serializer)
        {
            if (value is IControlReference valueRef &&
                other is IControlReference otherRef &&
                JsonSerializer.SceneObjectEquals(valueRef.UIControl, otherRef.UIControl))
                return;
            base.WriteJsonDiff(writer, value, other, serializer);
        }

        /// <inheritdoc />
        public override object ReadJson(JsonReader reader, Type objectType, object existingValue, Newtonsoft.Json.JsonSerializer serializer)
        {
            var result = existingValue ?? Activator.CreateInstance(objectType);
            if (reader.TokenType == JsonToken.String && result is IControlReference controlReference)
            {
                JsonSerializer.ParseID((string)reader.Value, out var id);
                controlReference.Load(Object.Find<UIControl>(ref id));
            }
            return result;
        }

        /// <inheritdoc />
        public override bool CanConvert(Type objectType)
        {
            return objectType.Name.StartsWith("ControlReference", StringComparison.Ordinal);
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
