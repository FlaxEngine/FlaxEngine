// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using System.IO;
using System.Linq;
using FlaxEditor.CustomEditors;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.Scripting;
using FlaxEditor.Utilities;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEngine.Utilities;
using Object = FlaxEngine.Object;

namespace FlaxEditor.GUI.Timeline.Tracks
{
    /// <summary>
    /// The timeline media for <see cref="AnimEvent"/> and <see cref="AnimContinuousEvent"/>.
    /// </summary>
    public sealed class AnimationEventMedia : Media
    {
        private sealed class ProxyEditor : SyncPointEditor
        {
            /// <inheritdoc />
            public override IEnumerable<object> UndoObjects => Values;

            /// <inheritdoc />
            public override void Initialize(LayoutElementsContainer layout)
            {
                base.Initialize(layout);

                var instance = (AnimEvent)Values[0];
                if (instance == null)
                    return;
                var scriptType = TypeUtils.GetObjectType(instance);
                var editor = CustomEditorsUtil.CreateEditor(scriptType, false);
                layout.Object(Values, editor);
            }
        }

        private sealed class Proxy : ProxyBase<AnimationEventTrack, AnimationEventMedia>
        {
            [EditorDisplay("General", EditorDisplayAttribute.InlineStyle), CustomEditor(typeof(ProxyEditor))]
            public AnimEvent Event
            {
                get => Media.Instance;
                set => Media.Instance = value;
            }

            public Proxy(AnimationEventTrack track, AnimationEventMedia media)
            : base(track, media)
            {
            }
        }

        private bool _isRegisteredForScriptsReload;
        private string _instanceTypeName;
        private byte[] _instanceData;

        /// <summary>
        /// The event type.
        /// </summary>
        public ScriptType Type;

        /// <summary>
        /// The event instance.
        /// </summary>
        public AnimEvent Instance;

        /// <summary>
        /// True if event is continuous (with duration), not a single frame.
        /// </summary>
        public bool IsContinuous;

        /// <summary>
        /// Initializes a new instance of the <see cref="AnimationEventMedia"/> class.
        /// </summary>
        public AnimationEventMedia()
        {
            PropertiesEditObject = new Proxy(null, this);
        }

        private void OnScriptsReloadBegin()
        {
            if (Instance)
            {
                _instanceTypeName = Type.TypeName;
                Type = ScriptType.Null;
                _instanceData = FlaxEngine.Json.JsonSerializer.SaveToBytes(Instance);
                Object.Destroy(ref Instance);
                ScriptsBuilder.ScriptsReloadEnd += OnScriptsReloadEnd;
            }
        }

        private void OnScriptsReloadEnd()
        {
            ScriptsBuilder.ScriptsReloadEnd -= OnScriptsReloadEnd;
            Type = TypeUtils.GetType(_instanceTypeName);
            if (Type == ScriptType.Null)
            {
                Editor.LogError("Missing anim event type " + _instanceTypeName);
                InitMissing();
                return;
            }
            Instance = (AnimEvent)Type.CreateInstance();
            FlaxEngine.Json.JsonSerializer.LoadFromBytes(Instance, _instanceData, Globals.EngineBuildNumber);
            _instanceData = null;
        }

        /// <summary>
        /// Initializes track for the specified type.
        /// </summary>
        /// <param name="type">The type.</param>
        public void Init(ScriptType type)
        {
            Type = type;
            IsContinuous = new ScriptType(typeof(AnimContinuousEvent)).IsAssignableFrom(type);
            CanDelete = true;
            CanSplit = IsContinuous;
            CanResize = IsContinuous;
            TooltipText = Surface.SurfaceUtils.GetVisualScriptTypeDescription(type);
            Instance = (AnimEvent)type.CreateInstance();
            BackgroundColor = Instance.Color;
            if (!_isRegisteredForScriptsReload)
            {
                _isRegisteredForScriptsReload = true;
                ScriptsBuilder.ScriptsReloadBegin += OnScriptsReloadBegin;
            }
            Type.TrackLifetime(OnTypeDisposing);
        }

