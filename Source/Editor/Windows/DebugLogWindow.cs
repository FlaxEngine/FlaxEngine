// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading;
using System.Xml;
using FlaxEditor.GUI;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.Options;
using FlaxEngine;
using FlaxEngine.Assertions;
using FlaxEngine.GUI;
using Object = FlaxEngine.Object;

namespace FlaxEditor.Windows
{
    /// <summary>
    /// Editor window used to show debug info, warning and error messages. Captures <see cref="Debug"/> messages.
    /// </summary>
    /// <seealso cref="FlaxEditor.Windows.EditorWindow" />
    public class DebugLogWindow : EditorWindow
    {
        /// <summary>
        /// Debug log entry description;
        /// </summary>
        public struct LogEntryDescription
        {
            /// <summary>
            /// The log level.
            /// </summary>
            public LogType Level;

            /// <summary>
            /// The message title.
            /// </summary>
            public string Title;

            /// <summary>
            /// The message description.
            /// </summary>
            public string Description;

            /// <summary>
            /// The target object hint id (don't store ref, object may be an actor that can be removed and restored later or sth).
            /// </summary>
            public Guid ContextObject;

            /// <summary>
            /// The location of the issue (file path).
            /// </summary>
            public string LocationFile;

            /// <summary>
            /// The location line number (zero or less to not use it);
            /// </summary>
            public int LocationLine;
        };

        private enum LogGroup
        {
            Error = 0,
            Warning,
            Info,
            Max
        };

        private class LogEntry : Control
        {
            private bool _isRightMouseDown;

            /// <summary>
            /// The default height of the entries.
            /// </summary>
            public const float DefaultHeight = 32.0f;

            private DebugLogWindow _window;
            public LogGroup Group;
            public LogEntryDescription Desc;
            public SpriteHandle Icon;

            public LogEntry(DebugLogWindow window, ref LogEntryDescription desc)
            : base(0, 0, 120, DefaultHeight)
            {
                AnchorPreset = AnchorPresets.HorizontalStretchTop;
                IsScrollable = true;

                _window = window;
                Desc = desc;
                switch (desc.Level)
                {
                case LogType.Warning:
                    Group = LogGroup.Warning;
                    Icon = _window.IconWarning;
                    break;
                case LogType.Info:
                    Group = LogGroup.Info;
                    Icon = _window.IconInfo;
                    break;
                default:
                    Group = LogGroup.Error;
                    Icon = _window.IconError;
                    break;
                }
            }

            /// <summary>
            /// Gets the information text.
            /// </summary>
            public string Info => Desc.Title + '\n' + Desc.Description;

            /// <inheritdoc />
            public override void Draw()
            {
                base.Draw();

                // Cache data
                var style = Style.Current;
                var index = IndexInParent;
                var clientRect = new Rectangle(Vector2.Zero, Size);

                // Background
                if (_window._selected == this)
                    Render2D.FillRectangle(clientRect, IsFocused ? style.BackgroundSelected : style.LightBackground);
                else if (IsMouseOver)
                    Render2D.FillRectangle(clientRect, style.BackgroundHighlighted);
                else if (index % 2 == 0)
                    Render2D.FillRectangle(clientRect, style.Background * 0.9f);

                // Icon
                var iconColor = Group == LogGroup.Error ? Color.Red : (Group == LogGroup.Warning ? Color.Yellow : style.Foreground);
                Render2D.DrawSprite(Icon, new Rectangle(5, 0, 32, 32), iconColor);

                // Title
                var textRect = new Rectangle(38, 2, clientRect.Width - 40, clientRect.Height - 10);
                Render2D.PushClip(ref clientRect);
                Render2D.DrawText(style.FontMedium, Desc.Title, textRect, style.Foreground);
                Render2D.PopClip();
            }

            /// <inheritdoc />
            public override void OnGotFocus()
            {
                base.OnGotFocus();

                _window.Selected = this;
            }

            /// <inheritdoc />
            protected override void OnVisibleChanged()
            {
                // Deselect on hide
                if (!Visible && _window.Selected == this)
                    _window.Selected = null;

                base.OnVisibleChanged();
            }

