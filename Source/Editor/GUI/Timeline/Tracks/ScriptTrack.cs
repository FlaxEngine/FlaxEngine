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
            get
            {
                if (Flags.HasFlag(TrackFlags.PrefabObject))
                {
                    // TODO: reuse cached script to improve perf
                    foreach (var window in Editor.Instance.Windows.Windows)
                    {
                        if (window is Windows.Assets.PrefabWindow prefabWindow && prefabWindow.Graph.MainActor)
                        {
                            var script = FindScriptWithPrefabObjectID(prefabWindow.Graph.MainActor, ref ScriptID);
                            if (script != null)
                                return script;
                        }
                    }
                    return null;
                }
                return FlaxEngine.Object.TryFind<Script>(ref ScriptID);
            }
            set
            {
                if (value != null)
                {
                    if (value.HasPrefabLink && !value.HasScene)
                    {
                        // Track with prefab object reference assigned in Editor
                        ScriptID = value.PrefabObjectID;
                        Flags |= TrackFlags.PrefabObject;
                    }
                    else
                    {
                        ScriptID = value.ID;
                        Flags &= ~TrackFlags.PrefabObject;
                    }
                }
                else
                {
                    ScriptID = Guid.Empty;
                    Flags &= ~TrackFlags.PrefabObject;
                }
            }
        }

        private static Script FindScriptWithPrefabObjectID(Actor actor, ref Guid id)
        {
            if (actor == null)
                return null;

            var scripts = actor.Scripts;
            for (int i = 0; i < scripts.Length; i++)
            {
                var script = scripts[i];
                if (script && script.PrefabObjectID == id)
                    return script;
            }
            for (int i = 0; i < actor.ChildrenCount; i++)
            {
                var e = FindScriptWithPrefabObjectID(actor.GetChild(i), ref id);
                if (e != null)
                    return e;
            }
            return null;
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