        private void OnTypeDisposing(ScriptType type)
        {
            if (Type == type && !IsDisposing)
            {
                // Turn into missing script
                OnScriptsReloadBegin();
                ScriptsBuilder.ScriptsReloadEnd -= OnScriptsReloadEnd;
                InitMissing();
            }
        }

        private void InitMissing()
        {
            CanDelete = true;
            CanSplit = false;
            CanResize = false;
            TooltipText = $"Missing Anim Event Type '{_instanceTypeName}'";
            BackgroundColor = Color.Red;
            Type = ScriptType.Null;
            Instance = null;
        }

        internal void InitMissing(string typeName, byte[] data)
        {
            IsContinuous = false;
            _instanceTypeName = typeName;
            _instanceData = data;
            InitMissing();
        }

        internal void Load(BinaryReader stream)
        {
            StartFrame = (int)stream.ReadSingle();
            DurationFrames = (int)stream.ReadSingle();
            var typeName = stream.ReadStrAnsi(13);
            var type = TypeUtils.GetType(typeName);
            if (type == ScriptType.Null)
            {
                InitMissing(typeName, stream.ReadJsonBytes());
                return;
            }
            Init(type);
            stream.ReadJson(Instance);
        }

        internal void Save(BinaryWriter stream)
        {
            stream.Write((float)StartFrame);
            stream.Write((float)DurationFrames);
            if (Type != ScriptType.Null)
            {
                stream.WriteStrAnsi(Type.TypeName, 13);
                stream.WriteJson(Instance);
            }
            else
            {
                // Missing
                stream.WriteStrAnsi(_instanceTypeName, 13);
                stream.WriteJsonBytes(_instanceData);
            }
        }

        /// <inheritdoc />
        protected override void OnDurationFramesChanged()
        {
            if (Type != ScriptType.Null)
                DurationFrames = IsContinuous ? Mathf.Max(DurationFrames, 2) : 1;

            base.OnDurationFramesChanged();
        }

        /// <inheritdoc />
        public override bool ContainsPoint(ref Float2 location, bool precise = false)
        {
            if (Timeline.Zoom > 0.5f && !IsContinuous)
            {
                // Hit-test dot
                var size = Height - 2.0f;
                var rect = new Rectangle(new Float2(size * -0.5f) + Size * 0.5f, new Float2(size));
                return rect.Contains(ref location);
            }

            return base.ContainsPoint(ref location, precise);
        }

        /// <inheritdoc />
        public override void Draw()
        {
            if (Timeline.Zoom > 0.5f && !IsContinuous)
            {
                // Draw more visible dot for the event that maintains size even when zooming out
                var style = Style.Current;
                var icon = Editor.Instance.Icons.VisjectBoxClosed32;
                var size = Height - 2.0f;
                var rect = new Rectangle(new Float2(size * -0.5f) + Size * 0.5f, new Float2(size));
                var outline = Color.Black; // Shadow
                if (_isMoving)
                    outline = style.SelectionBorder;
                else if (IsMouseOver)
                    outline = style.BorderHighlighted;
                else if (Timeline.SelectedMedia.Contains(this))
                    outline = style.BackgroundSelected;
                Render2D.DrawSprite(icon, rect.MakeExpanded(6.0f), outline);
                Render2D.DrawSprite(icon, rect, BackgroundColor);

                DrawChildren();
            }
            else
            {
                // Default drawing
                base.Draw();
            }
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            _instanceData = null;
            _instanceTypeName = null;
            Type = ScriptType.Null;
            Object.Destroy(ref Instance);
            if (_isRegisteredForScriptsReload)
            {
                _isRegisteredForScriptsReload = false;
                ScriptsBuilder.ScriptsReloadBegin -= OnScriptsReloadBegin;
            }

            base.OnDestroy();
        }
    }

    /// <summary>
    /// The timeline track for <see cref="AnimEvent"/> and <see cref="AnimContinuousEvent"/>.
    /// </summary>
    public sealed class AnimationEventTrack : Track
    {
        /// <summary>
        /// Gets the archetype.
        /// </summary>
        /// <returns>The archetype.</returns>
        public static TrackArchetype GetArchetype()
        {
            return new TrackArchetype
            {
                TypeId = 19,
                Name = "Animation Event",
                Create = options => new AnimationEventTrack(ref options),
                Load = LoadTrack,
                Save = SaveTrack,
            };
        }

