// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.IO;
using FlaxEditor.Utilities;
using FlaxEngine;

namespace FlaxEditor.GUI.Timeline.Tracks
{
    /// <summary>
    /// The timeline track for animating <see cref="FlaxEngine.Script"/> objects.
    /// </summary>
    /// <seealso cref="ObjectTrack" />
    public sealed class ScriptTrack : ObjectTrack
    {
        /// <summary>
        /// Gets the archetype.
        /// </summary>
        /// <returns>The archetype.</returns>
        public static TrackArchetype GetArchetype()
        {
            return new TrackArchetype
            {
                TypeId = 8,
                Name = "Script",
                DisableSpawnViaGUI = true,
                Create = options => new ScriptTrack(ref options),
                Load = LoadTrack,
                Save = SaveTrack,
            };
        }

        private static void LoadTrack(int version, Track track, BinaryReader stream)
        {
            var e = (ScriptTrack)track;
            e.ScriptID = stream.ReadGuid();
        }

        private static void SaveTrack(Track track, BinaryWriter stream)
        {
            var e = (ScriptTrack)track;
            stream.WriteGuid(ref e.ScriptID);
        }

        /// <summary>
        /// The object ID.
        /// </summary>
        public Guid ScriptID;

        /// <summary>
        /// Gets the object instance (it might be missing).
        /// </summary>
        public Script Script
        {
            get => FlaxEngine.Object.TryFind<Script>(ref ScriptID);
            set => ScriptID = value?.ID ?? Guid.Empty;
        }

        /// <inheritdoc />
        public ScriptTrack(ref TrackCreateOptions options)
        : base(ref options)
        {
        }

        /// <inheritdoc />
        public override object Object => Script;

        /// <inheritdoc />
        protected override void OnShowAddContextMenu(ContextMenu.ContextMenu menu)
        {
            base.OnShowAddContextMenu(menu);

            var script = Script;
            if (script == null)
            {
                menu.AddButton("Missing script");
                return;
            }

            var type = script.GetType();
            if (AddProperties(this, menu, type) != 0)
                menu.AddSeparator();

            AddEvents(this, menu, type);
        }
    }
}
