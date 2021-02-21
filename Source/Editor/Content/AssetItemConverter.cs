// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using Newtonsoft.Json;

namespace FlaxEditor.Content
{
    /// <summary>
    /// Serialize references to the FlaxEngine.Object as Guid (format N).
    /// </summary>
    /// <seealso cref="Newtonsoft.Json.JsonConverter" />
    internal class AssetItemConverter : JsonConverter
    {
        /// <inheritdoc />
        public override void WriteJson(JsonWriter writer, object value, Newtonsoft.Json.JsonSerializer serializer)
        {
            Guid id = Guid.Empty;
            if (value is AssetItem obj)
                id = obj.ID;

            writer.WriteValue(FlaxEngine.Json.JsonSerializer.GetStringID(id));
        }

        /// <inheritdoc />
        public override object ReadJson(JsonReader reader, Type objectType, object existingValue, Newtonsoft.Json.JsonSerializer serializer)
        {
            if (reader.TokenType == JsonToken.String)
            {
                FlaxEngine.Json.JsonSerializer.ParseID((string)reader.Value, out Guid id);
                return Editor.Instance.ContentDatabase.Find(id);
            }

            return null;
        }

        /// <inheritdoc />
        public override bool CanConvert(Type objectType)
        {
            return typeof(AssetItem).IsAssignableFrom(objectType);
        }
    }
}
