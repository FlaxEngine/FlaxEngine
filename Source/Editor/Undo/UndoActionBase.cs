// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.SceneGraph;
using FlaxEngine;
using Newtonsoft.Json;

namespace FlaxEditor
{
    internal class SceneTreeNodeConverter : JsonConverter
    {
        /// <inheritdoc />
        public override void WriteJson(JsonWriter writer, object value, Newtonsoft.Json.JsonSerializer serializer)
        {
            Guid id = Guid.Empty;
            if (value is SceneGraphNode obj)
                id = obj.ID;

            writer.WriteValue(id.ToString("N"));
        }

        /// <inheritdoc />
        public override object ReadJson(JsonReader reader, Type objectType, object existingValue, Newtonsoft.Json.JsonSerializer serializer)
        {
            if (reader.TokenType == JsonToken.String)
            {
                var id = Guid.Parse((string)reader.Value);
                return SceneGraphFactory.FindNode(id);
            }
            return null;
        }

        /// <inheritdoc />
        public override bool CanConvert(Type objectType)
        {
            return typeof(SceneGraphNode).IsAssignableFrom(objectType);
        }
    }

    /// <summary>
    /// Base class for <see cref="IUndoAction"/> implementations. Stores undo data serialized and preserves references to the game objects.
    /// </summary>
    /// <typeparam name="TData">The type of the data. Must have <see cref="SerializableAttribute"/>.</typeparam>
    /// <seealso cref="FlaxEditor.IUndoAction" />
    [Serializable, HideInEditor]
    public abstract class UndoActionBase<TData> : IUndoAction where TData : struct
    {
        /// <summary>
        /// The serialized data (Json text).
        /// </summary>
        [Serialize]
        protected string _data;

        /// <summary>
        /// Gets or sets the serialized undo data.
        /// </summary>
        [NoSerialize]
        public virtual TData Data
        {
            get => JsonConvert.DeserializeObject<TData>(_data, FlaxEngine.Json.JsonSerializer.Settings);
            protected set => _data = JsonConvert.SerializeObject(value, Formatting.None, FlaxEngine.Json.JsonSerializer.Settings);
        }

        /// <inheritdoc />
        public abstract string ActionString { get; }

        /// <inheritdoc />
        public abstract void Do();

        /// <inheritdoc />
        public abstract void Undo();

        /// <inheritdoc />
        public virtual void Dispose()
        {
            _data = null;
        }
    }
}