        private static void LoadTrack(int version, Track track, BinaryReader stream)
        {
            var e = (AnimationEventTrack)track;
            var count = stream.ReadInt32();
            while (e.Media.Count > count)
                e.RemoveMedia(e.Media.Last());
            while (e.Media.Count < count)
                e.AddMedia(new AnimationEventMedia());
            for (int i = 0; i < count; i++)
            {
                var m = (AnimationEventMedia)e.Media[i];
                m.Load(stream);
            }
        }

        private static void SaveTrack(Track track, BinaryWriter stream)
        {
            var e = (AnimationEventTrack)track;
            var count = e.Media.Count;
            stream.Write(count);
            for (int i = 0; i < count; i++)
            {
                var m = (AnimationEventMedia)e.Media[i];
                m.Save(stream);
            }
        }

        /// <inheritdoc />
        public AnimationEventTrack(ref TrackCreateOptions options)
        : base(ref options)
        {
            // Add button
            const float buttonSize = 14;
            var addButton = new Button
            {
                Text = "+",
                TooltipText = "Add events",
                AutoFocus = true,
                AnchorPreset = AnchorPresets.MiddleRight,
                IsScrollable = false,
                Offsets = new Margin(-buttonSize - 2 + _muteCheckbox.Offsets.Left, buttonSize, buttonSize * -0.5f, buttonSize),
                Parent = this,
            };
            addButton.ButtonClicked += OnAddButtonClicked;
        }

        private void OnAddButtonClicked(Button button)
        {
            var cm = new ContextMenu.ContextMenu();
            OnContextMenu(cm);
            cm.Show(button.Parent, button.BottomLeft);
        }

        /// <inheritdoc />
        public override void OnTimelineContextMenu(ContextMenu.ContextMenu menu, float time)
        {
            base.OnTimelineContextMenu(menu, time);

            AddContextMenu(menu, time);
        }

        /// <inheritdoc />
        protected override void OnContextMenu(ContextMenu.ContextMenu menu)
        {
            base.OnContextMenu(menu);

            menu.AddSeparator();
            AddContextMenu(menu, 0.0f);
        }

        private void AddContextMenu(ContextMenu.ContextMenu menu, float time)
        {
            var addEvent = menu.AddChildMenu("Add Anim Event");
            var addContinuousEvent = menu.AddChildMenu("Add Anim Continuous Event");
            var animEventTypes = Editor.Instance.CodeEditing.All.Get().Where(x => new ScriptType(typeof(AnimEvent)).IsAssignableFrom(x));
            foreach (var type in animEventTypes)
            {
                if (type.IsAbstract || !type.CanCreateInstance || type.HasAttribute(typeof(HideInEditorAttribute), true))
                    continue;
                var add = new ScriptType(typeof(AnimContinuousEvent)).IsAssignableFrom(type) ? addContinuousEvent : addEvent;
                var b = add.ContextMenu.AddButton(type.Name);
                b.TooltipText = Surface.SurfaceUtils.GetVisualScriptTypeDescription(type);
                b.Tag = type;
                b.Parent.Tag = time;
                b.ButtonClicked += OnAddAnimEvent;
            }
            if (!addEvent.ContextMenu.Items.Any())
                addEvent.ContextMenu.AddButton("No Anim Events found").CloseMenuOnClick = false;
            if (!addContinuousEvent.ContextMenu.Items.Any())
                addContinuousEvent.ContextMenu.AddButton("No Continuous Anim Events found").CloseMenuOnClick = false;
        }


        private void OnAddAnimEvent(ContextMenuButton button)
        {
            var type = (ScriptType)button.Tag;
            var time = (float)button.Parent.Tag;
            var media = new AnimationEventMedia();
            media.Init(type);
            media.StartFrame = (int)(time * Timeline.FramesPerSecond);
            media.DurationFrames = media.IsContinuous ? Mathf.Max(Timeline.DurationFrames / 10, 10) : 1;
            Timeline.AddMedia(this, media);
        }
    }
}
