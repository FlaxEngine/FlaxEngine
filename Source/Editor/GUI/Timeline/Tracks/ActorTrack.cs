// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.IO;
using System.Linq;
using FlaxEditor.CustomEditors;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.GUI.Timeline.Undo;
using FlaxEditor.SceneGraph;
using FlaxEditor.Utilities;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI.Timeline.Tracks
{
    /// <summary>
    /// The timeline track for animating <see cref="FlaxEngine.Actor"/> objects.
    /// </summary>
    /// <seealso cref="ObjectTrack" />
    public class ActorTrack : ObjectTrack
    {
        /// <summary>
        /// Gets the archetype.
        /// </summary>
        /// <returns>The archetype.</returns>
        public static TrackArchetype GetArchetype()
        {
            return new TrackArchetype
            {
                TypeId = 7,
                Name = "Actor",
                Create = options => new ActorTrack(ref options),
                Load = LoadTrack,
                Save = SaveTrack,
            };
        }

        private static void LoadTrack(int version, Track track, BinaryReader stream)
        {
            var e = (ActorTrack)track;
            e.ActorID = stream.ReadGuid();
        }

        private static void SaveTrack(Track track, BinaryWriter stream)
        {
            var e = (ActorTrack)track;
            stream.WriteGuid(ref e.ActorID);
        }

        /// <summary>
        /// The select actor icon.
        /// </summary>
        protected Image _selectActor;

        /// <summary>
        /// The object ID.
        /// </summary>
        public Guid ActorID;

        /// <summary>
        /// Gets the object instance (it might be missing).
        /// </summary>
        public Actor Actor
        {
            get
            {
                if (Flags.HasFlag(TrackFlags.PrefabObject))
                {
                    // TODO: reuse cached actor to improve perf
                    foreach (var window in Editor.Instance.Windows.Windows)
                    {
                        if (window is Windows.Assets.PrefabWindow prefabWindow && prefabWindow.Graph.MainActor)
                        {
                            var actor = FindActorWithPrefabObjectID(prefabWindow.Graph.MainActor, ref ActorID);
                            if (actor != null)
                                return actor;
                        }
                    }
                    return null;
                }
                return FlaxEngine.Object.TryFind<Actor>(ref ActorID);
            }
            set
            {
                if (value != null)
                {
                    if (value.HasPrefabLink && value.Scene == null)
                    {
                        // Track with prefab object reference assigned in Editor
                        ActorID = value.PrefabObjectID;
                        Flags |= TrackFlags.PrefabObject;
                    }
                    else
                    {
                        ActorID = value.ID;
                    }
                }
                else
                {
                    ActorID = Guid.Empty;
                }
            }
        }

        private static Actor FindActorWithPrefabObjectID(Actor a, ref Guid id)
        {
            if (a.PrefabObjectID == id)
                return a;
            for (int i = 0; i < a.ChildrenCount; i++)
            {
                var e = FindActorWithPrefabObjectID(a.GetChild(i), ref id);
                if (e != null)
                    return e;
            }
            return null;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ActorTrack"/> class.
        /// </summary>
        /// <param name="options">The track initial options.</param>
        /// <param name="useProxyKeyframes">True if show sub-tracks keyframes as a proxy on this track, otherwise false.</param>
        public ActorTrack(ref TrackCreateOptions options, bool useProxyKeyframes = true)
        : base(ref options, useProxyKeyframes)
        {
            // Select Actor button
            const float buttonSize = 18;
            var icons = Editor.Instance.Icons;
            _selectActor = new Image
            {
                TooltipText = "Selects the actor animated by this track",
                AutoFocus = true,
                AnchorPreset = AnchorPresets.MiddleRight,
                IsScrollable = false,
                Color = Style.Current.ForegroundGrey,
                Margin = new Margin(1),
                Brush = new SpriteBrush(icons.Search32),
                Offsets = new Margin(-buttonSize - 2 + _addButton.Offsets.Left, buttonSize, buttonSize * -0.5f, buttonSize),
                Parent = this,
            };
            _selectActor.Clicked += OnClickedSelectActor;
        }

        /// <inheritdoc />
        public override object Object => Actor;

        /// <inheritdoc />
        protected override void OnShowAddContextMenu(ContextMenu.ContextMenu menu)
        {
            base.OnShowAddContextMenu(menu);

            OnSelectActorContextMenu(menu);

            var actor = Actor;
            if (actor == null)
                return;
            var type = actor.GetType();
            menu.AddSeparator();

            // Properties and events
            if (AddProperties(this, menu, type) != 0)
                menu.AddSeparator();
            if (AddEvents(this, menu, type) != 0)
                menu.AddSeparator();

            // Child scripts
            var scripts = actor.Scripts;
            for (int i = 0; i < scripts.Length; i++)
            {
                var script = scripts[i];

                // Skip invalid or hidden scripts
                if (script == null || script.GetType().GetCustomAttributes(true).Any(x => x is HideInEditorAttribute))
                    continue;

                // Prevent from adding the same track twice
                if (SubTracks.Any(x => x is IObjectTrack y && y.Object as SceneObject == script))
                    continue;

                var name = Utilities.Utils.GetPropertyNameUI(script.GetType().Name);
                menu.AddButton(name, OnAddScriptTrack).Tag = script;
            }
        }

        /// <inheritdoc />
        protected override void OnContextMenu(ContextMenu.ContextMenu menu)
        {
            base.OnContextMenu(menu);

            menu.AddSeparator();
            OnSelectActorContextMenu(menu);
        }

        private void OnSelectActorContextMenu(ContextMenu.ContextMenu menu)
        {
            var actor = Actor;
            var selection = Editor.Instance.SceneEditing.Selection;
            foreach (var node in selection)
            {
                if (node is ActorNode actorNode && IsActorValid(actorNode.Actor) && actorNode.Actor != actor)
                {
                    var b = menu.AddButton("Select " + actorNode.Actor, OnClickedSelectActor);
                    b.Tag = actorNode.Actor;
                    b.TooltipText = Utilities.Utils.GetTooltip(actorNode.Actor);
                }
            }
            menu.AddButton("Retarget...", OnClickedSelect).TooltipText = "Opens actor picker dialog to select the target actor for this track";
        }

        /// <summary>
        /// Adds the script track to this actor track.
        /// </summary>
        /// <param name="script">The script object.</param>
        public void AddScriptTrack(Script script)
        {
            var track = (ScriptTrack)Timeline.NewTrack(ScriptTrack.GetArchetype());
            track.ParentTrack = this;
            track.TrackIndex = TrackIndex + 1;
            track.Script = script;
            track.Rename(script.GetType().Name);
            Timeline.AddTrack(track);

            Expand();
        }

        /// <summary>
        /// Determines whether actor is valid for this track.
        /// </summary>
        /// <param name="actor">The actor.</param>
        /// <returns>True if it's valid, otherwise false.</returns>
        protected virtual bool IsActorValid(Actor actor)
        {
            return actor;
        }

        /// <summary>
        /// Called when actor gets changed.
        /// </summary>
        protected virtual void OnActorChanged()
        {
        }

        private void OnAddScriptTrack(ContextMenuButton button)
        {
            var script = (Script)button.Tag;
            AddScriptTrack(script);
        }

        private void OnClickedSelectActor(ContextMenuButton b)
        {
            SetActor((Actor)b.Tag);
        }

        private void SetActor(Actor actor)
        {
            if (Actor == actor || !IsActorValid(actor))
                return;
            var oldName = Name;
            Rename(actor.Name);
            using (new TrackUndoBlock(this, new RenameTrackAction(Timeline, this, oldName, Name)))
                Actor = actor;
            OnActorChanged();
        }

        private void OnClickedSelect()
        {
            ActorSearchPopup.Show(this, PointFromScreen(FlaxEngine.Input.MouseScreenPosition), IsActorValid, SetActor, null);
        }

        private void OnClickedSelectActor(Image image, MouseButton button)
        {
            if (button == MouseButton.Left)
            {
                var actor = Actor;
                if (actor)
                {
                    Editor.Instance.SceneEditing.Select(actor);
                }
            }
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            _selectActor = null;

            base.OnDestroy();
        }
    }
}