            /// <inheritdoc />
            public override bool OnKeyDown(KeyboardKeys key)
            {
                // Up
                if (key == KeyboardKeys.ArrowUp)
                {
                    int index = IndexInParent - 1;
                    if (index >= 0)
                    {
                        var target = Parent.GetChild(index);
                        target.Focus();
                        ((Panel)Parent.Parent).ScrollViewTo(target);
                        return true;
                    }
                }
                // Down
                else if (key == KeyboardKeys.ArrowDown)
                {
                    int index = IndexInParent + 1;
                    if (index < Parent.ChildrenCount)
                    {
                        var target = Parent.GetChild(index);
                        target.Focus();
                        ((Panel)Parent.Parent).ScrollViewTo(target);
                        return true;
                    }
                }
                // Enter
                else if (key == KeyboardKeys.Return)
                {
                    Open();
                }
                // Ctrl+C
                else if (key == KeyboardKeys.C && Root.GetKey(KeyboardKeys.Control))
                {
                    Copy();
                    return true;
                }

                return base.OnKeyDown(key);
            }

            /// <summary>
            /// Opens the entry location.
            /// </summary>
            public void Open()
            {
                if (!string.IsNullOrEmpty(Desc.LocationFile) && File.Exists(Desc.LocationFile))
                {
                    Editor.Instance.CodeEditing.OpenFile(Desc.LocationFile, Desc.LocationLine);
                }
            }

            /// <summary>
            /// Copies the entry information to the system clipboard (as text).
            /// </summary>
            public void Copy()
            {
                Clipboard.Text = Info.Replace("\r\n", "\n").Replace("\n", Environment.NewLine);
            }

            public override bool OnMouseDoubleClick(Vector2 location, MouseButton button)
            {
                Open();
                return true;
            }

            /// <inheritdoc />
            public override bool OnMouseDown(Vector2 location, MouseButton button)
            {
                if (base.OnMouseDown(location, button))
                    return true;

                if (button == MouseButton.Right)
                {
                    Focus();

                    _isRightMouseDown = true;
                    return true;
                }

                return false;
            }

            /// <inheritdoc />
            public override bool OnMouseUp(Vector2 location, MouseButton button)
            {
                if (base.OnMouseUp(location, button))
                    return true;

                if (_isRightMouseDown && button == MouseButton.Right)
                {
                    Focus();

                    var menu = new ContextMenu();
                    menu.AddButton("Copy", Copy);
                    menu.AddButton("Open", Open).Enabled = !string.IsNullOrEmpty(Desc.LocationFile) && File.Exists(Desc.LocationFile);
                    menu.Show(this, location);
                }

                return false;
            }

            /// <inheritdoc />
            public override void OnMouseLeave()
            {
                _isRightMouseDown = false;

                base.OnMouseLeave();
            }
        }

        private readonly SplitPanel _split;
        private readonly Label _logInfo;
        private readonly VerticalPanel _entriesPanel;
        private LogEntry _selected;
        private readonly int[] _logCountPerGroup = new int[(int)LogGroup.Max];
        private readonly Regex _logRegex = new Regex("at (.*) in (.*):(line (\\d*)|(\\d*))");
        private readonly ThreadLocal<StringBuilder> _stringBuilder = new ThreadLocal<StringBuilder>(() => new StringBuilder(), false);
        private InterfaceOptions.TimestampsFormats _timestampsFormats;

        private readonly object _locker = new object();
        private readonly List<LogEntry> _pendingEntries = new List<LogEntry>(32);

        private readonly ToolStripButton _clearOnPlayButton;
        private readonly ToolStripButton _pauseOnErrorButton;
        private readonly ToolStripButton[] _groupButtons = new ToolStripButton[3];

        private LogType _iconType = LogType.Info;

        internal SpriteHandle IconInfo;
        internal SpriteHandle IconWarning;
        internal SpriteHandle IconError;

