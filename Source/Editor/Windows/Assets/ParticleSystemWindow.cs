// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Linq;
using System.Xml;
using FlaxEditor.Content;
using FlaxEditor.CustomEditors;
using FlaxEditor.CustomEditors.Editors;
using FlaxEditor.CustomEditors.GUI;
using FlaxEditor.GUI;
using FlaxEditor.GUI.Timeline;
using FlaxEditor.GUI.Timeline.Tracks;
using FlaxEditor.Surface;
using FlaxEditor.Viewport.Previews;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Windows.Assets
{
    /// <summary>
    /// Particle System window allows to view and edit <see cref="ParticleSystem"/> asset.
    /// Note: it uses ClonedAssetEditorWindowBase which is creating cloned asset to edit/preview.
    /// </summary>
    /// <seealso cref="ParticleSystem" />
    /// <seealso cref="FlaxEditor.Windows.Assets.AssetEditorWindow" />
    public sealed class ParticleSystemWindow : ClonedAssetEditorWindowBase<ParticleSystem>
    {
        sealed class EditParameterOverrideAction : IUndoAction
        {
            private ParticleSystemWindow _window;
            private string _trackName;
            private Guid _paramId;
            private bool _beforeOverride, _afterOverride;
            private object _beforeValue, _afterValue;

            public string ActionString => "Edit Parameter Override";

            public EditParameterOverrideAction(ParticleSystemWindow window, ParticleEmitterTrack track, GraphParameter parameter, bool newOverride)
            {
                _window = window;
                _trackName = track.Name;
                _paramId = parameter.Identifier;
                _beforeOverride = !newOverride;
                _afterOverride = newOverride;
                _beforeValue = _afterValue = parameter.Value;
            }

            public EditParameterOverrideAction(ParticleSystemWindow window, ParticleEmitterTrack track, GraphParameter parameter, object newValue)
            {
                _window = window;
                _trackName = track.Name;
                _paramId = parameter.Identifier;
                _beforeOverride = true;
                _afterOverride = true;
                _beforeValue = parameter.Value;
                _afterValue = newValue;
            }

            private void Set(bool isOverride, object value)
            {
                var track = (ParticleEmitterTrack)_window.Timeline.FindTrack(_trackName);
                if (track == null)
                    throw new Exception($"Missing track of name {_trackName} in particle system {_window.Title}");
                if (_beforeOverride != _afterOverride)
                {
                    if (isOverride)
                        track.ParametersOverrides.Add(_paramId, value);
                    else
                        track.ParametersOverrides.Remove(_paramId);
                }
                else
                {
                    _window._isEditingInstancedParameterValue = true;
                    var param = _window.Preview.PreviewActor.GetParameter(_trackName, _paramId);
                    if (param != null)
                        param.Value = value;

                    if (track.ParametersOverrides.ContainsKey(_paramId))
                        track.ParametersOverrides[_paramId] = value;
                }
                _window.Timeline.OnEmittersParametersOverridesEdited();
                _window.Timeline.MarkAsEdited();
            }

            public void Do()
            {
                Set(_afterOverride, _afterValue);
            }

            public void Undo()
            {
                Set(_beforeOverride, _beforeValue);
            }

            public void Dispose()
            {
                _window = null;
                _trackName = null;
                _beforeValue = _afterValue = null;
            }
        }

        /// <summary>
        /// The proxy object for editing particle system properties.
        /// </summary>
        private class GeneralProxy
        {
            private readonly ParticleSystemWindow _window;

            [EditorDisplay("Particle System"), EditorOrder(-100), Limit(1), Tooltip("The timeline animation duration in frames.")]
            public int TimelineDurationFrames
            {
                get => _window.Timeline.DurationFrames;
                set => _window.Timeline.DurationFrames = value;
            }

            public GeneralProxy(ParticleSystemWindow window)
            {
                _window = window;
            }
        }

        /// <summary>
        /// The proxy object for editing particle system track properties.
        /// </summary>
        [CustomEditor(typeof(EmitterTrackProxyEditor))]
        private class EmitterTrackProxy : GeneralProxy
        {
            private readonly ParticleSystemWindow _window;
            private readonly ParticleEffect _effect;
            private readonly int _emitterIndex;
            private readonly ParticleEmitterTrack _track;

            [EditorDisplay("Particle Emitter"), EditorOrder(0), Tooltip("The name text.")]
            public string Name
            {
                get => _track.Name;
                set
                {
                    if (!_track.Timeline.IsTrackNameValid(value))
                    {
                        MessageBox.Show("Invalid name. It must be unique.", "Invalid track name", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                        return;
                    }

                    _track.Name = value;
                }
            }

            [EditorDisplay("Particle Emitter"), EditorOrder(100), Tooltip("The particle emitter to use for the track media event playback.")]
            public ParticleEmitter Emitter
            {
                get => _track.Asset;
                set => _track.Asset = value;
            }

            private bool HasEmitter => _track.Asset != null;

            [EditorDisplay("Particle Emitter"), VisibleIf(nameof(HasEmitter)), EditorOrder(200), Tooltip("The start frame of the media event.")]
            public int StartFrame
            {
                get => _track.Media.Count > 0 ? _track.TrackMedia.StartFrame : 0;
                set => _track.TrackMedia.StartFrame = value;
            }

            [EditorDisplay("Particle Emitter"), Limit(1), VisibleIf(nameof(HasEmitter)), EditorOrder(300), Tooltip("The total duration of the media event in the timeline sequence frames amount.")]
            public int DurationFrames
            {
                get => _track.Media.Count > 0 ? _track.TrackMedia.DurationFrames : 0;
                set => _track.TrackMedia.DurationFrames = value;
            }

            public EmitterTrackProxy(ParticleSystemWindow window, ParticleEffect effect, ParticleEmitterTrack track, int emitterIndex)
            : base(window)
            {
                _window = window;
                _effect = effect;
                _emitterIndex = emitterIndex;
                _track = track;
            }

            private sealed class EmitterTrackProxyEditor : GenericEditor
            {
                /// <inheritdoc />
                public override void Initialize(LayoutElementsContainer layout)
                {
                    base.Initialize(layout);

                    var emitterTrack = Values[0] as EmitterTrackProxy;
                    if (emitterTrack?._effect == null || emitterTrack?._effect.Parameters == null)
                        return;

                    var group = layout.Group("Parameters");
                    var parameters = emitterTrack._effect.Parameters.Where(x => x.EmitterIndex == emitterTrack._emitterIndex && x.Emitter == emitterTrack.Emitter && x.IsPublic).ToArray();

                    if (!parameters.Any())
                    {
                        group.Label("No parameters", TextAlignment.Center);
                        return;
                    }

                    var data = SurfaceUtils.InitGraphParameters(parameters);
                    SurfaceUtils.DisplayGraphParameters(group, data,
                                                        (instance, parameter, tag) => ((ParticleEffectParameter)tag).Value,
                                                        (instance, value, parameter, tag) =>
                                                        {
                                                            if (parameter.Value == value)
                                                                return;

                                                            var proxy = (EmitterTrackProxy)instance;
                                                            var action = new EditParameterOverrideAction(proxy._window, proxy._track, parameter, value);
                                                            action.Do();
                                                            proxy._window.Undo.AddAction(action);
                                                        },
                                                        Values,
                                                        (instance, parameter, tag) => ((ParticleEffectParameter)tag).DefaultEmitterValue,
                                                        (LayoutElementsContainer itemLayout, ValueContainer valueContainer, ref SurfaceUtils.GraphParameterData e) =>
                                                        {
                                                            // Use label with parameter value override checkbox
                                                            var label = new CheckablePropertyNameLabel(e.Parameter.Name);
                                                            label.CheckBox.Checked = emitterTrack._track.ParametersOverrides.ContainsKey(e.Parameter.Identifier);
                                                            label.CheckBox.Tag = e.Parameter;
                                                            label.CheckChanged += nameLabel =>
                                                            {
                                                                var parameter = (GraphParameter)nameLabel.CheckBox.Tag;
                                                                var proxy = emitterTrack;
                                                                var action = new EditParameterOverrideAction(proxy._window, proxy._track, parameter, nameLabel.CheckBox.Checked);
                                                                action.Do();
                                                                proxy._window.Undo.AddAction(action);
                                                            };

                                                            itemLayout.Property(label, valueContainer, null, e.Tooltip?.Text);
                                                            label.UpdateStyle();
                                                        });
                }
            }
        }

        /// <summary>
        /// The proxy object for editing folder track properties.
        /// </summary>
        private class FolderTrackProxy : GeneralProxy
        {
            private readonly FolderTrack _track;

            [EditorDisplay("Folder"), EditorOrder(0), Tooltip("The name text.")]
            public string Name
            {
                get => _track.Name;
                set
                {
                    if (!_track.Timeline.IsTrackNameValid(value))
                    {
                        MessageBox.Show("Invalid name. It must be unique.", "Invalid track name", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                        return;
                    }

                    _track.Name = value;
                }
            }

            [EditorDisplay("Folder"), EditorOrder(200), Tooltip("The folder icon color.")]
            public Color Color
            {
                get => _track.IconColor;
                set => _track.IconColor = value;
            }

            public FolderTrackProxy(ParticleSystemWindow window, FolderTrack track)
            : base(window)
            {
                _track = track;
            }
        }

        private readonly SplitPanel _split1;
        private readonly SplitPanel _split2;
        private ParticleSystemTimeline _timeline;
        private readonly ParticleSystemPreview _preview;
        private readonly CustomEditorPresenter _propertiesEditor;
        private ToolStripButton _saveButton;
        private ToolStripButton _undoButton;
        private ToolStripButton _redoButton;
        private Undo _undo;
        private bool _tmpParticleSystemIsDirty;
        private bool _isWaitingForTimelineLoad;
        private bool _isEditingInstancedParameterValue;
        private uint _parametersVersion;

        /// <summary>
        /// Gets the particle system preview.
        /// </summary>
        public ParticleSystemPreview Preview => _preview;

        /// <summary>
        /// Gets the timeline editor.
        /// </summary>
        public ParticleSystemTimeline Timeline => _timeline;

        /// <summary>
        /// Gets the undo history context for this window.
        /// </summary>
        public Undo Undo => _undo;

        /// <inheritdoc />
        public ParticleSystemWindow(Editor editor, AssetItem item)
        : base(editor, item)
        {
            var inputOptions = Editor.Options.Options.Input;

            // Undo
            _undo = new Undo();
            _undo.UndoDone += OnUndoRedo;
            _undo.RedoDone += OnUndoRedo;
            _undo.ActionDone += OnUndoRedo;

            // Split Panel 1
            _split1 = new SplitPanel(Orientation.Vertical, ScrollBars.None, ScrollBars.None)
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = new Margin(0, 0, _toolstrip.Bottom, 0),
                SplitterValue = 0.6f,
                Parent = this
            };

            // Split Panel 2
            _split2 = new SplitPanel(Orientation.Horizontal, ScrollBars.None, ScrollBars.Vertical)
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = Margin.Zero,
                SplitterValue = 0.5f,
                Parent = _split1.Panel1
            };

            // Particles preview
            _preview = new ParticleSystemPreview(true)
            {
                PlaySimulation = true,
                Parent = _split2.Panel1
            };

            // Timeline
            _timeline = new ParticleSystemTimeline(_preview, _undo)
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = Margin.Zero,
                Parent = _split1.Panel2,
                Enabled = false
            };
            _timeline.Modified += OnTimelineModified;
            _timeline.SelectionChanged += OnTimelineSelectionChanged;
            _timeline.SetNoTracksText("Loading...");

            // Properties editor
            var propertiesEditor = new CustomEditorPresenter(_undo, string.Empty);
            propertiesEditor.Panel.Parent = _split2.Panel2;
            propertiesEditor.Modified += OnParticleSystemPropertyEdited;
            _propertiesEditor = propertiesEditor;
            propertiesEditor.Select(new GeneralProxy(this));

            // Toolstrip
            _saveButton = _toolstrip.AddButton(Editor.Icons.Save64, Save).LinkTooltip("Save", ref inputOptions.Save);
            _toolstrip.AddSeparator();
            _undoButton = _toolstrip.AddButton(Editor.Icons.Undo64, _undo.PerformUndo).LinkTooltip("Undo", ref inputOptions.Undo);
            _redoButton = _toolstrip.AddButton(Editor.Icons.Redo64, _undo.PerformRedo).LinkTooltip("Redo", ref inputOptions.Redo);
            _toolstrip.AddSeparator();
            _toolstrip.AddButton(editor.Icons.Docs64, () => Platform.OpenUrl(Utilities.Constants.DocsUrl + "manual/particles/index.html")).LinkTooltip("See documentation to learn more");

            // Setup input actions
            InputActions.Add(options => options.Undo, _undo.PerformUndo);
            InputActions.Add(options => options.Redo, _undo.PerformRedo);
        }

        private void OnUndoRedo(IUndoAction action)
        {
            MarkAsEdited();
            UpdateToolstrip();

            if (!_isEditingInstancedParameterValue)
            {
                _propertiesEditor.BuildLayoutOnUpdate();
            }
        }

        private void OnTimelineModified()
        {
            if (!_isEditingInstancedParameterValue)
            {
                _tmpParticleSystemIsDirty = true;
            }

            MarkAsEdited();
        }

        private void OnTimelineSelectionChanged()
        {
            if (_timeline.SelectedTracks.Count == 0)
            {
                _propertiesEditor.Select(new GeneralProxy(this));
                return;
            }

            var tracks = new object[_timeline.SelectedTracks.Count];
            var emitterTracks = _timeline.Tracks.Where(track => track is ParticleEmitterTrack).Cast<ParticleEmitterTrack>().ToList();
            for (var i = 0; i < _timeline.SelectedTracks.Count; i++)
            {
                var track = _timeline.SelectedTracks[i];
                if (track is ParticleEmitterTrack particleEmitterTrack)
                {
                    tracks[i] = new EmitterTrackProxy(this, Preview.PreviewActor, particleEmitterTrack, emitterTracks.IndexOf(particleEmitterTrack));
                }
                else if (track is FolderTrack folderTrack)
                {
                    tracks[i] = new FolderTrackProxy(this, folderTrack);
                }
                else
                {
                    throw new NotImplementedException("Invalid track type.");
                }
            }
            _propertiesEditor.Select(tracks);
        }

        private void OnParticleSystemPropertyEdited()
        {
            if (_isEditingInstancedParameterValue)
                return;

            _timeline.MarkAsEdited();
        }

        /// <summary>
        /// Refreshes temporary asset to see changes live when editing the timeline.
        /// </summary>
        /// <returns>True if cannot refresh it, otherwise false.</returns>
        public bool RefreshTempAsset()
        {
            if (_asset == null || _isWaitingForTimelineLoad)
                return true;

            if (_timeline.IsModified)
            {
                _propertiesEditor.BuildLayoutOnUpdate();
                _timeline.Save(_asset);
            }

            return false;
        }

        /// <inheritdoc />
        public override void Save()
        {
            if (!IsEdited)
                return;

            if (RefreshTempAsset())
            {
                return;
            }

            if (SaveToOriginal())
            {
                return;
            }

            ClearEditedFlag();
            _item.RefreshThumbnail();
        }

        /// <inheritdoc />
        protected override void UpdateToolstrip()
        {
            _saveButton.Enabled = IsEdited;
            _undoButton.Enabled = _undo.CanUndo;
            _redoButton.Enabled = _undo.CanRedo;

            base.UpdateToolstrip();
        }

        /// <inheritdoc />
        protected override void UnlinkItem()
        {
            _propertiesEditor.Deselect();
            _preview.System = null;
            _isWaitingForTimelineLoad = false;

            base.UnlinkItem();
        }

        /// <inheritdoc />
        protected override void OnAssetLinked()
        {
            _preview.System = _asset;
            _isWaitingForTimelineLoad = true;

            base.OnAssetLinked();
        }

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            // Check if need to refresh the parameters
            if (_parametersVersion != _preview.PreviewActor.ParametersVersion)
            {
                _parametersVersion = _preview.PreviewActor.ParametersVersion;
                _propertiesEditor.BuildLayoutOnUpdate();
            }

            base.Update(deltaTime);

            // Check if temporary asset need to be updated
            if (_tmpParticleSystemIsDirty)
            {
                // Clear flag
                _tmpParticleSystemIsDirty = false;

                // Update
                RefreshTempAsset();
            }

            // Check if need to load timeline
            if (_isWaitingForTimelineLoad && _asset.IsLoaded)
            {
                // Clear flag
                _isWaitingForTimelineLoad = false;

                // Load timeline data from the asset
                _timeline.Load(_asset);

                // Setup
                _undo.Clear();
                _timeline.Enabled = true;
                _timeline.SetNoTracksText(null);
                _propertiesEditor.Select(new GeneralProxy(this));
                ClearEditedFlag();
            }

            // Clear flag
            _isEditingInstancedParameterValue = false;
        }

        /// <inheritdoc />
        public override bool UseLayoutData => true;

        /// <inheritdoc />
        public override void OnLayoutSerialize(XmlWriter writer)
        {
            LayoutSerializeSplitter(writer, "Split1", _split1);
            LayoutSerializeSplitter(writer, "Split2", _split2);
            LayoutSerializeSplitter(writer, "Split3", _timeline.Splitter);
        }

        /// <inheritdoc />
        public override void OnLayoutDeserialize(XmlElement node)
        {
            LayoutDeserializeSplitter(node, "Split1", _split1);
            LayoutDeserializeSplitter(node, "Split2", _split2);
            LayoutDeserializeSplitter(node, "Split3", _timeline.Splitter);
        }

        /// <inheritdoc />
        public override void OnLayoutDeserialize()
        {
            _split1.SplitterValue = 0.6f;
            _split2.SplitterValue = 0.5f;
            _timeline.Splitter.SplitterValue = 0.2f;
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            if (_undo != null)
                _undo.Enabled = false;
            _propertiesEditor?.Deselect();
            _undo?.Clear();
            _undo = null;

            _timeline = null;
            _saveButton = null;
            _undoButton = null;
            _redoButton = null;

            base.OnDestroy();
        }
    }
}
