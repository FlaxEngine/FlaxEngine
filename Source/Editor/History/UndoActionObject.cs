// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEditor.Utilities;
using FlaxEngine;
using Newtonsoft.Json;
using JsonSerializer = FlaxEngine.Json.JsonSerializer;

namespace FlaxEditor.History
{
    /// <summary>
    /// Undo action object.
    /// </summary>
    /// <seealso cref="IUndoAction" />
    [Serializable]
    public class UndoActionObject : UndoActionBase<UndoActionObject.DataStorage>
    {
        /// <summary>
        /// The data value storage to solve issue for flax objects and editor scene tree nodes which are serialized by ref id.
        /// </summary>
        [Serializable]
        public struct DataValue
        {
            /// <summary>
            /// The generic value (anything).
            /// </summary>
            public object Generic;

            /// <summary>
            /// The flax object.
            /// </summary>
            public FlaxEngine.Object FlaxObject;

            /// <summary>
            /// The editor node object.
            /// </summary>
            public SceneGraph.SceneGraphNode EditorNode;

            /// <summary>
            /// The id.
            /// </summary>
            public Guid? Id;

            /// <summary>
            /// Gets the proper value.
            /// </summary>
            [NoSerialize]
            public object Value => EditorNode ?? FlaxObject ?? Generic ?? Id;

            /// <summary>
            /// Initializes a new instance of the <see cref="DataValue"/> struct.
            /// </summary>
            /// <param name="value">The value.</param>
            public DataValue(object value)
            {
                Generic = null;
                FlaxObject = null;
                EditorNode = null;
                Id = null;

                if (value is SceneGraph.SceneGraphNode editorNode)
                {
                    EditorNode = editorNode;
                }
                else if (value is FlaxEngine.Object flaxObject)
                {
                    FlaxObject = flaxObject;
                }
                else if (value is Guid guid)
                {
                    Id = guid;
                }
                else
                {
                    Generic = value;
                }
            }
        }

        /// <summary>
        /// The data.
        /// </summary>
        [Serializable]
        public struct DataStorage
        {
            /// <summary>
            /// The values 1.
            /// </summary>
            public DataValue[] Values1;

            /// <summary>
            /// The values 2.
            /// </summary>
            public DataValue[] Values2;

            /// <summary>
            /// The instance.
            /// </summary>
            public DataValue Instance;
        }

        /// <summary>
        /// Prepared undo data container object.
        /// </summary>
        public struct DataPrepared
        {
            /// <summary>
            /// The difference data.
            /// </summary>
            public MemberComparison[] Diff;

            /// <summary>
            /// The target object instance.
            /// </summary>
            public object TargetInstance;
        }

        internal static JsonSerializerSettings JsonSettings;

        // For objects that cannot be referenced in undo action like: FlaxEngine.Object or SceneGraphNode we store them in DataStorage,
        // otherwise here:
        private readonly object TargetInstance;

        private readonly MemberInfoPath[] Members;

        /// <summary>
        /// Gets the target.
        /// </summary>
        public object Target => TargetInstance;

        /// <summary>
        /// Initializes a new instance of the <see cref="UndoActionObject"/> class.
        /// </summary>
        /// <param name="diff">The difference.</param>
        /// <param name="actionString">The action string.</param>
        /// <param name="targetInstance">The target instance.</param>
        public UndoActionObject(List<MemberComparison> diff, string actionString, object targetInstance)
        {
            bool useDataStorageForInstance = targetInstance is FlaxEngine.Object || targetInstance is SceneGraph.SceneGraphNode;

            ActionString = actionString;
            TargetInstance = useDataStorageForInstance ? null : targetInstance;

            int count = diff.Count;
            var values1 = new DataValue[count];
            var values2 = new DataValue[count];
            Members = new MemberInfoPath[count];
            for (int i = 0; i < count; i++)
            {
                values1[i] = new DataValue(diff[i].Value1);
                values2[i] = new DataValue(diff[i].Value2);
                Members[i] = diff[i].MemberPath;
            }

            Data = new DataStorage
            {
                Values1 = values1,
                Values2 = values2,
                Instance = new DataValue(useDataStorageForInstance ? targetInstance : null)
            };
        }

        /// <summary>
        /// Prepares the data for the undo.
        /// </summary>
        /// <returns>The prepared undo action data.</returns>
        public DataPrepared PrepareData()
        {
            var data = Data;
            var count = data.Values1.Length;
            var diff = new MemberComparison[count];
            for (int i = 0; i < count; i++)
            {
                diff[i] = new MemberComparison(Members[i], data.Values1[i].Value, data.Values2[i].Value);
            }
            return new DataPrepared
            {
                Diff = diff,
                TargetInstance = data.Instance.Value ?? TargetInstance,
            };
        }

        /// <inheritdoc />
        public override DataStorage Data
        {
            protected set
            {
                // Inject objects typename serialization to prevent data type mismatch when loading from saved state
                var settings = JsonSettings;
                if (settings == null)
                {
                    settings = JsonSerializer.CreateDefaultSettings(false);
                    settings.TypeNameHandling = TypeNameHandling.All;
                    JsonSettings = settings;
                }
                _data = JsonConvert.SerializeObject(value, Formatting.Indented, settings);
                //Editor.Log(_data);
            }
        }

        /// <inheritdoc />
        public override string ActionString { get; }

        /// <inheritdoc />
        public override void Do()
        {
            var data = PrepareData();
            for (var i = 0; i < data.Diff.Length; i++)
            {
                var diff = data.Diff[i];
                diff.SetMemberValue(data.TargetInstance, diff.Value2);
            }
        }

        /// <inheritdoc />
        public override void Undo()
        {
            var data = PrepareData();
            for (var i = data.Diff.Length - 1; i >= 0; i--)
            {
                var diff = data.Diff[i];
                diff.SetMemberValue(data.TargetInstance, diff.Value1);
            }
        }
    }
}
