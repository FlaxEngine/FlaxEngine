// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Diagnostics;
using System.Xml;
using FlaxEditor.GUI.Input;
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
#if USE_PROFILER
        private readonly ToolStripButton _liveRecordingButton;
        private readonly ToolStripButton _clearButton;
        private readonly ToolStripButton _liveDetailsButton;
        private readonly Label _detailsFrameLabel;
        private readonly IntValueBox _refreshRateBox;
        private readonly SamplesBuffer<(ulong CPU, ulong GPU)> _frameIds = new SamplesBuffer<(ulong, ulong)>();
        private readonly ToolStripButton _prevFrameButton;
        private readonly ToolStripButton _nextFrameButton;
        private readonly ToolStripButton _lastFrameButton;
        private readonly ToolStripButton _showOnlyLastUpdateEventsButton;
        private readonly Tabs _tabs;
        private int _frameIndex = -1;
        private int _framesCount;
        private bool _showOnlyLastUpdateEvents;
        private ulong _lastRecordedFrame = ulong.MaxValue;
        private long _nextDetailsRefresh;
        private long _lastManagedMemory = 0;
        private long _lastManagedMemoryProfiler = 0;

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
                    OnLiveRecordingChanged();
                }
            }
        }

        /// <summary>Gets or sets the live details refresh rate in Hz. Frame recording is independent.</summary>
        public int RefreshRateHz
        {
            get => _refreshRateBox.Value;
            set { _refreshRateBox.Value = Mathf.Clamp(value, 1, 120); _nextDetailsRefresh = 0; }
        }

        /// <summary>
        /// Gets or sets the index of the selected frame to view (note: some view modes may not use it). -1 for the last frame.
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
                    UpdateView();
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
                    UpdateView();
                }
            }
        }
#endif

        /// <summary>
        /// Initializes a new instance of the <see cref="ProfilerWindow"/> class.
        /// </summary>
        /// <param name="editor">The editor.</param>
        public ProfilerWindow(Editor editor)
        : base(editor, true, ScrollBars.None)
        {
            Title = "Profiler";

#if USE_PROFILER
            var toolstrip = new ToolStrip
            {
                Parent = this,
            };
            _liveRecordingButton = toolstrip.AddButton(editor.Icons.Play64);
            //_liveRecordingButton.LinkTooltip("Live profiling events recording");
            _liveRecordingButton.AutoCheck = true;
            _liveRecordingButton.Clicked += OnLiveRecordingChanged;
            _clearButton = toolstrip.AddButton(editor.Icons.Rotate32, Clear);
            _clearButton.LinkTooltip("Clear data");
            toolstrip.AddSeparator();
            _prevFrameButton = toolstrip.AddButton(editor.Icons.Left64, () => ViewFrameIndex--);
            _prevFrameButton.LinkTooltip("Previous frame");
            _nextFrameButton = toolstrip.AddButton(editor.Icons.Right64, () => ViewFrameIndex++);
            _nextFrameButton.LinkTooltip("Next frame");
            _lastFrameButton = toolstrip.AddButton(editor.Icons.Skip64, () => ViewFrameIndex = -1);
            _lastFrameButton.LinkTooltip("Current frame");
            toolstrip.AddSeparator();
            _showOnlyLastUpdateEventsButton = toolstrip.AddButton(editor.Icons.CenterView64, () => ShowOnlyLastUpdateEvents = !ShowOnlyLastUpdateEvents);
            _showOnlyLastUpdateEventsButton.LinkTooltip("Show only last update events and hide events from the other callbacks (e.g. draw or fixed update)");
            _liveDetailsButton = toolstrip.AddButton("Live details", () => { _nextDetailsRefresh = 0; if (_liveDetailsButton.Checked) UpdateView(); });
            _liveDetailsButton.AutoCheck = true;
            _liveDetailsButton.Checked = true;
            new Label { Text = "UI Hz", Width = 38, HorizontalAlignment = TextAlignment.Near, Parent = toolstrip };
            _refreshRateBox = new IntValueBox(10, width: 56, min: 1, max: 120) { Parent = toolstrip };
            _refreshRateBox.ValueChanged += () => _nextDetailsRefresh = 0;
            _refreshRateBox.LinkTooltip("Live details updates per second (1–120). Recording still captures every completed frame.");
            _detailsFrameLabel = new Label { Text = "No capture", Width = 180, HorizontalAlignment = TextAlignment.Near, Parent = toolstrip };
            _liveDetailsButton.LinkTooltip("Refresh live details at the selected UI Hz rate. Turn off to read the current details while frame recording continues. Asset/resource inventories are captured only while their tab is active.");

            _tabs = new Tabs
            {
                Orientation = Orientation.Vertical,
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = new Margin(0, 0, toolstrip.Bottom, 0),
                TabsSize = new Float2(120, 32),
                Parent = this
            };
            _tabs.SelectedTabChanged += OnSelectedTabChanged;

            FlaxEditor.Utilities.Utils.SetupCommonInputActions(this);
            InputActions.Bindings.RemoveAll(x => x.Callback == this.FocusOrShow);
            InputActions.Add(options => options.ProfilerWindow, Hide);
#endif
        }

