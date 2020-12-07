// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using System.Collections.Generic;

namespace FlaxEngine.GUI
{
    /// <summary>
    /// GUI root control that is represented by a window or an canvas and can contain children but has no parent at all. It's a source of the input events.
    /// </summary>
    public abstract class RootControl : ContainerControl
    {
        private static ContainerControl _gameRoot;
        private static CanvasContainer _canvasContainer = new CanvasContainer();

        /// <summary>
        /// Gets the main GUI control (it can be window or editor overriden control). Use it to plug-in custom GUI controls.
        /// </summary>
        public static ContainerControl GameRoot
        {
            get => _gameRoot;
            internal set
            {
                _gameRoot = value;
                _canvasContainer.Parent = _gameRoot;
            }
        }

        /// <summary>
        /// Gets the canvas controls root container.
        /// </summary>
        internal static CanvasContainer CanvasRoot => _canvasContainer;

        /// <summary>
        /// Gets or sets the current focused control
        /// </summary>
        public abstract Control FocusedControl { get; set; }

        /// <summary>
        /// Gets the tracking mouse offset.
        /// </summary>
        public abstract Vector2 TrackingMouseOffset { get; }

        /// <summary>
        /// Gets or sets the position of the mouse in the window space coordinates.
        /// </summary>
        public abstract Vector2 MousePosition { get; set; }

        /// <summary>
        /// The update callbacks collection. Controls can register for this to get the update event for logic handling.
        /// </summary>
        public readonly List<UpdateDelegate> UpdateCallbacks = new List<UpdateDelegate>(1024);

        /// <summary>
        /// The update callbacks to add before invoking the update.
        /// </summary>
        public List<UpdateDelegate> UpdateCallbacksToAdd = new List<UpdateDelegate>();

        /// <summary>
        /// The update callbacks to remove before invoking the update.
        /// </summary>
        public List<UpdateDelegate> UpdateCallbacksToRemove = new List<UpdateDelegate>();

        /// <summary>
        /// Initializes a new instance of the <see cref="RootControl"/> class.
        /// </summary>
        protected RootControl()
        : base(0, 0, 100, 60)
        {
            AutoFocus = false;
        }

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            base.Update(deltaTime);

            // Flush requests
            Profiler.BeginEvent("RootControl.SyncCallbacks");
            for (int i = 0; i < UpdateCallbacksToAdd.Count; i++)
            {
                UpdateCallbacks.Add(UpdateCallbacksToAdd[i]);
            }
            UpdateCallbacksToAdd.Clear();
            for (int i = 0; i < UpdateCallbacksToRemove.Count; i++)
            {
                UpdateCallbacks.Remove(UpdateCallbacksToRemove[i]);
            }
            UpdateCallbacksToRemove.Clear();
            Profiler.EndEvent();

            // Perform the UI update
            try
            {
                Profiler.BeginEvent("RootControl.Update");
                for (int i = 0; i < UpdateCallbacks.Count; i++)
                {
                    UpdateCallbacks[i](deltaTime);
                }
            }
            finally
            {
                Profiler.EndEvent();
            }
        }

        /// <summary>
        /// Starts the mouse tracking. Used by the scrollbars, splitters, etc.
        /// </summary>
        /// <param name="control">The target control that want to track mouse. It will receive OnMouseMove event.</param>
        /// <param name="useMouseScreenOffset">If set to <c>true</c> will use mouse screen offset.</param>
        public abstract void StartTrackingMouse(Control control, bool useMouseScreenOffset);

        /// <summary>
        /// Ends the mouse tracking.
        /// </summary>
        public abstract void EndTrackingMouse();

        /// <summary>
        /// Gets keyboard key state.
        /// </summary>
        /// <param name="key">Key to check.</param>
        /// <returns>True while the user holds down the key identified by id.</returns>
        public abstract bool GetKey(KeyboardKeys key);

        /// <summary>
        /// Gets keyboard key down state.
        /// </summary>
        /// <param name="key">Key to check.</param>
        /// <returns>True during the frame the user starts pressing down the key.</returns>
        public abstract bool GetKeyDown(KeyboardKeys key);

        /// <summary>
        /// Gets keyboard key up state.
        /// </summary>
        /// <param name="key">Key to check.</param>
        /// <returns>True during the frame the user releases the button.</returns>
        public abstract bool GetKeyUp(KeyboardKeys key);

        /// <summary>
        /// Gets mouse button state.
        /// </summary>
        /// <param name="button">Mouse button to check.</param>
        /// <returns>True while the user holds down the button.</returns>
        public abstract bool GetMouseButton(MouseButton button);

        /// <summary>
        /// Gets mouse button down state.
        /// </summary>
        /// <param name="button">Mouse button to check.</param>
        /// <returns>True during the frame the user starts pressing down the button.</returns>
        public abstract bool GetMouseButtonDown(MouseButton button);

        /// <summary>
        /// Gets mouse button up state.
        /// </summary>
        /// <param name="button">Mouse button to check.</param>
        /// <returns>True during the frame the user releases the button.</returns>
        public abstract bool GetMouseButtonUp(MouseButton button);

        /// <inheritdoc />
        public override RootControl Root => this;
    }
}