        /// <summary>
        /// Initializes a new instance of the <see cref="DebugLogWindow"/> class.
        /// </summary>
        /// <param name="editor">The editor.</param>
        public DebugLogWindow(Editor editor)
        : base(editor, true, ScrollBars.None)
        {
            Title = "Debug Log";
            Icon = IconInfo;
            OnEditorOptionsChanged(Editor.Options.Options);

            // Toolstrip
            var toolstrip = new ToolStrip(22.0f)
            {
                Parent = this,
            };
            toolstrip.AddButton("Clear", Clear).LinkTooltip("Clears all log entries");
            _clearOnPlayButton = (ToolStripButton)toolstrip.AddButton("Clear on Play").SetAutoCheck(true).SetChecked(true).LinkTooltip("Clears all log entries on enter playmode");
            _pauseOnErrorButton = (ToolStripButton)toolstrip.AddButton("Pause on Error").SetAutoCheck(true).LinkTooltip("Performs auto pause on error");
            toolstrip.AddSeparator();
            _groupButtons[0] = (ToolStripButton)toolstrip.AddButton(editor.Icons.Error32, () => UpdateLogTypeVisibility(LogGroup.Error, _groupButtons[0].Checked)).SetAutoCheck(true).SetChecked(true).LinkTooltip("Shows/hides error messages");
            _groupButtons[1] = (ToolStripButton)toolstrip.AddButton(editor.Icons.Warning32, () => UpdateLogTypeVisibility(LogGroup.Warning, _groupButtons[1].Checked)).SetAutoCheck(true).SetChecked(true).LinkTooltip("Shows/hides warning messages");
            _groupButtons[2] = (ToolStripButton)toolstrip.AddButton(editor.Icons.Info32, () => UpdateLogTypeVisibility(LogGroup.Info, _groupButtons[2].Checked)).SetAutoCheck(true).SetChecked(true).LinkTooltip("Shows/hides info messages");
            UpdateCount();

            // Split panel
            _split = new SplitPanel(Orientation.Vertical, ScrollBars.Vertical, ScrollBars.Both)
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = new Margin(0, 0, toolstrip.Bottom, 0),
                SplitterValue = 0.8f,
                Parent = this
            };

            // Info detail info
            _logInfo = new Label
            {
                Parent = _split.Panel2,
                AutoWidth = true,
                AutoHeight = true,
                Margin = new Margin(4),
                Offsets = Margin.Zero,
                AnchorPreset = AnchorPresets.StretchAll,
            };

            // Entries panel
            _entriesPanel = new VerticalPanel
            {
                AnchorPreset = AnchorPresets.HorizontalStretchTop,
                Offsets = Margin.Zero,
                IsScrollable = true,
                Parent = _split.Panel1,
            };

            // Cache entries icons
            IconInfo = Editor.Icons.Info32;
            IconWarning = Editor.Icons.Warning32;
            IconError = Editor.Icons.Error32;

            // Bind events
            Editor.Options.OptionsChanged += OnEditorOptionsChanged;
            Debug.Logger.LogHandler.SendLog += LogHandlerOnSendLog;
            Debug.Logger.LogHandler.SendExceptionLog += LogHandlerOnSendExceptionLog;
        }

        private void OnEditorOptionsChanged(EditorOptions options)
        {
            _timestampsFormats = options.Interface.DebugLogTimestampsFormat;
        }

        /// <summary>
        /// Clears the log.
        /// </summary>
        public void Clear()
        {
            if (_entriesPanel == null)
                return;

            RemoveEntries();
        }