#if USE_PROFILER
        private void OnLiveRecordingChanged()
        {
            _liveRecordingButton.Icon = LiveRecording ? Editor.Icons.Stop64 : Editor.Icons.Play64;
            ProfilingTools.Enabled = LiveRecording;
            _lastRecordedFrame = ulong.MaxValue;
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
            _lastManagedMemory = 0;
            _lastRecordedFrame = ulong.MaxValue;
            _frameIds.Clear();
            for (int i = 0; i < _tabs.ChildrenCount; i++)
            {
                if (_tabs.Children[i] is ProfilerMode mode)
                {
                    mode.Clear();
                    FlaxEngine.Profiler.BeginEvent("ProfilerWindow.UpdateView");
                    RenderDetails(mode);
                    FlaxEngine.Profiler.EndEvent();
                }
            }

            UpdateButtons();
        }

        private void OnSelectedTabChanged(Tabs tabs)
        {
            if (tabs.SelectedTab is ProfilerMode mode)
            {
                FlaxEngine.Profiler.BeginEvent("ProfilerWindow.UpdateView");
                RenderDetails(mode);
                FlaxEngine.Profiler.EndEvent();
            }
        }

        private void RenderDetails(ProfilerMode mode)
        {
            mode.UpdateView(_frameIndex, _showOnlyLastUpdateEvents);
            var ids = _frameIds.Get(_frameIndex);
            _detailsFrameLabel.Text = ids.CPU == 0 ? "No capture" : mode is GPU ? $"GPU {ids.GPU} / CPU {ids.CPU}" : $"Frame {ids.CPU}";
        }

        private void UpdateButtons()
        {
            _clearButton.Enabled = _framesCount > 0;
            _prevFrameButton.Enabled = _frameIndex > 0;
            _nextFrameButton.Enabled = (_framesCount - _frameIndex - 1) > 0;
            _lastFrameButton.Enabled = _framesCount > 0;
            _showOnlyLastUpdateEventsButton.Checked = _showOnlyLastUpdateEvents;
        }

        private void UpdateView()
        {
            if (_tabs.SelectedTab is ProfilerMode mode)
            {
                FlaxEngine.Profiler.BeginEvent("ProfilerWindow.UpdateView");
                RenderDetails(mode);
                FlaxEngine.Profiler.EndEvent();
            }
        }

        /// <inheritdoc />
        public override void OnInit()
        {
            // Create default modes
            AddMode(new Overall());
            AddMode(new CPU());
            AddMode(new GPU());
            AddMode(new MemoryGPU());
            AddMode(new Memory());
            AddMode(new Assets());
            AddMode(new Network());
            AddMode(new Physics());

            // Init view
            _frameIndex = -1;
            for (int i = 0; i < _tabs.ChildrenCount; i++)
            {
                if (_tabs.Children[i] is ProfilerMode mode)
                    RenderDetails(mode);
            }

            UpdateButtons();

            ScriptsBuilder.ScriptsReloadEnd += Clear; // Prevent crashes if any of the profiler tabs has some scripting types cached (eg. asset type info)
        }

        /// <inheritdoc />
        public override void OnUpdate()
        {
            for (int i = 0; i < _tabs.ChildrenCount; i++)
            {
                if (_tabs.Children[i] is ProfilerMode mode)
                    mode.UpdateStats();
            }

            ulong completedFrame = ProfilingTools.Stats.FrameIndex;
            if (LiveRecording && completedFrame != 0 && completedFrame != _lastRecordedFrame)
            {
                FlaxEngine.Profiler.BeginEvent("ProfilerWindow.OnUpdate");

                // Get memory allocations during last frame
                long managedMemory = GC.GetAllocatedBytesForCurrentThread();
                if (_lastManagedMemory == 0)
                    _lastManagedMemory = managedMemory;
                var managedAllocs = managedMemory - _lastManagedMemory - _lastManagedMemoryProfiler;
                _lastManagedMemory = managedMemory;

                ProfilerMode.SharedUpdateData sharedData = new ProfilerMode.SharedUpdateData();
                sharedData.Begin();
                _lastRecordedFrame = sharedData.Stats.FrameIndex;
                _frameIds.Add((sharedData.Stats.FrameIndex, sharedData.Stats.GPUFrameIndex));
                sharedData.ManagedMemoryAllocation = (int)managedAllocs;
                for (int i = 0; i < _tabs.ChildrenCount; i++)
                {
                    if (_tabs.Children[i] is ProfilerMode mode)
                    {
                        FlaxEngine.Profiler.BeginEvent(mode.GetType().FullName);
                        if (mode == _tabs.SelectedTab) mode.Update(ref sharedData);
                        else mode.UpdateInactive(ref sharedData);
                        FlaxEngine.Profiler.EndEvent();
                    }
                }
                {
                    long now = Stopwatch.GetTimestamp();
                    if (_liveDetailsButton.Checked && now >= _nextDetailsRefresh && _tabs.SelectedTab is ProfilerMode mode)
                    {
                        _nextDetailsRefresh = now + Stopwatch.Frequency / RefreshRateHz;
                        FlaxEngine.Profiler.BeginEvent("ProfilerWindow.UpdateView");
                        RenderDetails(mode);
                        FlaxEngine.Profiler.EndEvent();
                    }
                }
                sharedData.End();

                _framesCount = Mathf.Min(_framesCount + 1, ProfilerMode.MaxSamples);
                UpdateButtons();

                // Get memory allocations within profiler window update to exclude from stats
                managedMemory = GC.GetAllocatedBytesForCurrentThread();
                _lastManagedMemoryProfiler = managedMemory - _lastManagedMemory;

                FlaxEngine.Profiler.EndEvent();
            }
        }

        /// <inheritdoc />
        public override bool UseLayoutData => true;

        /// <inheritdoc />
        public override void OnLayoutSerialize(XmlWriter writer)
        {
            writer.WriteAttributeString("RefreshRateHz", RefreshRateHz.ToString());
        }

        /// <inheritdoc />
        public override void OnLayoutDeserialize(XmlElement node)
        {
            if (int.TryParse(node.GetAttribute("RefreshRateHz"), out int rate)) RefreshRateHz = rate;
        }

        /// <inheritdoc />
        public override void OnLayoutDeserialize()
        {
            RefreshRateHz = 10;
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
#endif
    }
}
