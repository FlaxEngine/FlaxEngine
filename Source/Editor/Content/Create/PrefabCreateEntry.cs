// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.Scripting;
using FlaxEngine;
using Object = FlaxEngine.Object;

namespace FlaxEditor.Content.Create
{
    /// <summary>
    /// Visual Script asset creating handler. Allows to specify base class to inherit from.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.Create.CreateFileEntry" />
    public class PrefabCreateEntry : CreateFileEntry
    {
        /// <summary>
        /// The create options.
        /// </summary>
        public class Options
        {
            /// <summary>
            /// The root actor.
            /// </summary>
            [TypeReference(typeof(FlaxEngine.Actor), nameof(IsValid))]
            [Tooltip("The actor type of the root of the new Prefab.")]
            public Type RootActorType = typeof(EmptyActor);

            private static bool IsValid(Type type)
            {
                return (type.IsPublic || type.IsNestedPublic) && !type.IsAbstract && !type.IsGenericType;
            }
        }

        private readonly Options _options = new Options();

        /// <inheritdoc />
        public override object Settings => _options;

        /// <summary>
        /// Initializes a new instance of the <see cref="PrefabCreateEntry"/> class.
        /// </summary>
        /// <param name="resultUrl">The result file url.</param>
        public PrefabCreateEntry(string resultUrl)
        : base("Settings", resultUrl)
        {
        }

        /// <inheritdoc />
        public override bool Create()
        {
            if (_options.RootActorType == null)
                _options.RootActorType = typeof(EmptyActor);

            ScriptType actorType = new ScriptType(_options.RootActorType);

            Actor actor = null;
            try
            {
                actor = actorType.CreateInstance() as Actor;
                Object.Destroy(actor, 20.0f);
            }
            catch (Exception ex)
            {
                Editor.LogError("Failed to create prefab with root actor type: " + actorType.Name);
                Editor.LogWarning(ex);
                return true;
            }

            return PrefabManager.CreatePrefab(actor, ResultUrl, true);
        }
    }
}
