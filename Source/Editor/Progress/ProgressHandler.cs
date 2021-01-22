// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using FlaxEngine;

namespace FlaxEditor.Progress
{
    /// <summary>
    /// Base class for all editor handlers used to report actions progress to the user.
    /// </summary>
    [HideInEditor]
    public abstract class ProgressHandler
    {
        /// <summary>
        /// Progress handler events delegate.
        /// </summary>
        /// <param name="handler">The calling handler.</param>
        public delegate void ProgressDelegate(ProgressHandler handler);

        private float _progress;
        private bool _isActive;
        private string _infoText;

        /// <summary>
        /// Gets a value indicating whether this handler is active.
        /// </summary>
        public bool IsActive => _isActive;

        /// <summary>
        /// Gets the current progress (normalized to range [0;1]).
        /// </summary>
        public float Progress => _progress;

        /// <summary>
        /// Gets the information text.
        /// </summary>
        public string InfoText => _infoText;

        /// <summary>
        /// Occurs when progress starts (becomes active).
        /// </summary>
        public event ProgressDelegate ProgressStart;

        /// <summary>
        /// Occurs when progress gets changed (or info text changes).
        /// </summary>
        public event ProgressDelegate ProgressChanged;

        /// <summary>
        /// Occurs when progress end (becomes inactive).
        /// </summary>
        public event ProgressDelegate ProgressEnd;

        /// <summary>
        /// Gets a value indicating whether this handler action can be cancelled.
        /// </summary>
        public virtual bool CanBeCanceled => false;

        /// <summary>
        /// Cancels this progress action.
        /// </summary>
        public void Cancel()
        {
            throw new InvalidOperationException("Cannot cancel this progress action.");
        }

        /// <summary>
        /// Called when progress action starts.
        /// </summary>
        protected virtual void OnStart()
        {
            if (_isActive)
                throw new InvalidOperationException("Already started.");

            _isActive = true;
            _progress = 0;
            _infoText = string.Empty;
            ProgressStart?.Invoke(this);
            ProgressChanged?.Invoke(this);
        }

        /// <summary>
        /// Called when progress action gets updated (changed info text or progress value).
        /// </summary>
        /// <param name="progress">The progress (normalized to range [0;1]).</param>
        /// <param name="infoText">The information text.</param>
        protected virtual void OnUpdate(float progress, string infoText)
        {
            progress = Mathf.Saturate(progress);
            if (!Mathf.NearEqual(progress, _progress) || infoText != _infoText)
            {
                _progress = progress;
                _infoText = infoText;
                ProgressChanged?.Invoke(this);
            }
        }

        /// <summary>
        /// Called when progress action ends.
        /// </summary>
        protected virtual void OnEnd()
        {
            if (!_isActive)
                throw new InvalidOperationException("Already ended.");

            _isActive = false;
            _progress = 0;
            _infoText = string.Empty;
            ProgressChanged?.Invoke(this);
            ProgressEnd?.Invoke(this);
        }
    }
}
