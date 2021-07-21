// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.IO;
using System.Linq;
using FlaxEditor.CustomEditors;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.GUI.Timeline.Undo;
using FlaxEditor.SceneGraph;
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
            e.ActorID = new Guid(stream.ReadBytes(16));
        }

        private static void SaveTrack(Track track, BinaryWriter stream)
        {
            var e = (ActorTrack)track;
            stream.Write(e.ActorID.ToByteArray());
        }

        /// <summary>
        /// The select actor icon.
        /// </summary>
        protected ClickableImage _selectActor;

        /// <summary>
        /// The object ID.
        /// </summary>
        public Guid ActorID;

        /// <summary>
        /// Gets the object instance (it might be missing).
        /// </summary>
        public Actor Actor
        {
            get => FlaxEngine.Object.TryFind<Actor>(ref ActorID);
            set => ActorID = value?.ID ?? Guid.Empty;
        }

        /// <inheritdoc />
        public ActorTrack(ref TrackCreateOptions options)
        : base(ref options)
        {
            // Select Actor button
            const float buttonSize = 18;
            var icons = Editor.Instance.Icons;
            _selectActor = new ClickableImage
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

            var actor = Actor;
            var selection = Editor.Instance.SceneEditing.Selection;

            // Missing actor case
            if (actor == null)
            {
                if (selection.Count == 1 && selection[0] is ActorNode actorNode && actorNode.Actor && IsActorValid(actorNode.Actor))
                {
                    menu.AddButton("Select " + actorNode.Actor, OnClickedSelectActor);
                }
                else
                {
                    menu.AddButton("No valid actor selected");
                }
                return;
            }
            else if (selection.Count == 1)
            {
                // TODO: add option to change the actor to the selected one
            }

            var type = actor.GetType();
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
                if (SubTracks.Any(x => x is IObjectTrack y && y.Object == script))
                    continue;

                var name = CustomEditorsUtil.GetPropertyNameUI(script.GetType().Name);
                menu.AddButton(name, OnAddScriptTrack).Tag = script;
            }
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
            return true;
        }

        private void OnAddScriptTrack(ContextMenuButton button)
        {
            var script = (Script)button.Tag;
            AddScriptTrack(script);
        }

        private void OnClickedSelectActor()
        {
            var selection = Editor.Instance.SceneEditing.Selection;
            if (selection.Count == 1 && selection[0] is ActorNode actorNode && actorNode.Actor && IsActorValid(actorNode.Actor))
            {
                var oldName = Name;
                Rename(actorNode.Actor.Name);
                using (new TrackUndoBlock(this, new RenameTrackAction(Timeline, this, oldName, Name)))
                    Actor = actorNode.Actor;
            }
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
