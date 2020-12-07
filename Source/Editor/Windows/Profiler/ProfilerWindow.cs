// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.GUI;
using FlaxEditor.GUI.Tabs;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Windows.Profiler
{
    /// <summary>
    /// Editor tool window for profiling games.
    /// </summary>
    /// <seealso cref="FlaxEditor.Windows.EditorWindow" />
    public sealed class ProfilerWindow : EditorWindow
    {
        private readonly ToolStripButton _liveRecordingButton;
        private readonly ToolStripButton _clearButton;
        private readonly ToolStripButton _prevFrameButton;
        private readonly ToolStripButton _nextFrameButton;
        private readonly ToolStripButton _lastFrameButton;
        private readonly ToolStripButton _showOnlyLastUpdateEventsButton;
        private readonly Tabs _tabs;
        private int _frameIndex = -1;
        private int _framesCount;
        private bool _showOnlyLastUpdateEvents = true;

        /// <summary>
        /// Gets or sets a value indicating whether live events recording is enabled.
        /// </summary>
        public bool LiveRecording
        {
            get => _liveRecordingButton.Checked;
            set
            {
                if (value != LiveRecording)
                {
                    _liveRecordingButton.Checked = value;
                }
            }
        }

        /// <summary>
        /// Gets or sets the index of the selected frame to view (note: some view modes may not use it).
        /// </summary>
        public int ViewFrameIndex
        {
            get => _frameIndex;
            set
            {
                value = Mathf.Clamp(value, -1, _framesCount - 1);
                if (_frameIndex != value)
                {
                    _frameIndex = value;

                    UpdateButtons();
                    if (_tabs.SelectedTab is ProfilerMode mode)
                        mode.UpdateView(_frameIndex, _showOnlyLastUpdateEvents);
                }
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether show only last update events and hide events from the other callbacks (e.g. draw or fixed update).
        /// </summary>
        public bool ShowOnlyLastUpdateEvents
        {
            get => _showOnlyLastUpdateEvents;
            set
            {
                if (_showOnlyLastUpdateEvents != value)
                {
                    _showOnlyLastUpdateEvents = value;

                    UpdateButtons();
                    if (_tabs.SelectedTab is ProfilerMode mode)
                        mode.UpdateView(_frameIndex, _showOnlyLastUpdateEvents);
                }
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ProfilerWindow"/> class.
        /// </summary>
        /// <param name="editor">The editor.</param>
        public ProfilerWindow(Editor editor)
        : base(editor, true, ScrollBars.None)
        {
            Title = "Profiler";

            var toolstrip = new ToolStrip
            {
                Offsets = new Margin(0, 0, 0, 32),
                Parent = this,
            };
            _liveRecordingButton = toolstrip.AddButton(editor.Icons.Play32);
            _liveRecordingButton.LinkTooltip("Live profiling events recording");
            _liveRecordingButton.AutoCheck = true;
            _clearButton = toolstrip.AddButton(editor.Icons.Rotate32, Clear);
            _clearButton.LinkTooltip("Clear data");
            toolstrip.AddSeparator();
            _prevFrameButton = toolstrip.AddButton(editor.Icons.ArrowLeft32, () => ViewFrameIndex--);
            _prevFrameButton.LinkTooltip("Previous frame");
            _nextFrameButton = toolstrip.AddButton(editor.Icons.ArrowRight32, () => ViewFrameIndex++);
            _nextFrameButton.LinkTooltip("Next frame");
            _lastFrameButton = toolstrip.AddButton(editor.Icons.Step32, () => ViewFrameIndex = -1);
            _lastFrameButton.LinkTooltip("Current frame");
            toolstrip.AddSeparator();
            _showOnlyLastUpdateEventsButton = toolstrip.AddButton(editor.Icons.PageScale32, () => ShowOnlyLastUpdateEvents = !ShowOnlyLastUpdateEvents);
            _showOnlyLastUpdateEventsButton.LinkTooltip("Show only last update events and hide events from the other callbacks (e.g. draw or fixed update)");

            _tabs = new Tabs
            {
                Orientation = Orientation.Vertical,
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = new Margin(0, 0, toolstrip.Bottom, 0),
                TabsSize = new Vector2(120, 32),
                Parent = this
            };
            _tabs.SelectedTabChanged += OnSelectedTabChanged;
        }

        /// <summary>
        /// Adds the mode.
        /// </summary>
        /// <remarks>
        /// To remove the mode simply call <see cref="Control.Dispose"/> on mode.
        /// </remarks>
        /// <param name="mode">The mode.</param>
        public void AddMode(ProfilerMode mode)
        {
            if (mode == null)
                throw new ArgumentNullException();
            mode.Init();
            _tabs.AddTab(mode);
            mode.SelectedSampleChanged += ModeOnSelectedSampleChanged;
        }

        private void ModeOnSelectedSampleChanged(int frameIndex)
        {
            ViewFrameIndex = frameIndex;
        }

        /// <summary>
        /// Clears data.
        /// </summary>
        public void Clear()
        {
            _frameIndex = -1;
            _framesCount = 0;
            for (int i = 0; i < _tabs.ChildrenCount; i++)
            {
                if (_tabs.Children[i] is ProfilerMode mode)
                {
                    mode.Clear();
                    mode.UpdateView(ViewFrameIndex, _showOnlyLastUpdateEvents);
                }
            }

            UpdateButtons();
        }

        private void OnSelectedTabChanged(Tabs tabs)
        {
            if (tabs.SelectedTab is ProfilerMode mode)
                mode.UpdateView(ViewFrameIndex, _showOnlyLastUpdateEvents);
        }

        private void UpdateButtons()
        {
            _clearButton.Enabled = _framesCount > 0;
            _prevFrameButton.Enabled = _frameIndex > 0;
            _nextFrameButton.Enabled = (_framesCount - _frameIndex - 1) > 0;
            _lastFrameButton.Enabled = _framesCount > 0;
            _showOnlyLastUpdateEventsButton.Checked = _showOnlyLastUpdateEvents;
        }

        /// <inheritdoc />
        public override void OnInit()
        {
            // Create default modes
            AddMode(new Overall());
            AddMode(new CPU());
            AddMode(new GPU());
            AddMode(new Memory());

            // Init view
            _frameIndex = -1;
            for (int i = 0; i < _tabs.ChildrenCount; i++)
            {
                if (_tabs.Children[i] is ProfilerMode mode)
                    mode.UpdateView(ViewFrameIndex, _showOnlyLastUpdateEvents);
            }

            UpdateButtons();
        }

        /// <inheritdoc />
        public override void OnUpdate()
        {
            if (LiveRecording)
            {
                FlaxEngine.Profiler.BeginEvent("ProfilerWindow.OnUpdate");

                ProfilerMode.SharedUpdateData sharedData = new ProfilerMode.SharedUpdateData();
                sharedData.Begin();
                for (int i = 0; i < _tabs.ChildrenCount; i++)
                {
                    if (_tabs.Children[i] is ProfilerMode mode)
                        mode.Update(ref sharedData);
                }
                sharedData.End();

                _framesCount = Mathf.Min(_framesCount + 1, ProfilerMode.MaxSamples);
                UpdateButtons();

                FlaxEngine.Profiler.EndEvent();
            }
        }

        /// <inheritdoc />
        public override bool OnKeyDown(KeyboardKeys key)
        {
            if (base.OnKeyDown(key))
                return true;

            switch (key)
            {
            case KeyboardKeys.ArrowLeft:
                ViewFrameIndex--;
                return true;
            case KeyboardKeys.ArrowRight:
                ViewFrameIndex++;
                return true;
            }

            return false;
        }
    }
}