        /// <summary>
        /// Adds the specified log entry.
        /// </summary>
        /// <param name="desc">The log entry description.</param>
        public void Add(ref LogEntryDescription desc)
        {
            if (_entriesPanel == null)
                return;

            // Create new entry
            switch (_timestampsFormats)
            {
            case InterfaceOptions.TimestampsFormats.Utc:
                desc.Title = string.Format("[{0}] ", DateTime.UtcNow) + desc.Title;
                break;
            case InterfaceOptions.TimestampsFormats.LocalTime:
                desc.Title = string.Format("[{0}] ", DateTime.Now) + desc.Title;
                break;
            case InterfaceOptions.TimestampsFormats.TimeSinceStartup:
                desc.Title = string.Format("[{0:g}] ", TimeSpan.FromSeconds(Time.TimeSinceStartup)) + desc.Title;
                break;
            }
            var newEntry = new LogEntry(this, ref desc);

            // Enqueue
            lock (_locker)
            {
                _pendingEntries.Add(newEntry);
            }

            if (newEntry.Group == LogGroup.Warning && _iconType < LogType.Warning)
            {
                _iconType = LogType.Warning;
                UpdateIcon();
            }

            if (newEntry.Group == LogGroup.Error && _iconType < LogType.Error)
            {
                _iconType = LogType.Error;
                UpdateIcon();
            }

            // Pause on Error (we should do it as fast as possible)
            if (newEntry.Group == LogGroup.Error && _pauseOnErrorButton.Checked && Editor.StateMachine.CurrentState == Editor.StateMachine.PlayingState)
            {
                Editor.Simulation.RequestPausePlay();
            }
        }

        /// <summary>
        /// Gets or sets the selected entry.
        /// </summary>
        private LogEntry Selected
        {
            get => _selected;
            set
            {
                // Check if value will change
                if (_selected != value)
                {
                    // Select
                    _selected = value;
                    _logInfo.Text = _selected?.Info ?? string.Empty;
                }
            }
        }

        private void UpdateLogTypeVisibility(LogGroup group, bool isVisible)
        {
            _entriesPanel.IsLayoutLocked = true;

            var children = _entriesPanel.Children;
            for (int i = 0; i < children.Count; i++)
            {
                if (children[i] is LogEntry logEntry && logEntry.Group == group)
                    logEntry.Visible = isVisible;
            }

            _entriesPanel.IsLayoutLocked = false;
            _entriesPanel.PerformLayout();
        }

        private void UpdateCount()
        {
            UpdateCount((int)LogGroup.Error, " Error");
            UpdateCount((int)LogGroup.Warning, " Warning");
            UpdateCount((int)LogGroup.Info, " Message");

            if (_logCountPerGroup[(int)LogGroup.Error] == 0)
            {
                _iconType = _logCountPerGroup[(int)LogGroup.Warning] == 0 ? LogType.Info : LogType.Warning;
                UpdateIcon();
            }
        }

        private void UpdateCount(int group, string msg)
        {
            if (_logCountPerGroup[group] != 1)
                msg += 's';
            _groupButtons[group].Text = _logCountPerGroup[group] + msg;
        }

        private void UpdateIcon()
        {
            if (_iconType == LogType.Warning)
            {
                Icon = IconWarning;
            }
            else if (_iconType == LogType.Error)
            {
                Icon = IconError;
            }
            else
            {
                Icon = IconInfo;
            }
        }

        private void LogHandlerOnSendLog(LogType level, string msg, Object o, string stackTrace)
        {
            var desc = new LogEntryDescription
            {
                Level = level,
                Title = msg,
                ContextObject = o?.ID ?? Guid.Empty,
            };

            if (!string.IsNullOrEmpty(stackTrace))
            {
                // Detect code location and remove leading internal stack trace part
                var matches = _logRegex.Matches(stackTrace);
                bool foundStart = false, noLocation = true;
                var fineStackTrace = _stringBuilder.Value;
                fineStackTrace.Clear();
                fineStackTrace.Capacity = Mathf.Max(fineStackTrace.Capacity, stackTrace.Length);
                for (int i = 0; i < matches.Count; i++)
                {
                    var match = matches[i];
                    var matchLocation = match.Groups[1].Value.Trim();
                    if (matchLocation.StartsWith("FlaxEngine.Debug.", StringComparison.Ordinal))
                    {
                        // C# start
                        foundStart = true;
                    }
                    else if (matchLocation.StartsWith("DebugLog::", StringComparison.Ordinal))
                    {
                        // C++ start
                        foundStart = true;
                    }
                    else if (foundStart)
                    {
                        if (noLocation)
                        {
                            desc.LocationFile = match.Groups[2].Value;
                            int.TryParse(match.Groups[5].Value, out desc.LocationLine);
                            noLocation = false;
                        }
                        fineStackTrace.AppendLine(match.Groups[0].Value);
                    }
                }
                desc.Description = fineStackTrace.ToString();
            }

            Add(ref desc);
        }

