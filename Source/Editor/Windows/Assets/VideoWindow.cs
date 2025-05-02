// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEditor.Content;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Windows.Assets
{
    /// <summary>
    /// Editor window to view video media.
    /// </summary>
    public sealed class VideoWindow : EditorWindow, IContentItemOwner
    {
        private VideoItem _item;
        private Image _frame;
        private VideoPlayer _videoPlayer;
        private Image _seekBegin, _seekEnd, _seekLeft, _seekRight, _playPause, _stop;

        /// <inheritdoc />
        public VideoWindow(Editor editor, VideoItem item)
        : base(editor, false, ScrollBars.None)
        {
            _item = item;
            _item.AddReference(this);
            Title = _item.ShortName;

            // Setup video player
            _videoPlayer = new VideoPlayer
            {
                PlayOnStart = false,
                Url = item.Path,
            };

            // Setup UI
            var style = Style.Current;
            var icons = Editor.Icons;
            var playbackButtonsSize = 24.0f;
            var playbackButtonsMouseOverColor = Color.FromBgra(0xFFBBBBBB);
            _frame = new Image
            {
                Brush = new VideoBrush(_videoPlayer),
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = new Margin(0.0f, 0.0f, 0.0f, playbackButtonsSize),
                Parent = this,
            };
            var playbackButtonsArea = new ContainerControl
            {
                AutoFocus = false,
                ClipChildren = false,
                BackgroundColor = style.LightBackground,
                AnchorPreset = AnchorPresets.HorizontalStretchBottom,
                Offsets = new Margin(0, 0, -playbackButtonsSize, playbackButtonsSize),
                Parent = this
            };
            var playbackButtonsPanel = new ContainerControl
            {
                AutoFocus = false,
                ClipChildren = false,
                AnchorPreset = AnchorPresets.VerticalStretchCenter,
                Offsets = Margin.Zero,
                Parent = playbackButtonsArea,
            };
            _seekBegin = new Image(playbackButtonsPanel.Width, 0, playbackButtonsSize, playbackButtonsSize)
            {
                TooltipText = "Rewind to timeline start (Home)",
                Brush = new SpriteBrush(icons.Skip64),
                MouseOverColor = playbackButtonsMouseOverColor,
                Rotation = 180.0f,
                Parent = playbackButtonsPanel
            };
            _seekBegin.Clicked += (image, button) => SeekBegin();
            playbackButtonsPanel.Width += playbackButtonsSize;
            _seekLeft = new Image(playbackButtonsPanel.Width, 0, playbackButtonsSize, playbackButtonsSize)
            {
                TooltipText = "Move one frame back (Left Arrow)",
                Brush = new SpriteBrush(icons.Left32),
                MouseOverColor = playbackButtonsMouseOverColor,
                Parent = playbackButtonsPanel
            };
            _seekLeft.Clicked += (image, button) => SeekLeft();
            playbackButtonsPanel.Width += playbackButtonsSize;
            _stop = new Image(playbackButtonsPanel.Width, 0, playbackButtonsSize, playbackButtonsSize)
            {
                TooltipText = "Stop playback",
                Brush = new SpriteBrush(icons.Stop64),
                MouseOverColor = playbackButtonsMouseOverColor,
                Parent = playbackButtonsPanel
            };
            _stop.Clicked += (image, button) => Stop();
            playbackButtonsPanel.Width += playbackButtonsSize;
            _playPause = new Image(playbackButtonsPanel.Width, 0, playbackButtonsSize, playbackButtonsSize)
            {
                TooltipText = "Play/pause playback (Space)",
                Brush = new SpriteBrush(icons.Play64),
                MouseOverColor = playbackButtonsMouseOverColor,
                Parent = playbackButtonsPanel
            };
            _playPause.Clicked += (image, button) => PlayPause();
            playbackButtonsPanel.Width += playbackButtonsSize;
            _seekRight = new Image(playbackButtonsPanel.Width, 0, playbackButtonsSize, playbackButtonsSize)
            {
                TooltipText = "Move one frame forward (Right Arrow)",
                Brush = new SpriteBrush(icons.Right32),
                MouseOverColor = playbackButtonsMouseOverColor,
                Parent = playbackButtonsPanel
            };
            _seekRight.Clicked += (image, button) => SeekRight();
            playbackButtonsPanel.Width += playbackButtonsSize;
            _seekEnd = new Image(playbackButtonsPanel.Width, 0, playbackButtonsSize, playbackButtonsSize)
            {
                TooltipText = "Rewind to timeline end (End)",
                Brush = new SpriteBrush(icons.Skip64),
                MouseOverColor = playbackButtonsMouseOverColor,
                Parent = playbackButtonsPanel
            };
            _seekEnd.Clicked += (image, button) => SeekEnd();
            playbackButtonsPanel.Width += playbackButtonsSize;
            playbackButtonsPanel.X = (playbackButtonsPanel.Parent.Width - playbackButtonsPanel.Width) * 0.5f;
        }

        private void PlayPause()
        {
            if (_videoPlayer.State == VideoPlayer.States.Playing)
                _videoPlayer.Pause();
            else
                _videoPlayer.Play();
        }

        private void Stop()
        {
            _videoPlayer.Stop();
        }

        private void SeekBegin()
        {
            _videoPlayer.Time = 0.0f;
        }

        private void SeekEnd()
        {
            _videoPlayer.Time = _videoPlayer.Duration;
        }

        private void SeekLeft()
        {
            if (_videoPlayer.State == VideoPlayer.States.Paused)
                _videoPlayer.Time -= 1.0f / _videoPlayer.FrameRate;
        }

        private void SeekRight()
        {
            if (_videoPlayer.State == VideoPlayer.States.Paused)
                _videoPlayer.Time += 1.0f / _videoPlayer.FrameRate;
        }

        /// <inheritdoc />
        public override bool OnKeyDown(KeyboardKeys key)
        {
            if (base.OnKeyDown(key))
                return true;

            switch (key)
            {
            case KeyboardKeys.ArrowLeft:
                SeekLeft();
                return true;
            case KeyboardKeys.ArrowRight:
                SeekRight();
                return true;
            case KeyboardKeys.Home:
                SeekBegin();
                return true;
            case KeyboardKeys.End:
                SeekEnd();
                return true;
            case KeyboardKeys.Spacebar:
                PlayPause();
                return true;
            }

            return false;
        }

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            base.Update(deltaTime);

            // Update UI
            var state = _videoPlayer.State;
            var icons = Editor.Icons;
            _stop.Enabled = state != VideoPlayer.States.Stopped;
            _seekLeft.Enabled = _seekRight.Enabled = state != VideoPlayer.States.Playing;
            ((SpriteBrush)_playPause.Brush).Sprite = state == VideoPlayer.States.Playing ? icons.Pause64 : icons.Play64;
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            if (IsDisposing)
                return;
            base.OnDestroy();

            _videoPlayer.Stop();
            Object.Destroy(ref _videoPlayer);
            _item.RemoveReference(this);
            _item = null;
        }

        /// <inheritdoc />
        public void OnItemDeleted(ContentItem item)
        {
            if (item == _item)
                Close();
        }

        /// <inheritdoc />
        public void OnItemRenamed(ContentItem item)
        {
        }

        /// <inheritdoc />
        public void OnItemReimported(ContentItem item)
        {
        }

        /// <inheritdoc />
        public void OnItemDispose(ContentItem item)
        {
            if (item == _item)
                Close();
        }
    }
}