        private void LogHandlerOnSendExceptionLog(Exception exception, Object o)
        {
            LogEntryDescription desc = new LogEntryDescription
            {
                Level = LogType.Error,
                Title = exception.Message,
                Description = exception.StackTrace,
                ContextObject = o?.ID ?? Guid.Empty,
            };

            // Detect code location
            if (!string.IsNullOrEmpty(exception.StackTrace))
            {
                var match = _logRegex.Match(exception.StackTrace);
                if (match.Success)
                {
                    desc.LocationFile = match.Groups[2].Value;
                    int.TryParse(match.Groups[3].Value, out desc.LocationLine);
                }
            }

            Add(ref desc);
        }

        private void RemoveEntries()
        {
            _entriesPanel.IsLayoutLocked = true;

            Selected = null;
            for (int i = 0; i < _entriesPanel.ChildrenCount; i++)
            {
                if (_entriesPanel.GetChild(i) is LogEntry entry)
                {
                    _logCountPerGroup[(int)entry.Group]--;
                    entry.Dispose();
                    i--;
                }
            }
            UpdateCount();

            _entriesPanel.IsLayoutLocked = false;
            _entriesPanel.PerformLayout();
        }

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            // Check pending events (in a thread safe manner)
            lock (_locker)
            {
                if (_pendingEntries.Count > 0)
                {
                    // TODO: we should provide max limit for entries count and remove if too many

                    // Check if user want's to scroll view by var (or is viewing earlier entry)
                    var panelScroll = (Panel)_entriesPanel.Parent;
                    bool scrollView = (panelScroll.VScrollBar.Maximum - panelScroll.VScrollBar.TargetValue) < LogEntry.DefaultHeight * 1.5f;

                    // Add pending entries
                    LogEntry newEntry = null;
                    bool anyVisible = false;
                    for (int i = 0; i < _pendingEntries.Count; i++)
                    {
                        newEntry = _pendingEntries[i];
                        newEntry.Visible = _groupButtons[(int)newEntry.Group].Checked;
                        anyVisible |= newEntry.Visible;
                        newEntry.Parent = _entriesPanel;
                        _logCountPerGroup[(int)newEntry.Group]++;
                    }
                    _pendingEntries.Clear();
                    UpdateCount();
                    Assert.IsNotNull(newEntry);

                    // Scroll to the new entry (if any added to view)
                    if (scrollView && anyVisible)
                        panelScroll.ScrollViewTo(newEntry);
                }
            }

            base.Update(deltaTime);
        }

        /// <inheritdoc />
        public override void OnPlayBegin()
        {
            // Clear on Play
            if (_clearOnPlayButton.Checked)
            {
                Clear();
            }
        }

        /// <inheritdoc />
        public override void OnStartContainsFocus()
        {
            _iconType = LogType.Info;
            UpdateIcon();
            base.OnStartContainsFocus();
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            if (IsDisposing)
                return;

            // Unbind events
            Editor.Options.OptionsChanged -= OnEditorOptionsChanged;
            Debug.Logger.LogHandler.SendLog -= LogHandlerOnSendLog;
            Debug.Logger.LogHandler.SendExceptionLog -= LogHandlerOnSendExceptionLog;

            base.OnDestroy();
        }

        /// <inheritdoc />
        public override bool UseLayoutData => true;

        /// <inheritdoc />
        public override void OnLayoutSerialize(XmlWriter writer)
        {
            writer.WriteAttributeString("Split", _split.SplitterValue.ToString());
        }

        /// <inheritdoc />
        public override void OnLayoutDeserialize(XmlElement node)
        {
            if (float.TryParse(node.GetAttribute("Split"), out float value1))
                _split.SplitterValue = value1;
        }

        /// <inheritdoc />
        public override void OnLayoutDeserialize()
        {
            _split.SplitterValue = 0.8f;
        }
    }
}
