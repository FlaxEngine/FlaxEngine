// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using FlaxEditor.History;
using FlaxEditor.Utilities;
using FlaxEngine.Collections;

namespace FlaxEditor
{
    /// <summary>
    /// The undo/redo actions recording object.
    /// </summary>
    public class Undo : IDisposable
    {
        /// <summary>
        /// Undo system event.
        /// </summary>
        /// <param name="action">The action.</param>
        public delegate void UndoEventDelegate(IUndoAction action);

        internal interface IUndoInternal
        {
            /// <summary>
            /// Creates the undo action object on recording end.
            /// </summary>
            /// <param name="snapshotInstance">The snapshot object.</param>
            /// <returns>The undo action. May be null if no changes found.</returns>
            IUndoAction End(object snapshotInstance);
        }

        /// <summary>
        /// Stack of undo actions for future disposal.
        /// </summary>
        private readonly OrderedDictionary<object, IUndoInternal> _snapshots = new OrderedDictionary<object, IUndoInternal>();

        /// <summary>
        /// Gets the undo operations stack.
        /// </summary>
        public HistoryStack UndoOperationsStack { get; }

        /// <summary>
        /// Occurs when undo operation is done.
        /// </summary>
        public event UndoEventDelegate UndoDone;

        /// <summary>
        /// Occurs when redo operation is done.
        /// </summary>
        public event UndoEventDelegate RedoDone;

        /// <summary>
        /// Occurs when action is done and appended to the <see cref="Undo"/>.
        /// </summary>
        public event UndoEventDelegate ActionDone;

        /// <summary>
        /// Gets or sets a value indicating whether this <see cref="Undo"/> is enabled.
        /// </summary>
        public virtual bool Enabled { get; set; } = true;

        /// <summary>
        /// Gets a value indicating whether can do undo on last performed action.
        /// </summary>
        public bool CanUndo => UndoOperationsStack.HistoryCount > 0;

        /// <summary>
        /// Gets a value indicating whether can do redo on last undone action.
        /// </summary>
        public bool CanRedo => UndoOperationsStack.ReverseCount > 0;

        /// <summary>
        /// Gets the first name of the undo action.
        /// </summary>
        public string FirstUndoName => UndoOperationsStack.PeekHistory().ActionString;

        /// <summary>
        /// Gets the first name of the redo action.
        /// </summary>
        public string FirstRedoName => UndoOperationsStack.PeekReverse().ActionString;

        /// <summary>
        /// Gets or sets the capacity of the undo history buffers.
        /// </summary>
        public int Capacity
        {
            get => UndoOperationsStack.HistoryActionsLimit;
            set => UndoOperationsStack.HistoryActionsLimit = value;
        }

        /// <summary>
        /// Internal class for keeping reference of undo action.
        /// </summary>
        internal class UndoInternal : IUndoInternal
        {
            public string ActionString;
            public object SnapshotInstance;
            public ObjectSnapshot Snapshot;

            public UndoInternal(object snapshotInstance, string actionString)
            {
                ActionString = actionString;
                SnapshotInstance = snapshotInstance;
                Snapshot = ObjectSnapshot.CaptureSnapshot(snapshotInstance);
            }

            /// <inheritdoc />
            public IUndoAction End(object snapshotInstance)
            {
                var diff = Snapshot.Compare(snapshotInstance);
                if (diff.Count == 0)
                    return null;
                return new UndoActionObject(diff, ActionString, SnapshotInstance);
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Undo"/> class.
        /// </summary>
        /// <param name="historyActionsLimit">The history actions limit.</param>
        public Undo(int historyActionsLimit = 1000)
        {
            UndoOperationsStack = new HistoryStack(historyActionsLimit);
        }

        /// <summary>
        /// Begins recording for undo action.
        /// </summary>
        /// <param name="snapshotInstance">Instance of an object to record.</param>
        /// <param name="actionString">Name of action to be displayed in undo stack.</param>
        public void RecordBegin(object snapshotInstance, string actionString)
        {
            if (!Enabled)
                return;

            _snapshots.Add(snapshotInstance, new UndoInternal(snapshotInstance, actionString));

        }

        /// <summary>
        /// Ends recording for undo action.
        /// </summary>
        /// <param name="snapshotInstance">Instance of an object to finish recording, if null take last provided.</param>
        /// <param name="customActionBefore">Custom action to append to the undo block action before recorded modifications apply.</param>
        /// <param name="customActionAfter">Custom action to append to the undo block action after recorded modifications apply.</param>
        public void RecordEnd(object snapshotInstance = null, IUndoAction customActionBefore = null, IUndoAction customActionAfter = null)
        {
            if (!Enabled)
                return;

            if (snapshotInstance == null)
                snapshotInstance = _snapshots.Last().Key;
            var action = _snapshots[snapshotInstance].End(snapshotInstance);
            _snapshots.Remove(snapshotInstance);

            // It may be null if no changes has been found during recording
            if (action != null)
            {
                // Batch with a custom action if provided
                if (customActionBefore != null && customActionAfter != null)
                {
                    action = new MultiUndoAction(new[] { customActionBefore, action, customActionAfter });
                }
                else if (customActionBefore != null)
                {
                    action = new MultiUndoAction(new[] { customActionBefore, action });
                }
                else if (customActionAfter != null)
                {
                    action = new MultiUndoAction(new[] { action, customActionAfter });
                }

                UndoOperationsStack.Push(action);
                OnAction(action);
            }
        }

        /// <summary>
        /// Internal class for keeping reference of undo action that modifies collection of objects.
        /// </summary>
        internal class UndoMultiInternal : IUndoInternal
        {
            public string ActionString;
            public object[] SnapshotInstances;
            public ObjectSnapshot[] Snapshot;

            public UndoMultiInternal(object[] snapshotInstances, string actionString)
            {
                ActionString = actionString;
                SnapshotInstances = snapshotInstances;
                Snapshot = new ObjectSnapshot[snapshotInstances.Length];
                for (var i = 0; i < snapshotInstances.Length; i++)
                {
                    Snapshot[i] = ObjectSnapshot.CaptureSnapshot(snapshotInstances[i]);
                }
            }

            /// <inheritdoc />
            public IUndoAction End(object snapshotInstance)
            {
                var snapshotInstances = (object[])snapshotInstance;
                if (snapshotInstances == null || snapshotInstances.Length != SnapshotInstances.Length)
                    throw new ArgumentException("Invalid multi undo action objects.");
                List<UndoActionObject> actions = null;
                for (int i = 0; i < snapshotInstances.Length; i++)
                {
                    var diff = Snapshot[i].Compare(snapshotInstances[i]);
                    if (diff.Count == 0)
                        continue;
                    if (actions == null)
                        actions = new List<UndoActionObject>();
                    actions.Add(new UndoActionObject(diff, ActionString, SnapshotInstances[i]));
                }
                if (actions == null)
                    return null;
                if (actions.Count == 1)
                    return actions[0];
                return new MultiUndoAction(actions);
            }
        }

        /// <summary>
        /// Begins recording for undo action.
        /// </summary>
        /// <param name="snapshotInstances">Instances of objects to record.</param>
        /// <param name="actionString">Name of action to be displayed in undo stack.</param>
        public void RecordMultiBegin(object[] snapshotInstances, string actionString)
        {
            if (!Enabled)
                return;

            _snapshots.Add(snapshotInstances, new UndoMultiInternal(snapshotInstances, actionString));
        }

        /// <summary>
        /// Ends recording for undo action.
        /// </summary>
        /// <param name="snapshotInstance">Instance of an object to finish recording, if null take last provided.</param>
        /// <param name="customActionBefore">Custom action to append to the undo block action before recorded modifications apply.</param>
        /// <param name="customActionAfter">Custom action to append to the undo block action after recorded modifications apply.</param>
        public void RecordMultiEnd(object[] snapshotInstance = null, IUndoAction customActionBefore = null, IUndoAction customActionAfter = null)
        {
            if (!Enabled)
                return;

            if (snapshotInstance == null)
                snapshotInstance = (object[])_snapshots.Last().Key;
            var action = _snapshots[snapshotInstance].End(snapshotInstance);
            _snapshots.Remove(snapshotInstance);

            // It may be null if no changes has been found during recording
            if (action != null)
            {
                // Batch with a custom action if provided
                if (customActionBefore != null && customActionAfter != null)
                {
                    action = new MultiUndoAction(new[] { customActionBefore, action, customActionAfter });
                }
                else if (customActionBefore != null)
                {
                    action = new MultiUndoAction(new[] { customActionBefore, action });
                }
                else if (customActionAfter != null)
                {
                    action = new MultiUndoAction(new[] { action, customActionAfter });
                }

                UndoOperationsStack.Push(action);
                OnAction(action);
            }
        }

        /// <summary>
        /// Creates new undo action for provided instance of object.
        /// </summary>
        /// <param name="snapshotInstance">Instance of an object to record</param>
        /// <param name="actionString">Name of action to be displayed in undo stack.</param>
        /// <param name="actionsToSave">Action in after witch recording will be finished.</param>
        public void RecordAction(object snapshotInstance, string actionString, Action actionsToSave)
        {
            RecordBegin(snapshotInstance, actionString);
            actionsToSave?.Invoke();
            RecordEnd(snapshotInstance);
        }

        /// <summary>
        /// Creates new undo action for provided instance of object.
        /// </summary>
        /// <param name="snapshotInstance">Instance of an object to record</param>
        /// <param name="actionString">Name of action to be displayed in undo stack.</param>
        /// <param name="actionsToSave">Action in after witch recording will be finished.</param>
        public void RecordAction<T>(T snapshotInstance, string actionString, Action<T> actionsToSave)
        where T : new()
        {
            RecordBegin(snapshotInstance, actionString);
            actionsToSave?.Invoke(snapshotInstance);
            RecordEnd(snapshotInstance);
        }

        /// <summary>
        /// Creates new undo action for provided instance of object.
        /// </summary>
        /// <param name="snapshotInstance">Instance of an object to record</param>
        /// <param name="actionString">Name of action to be displayed in undo stack.</param>
        /// <param name="actionsToSave">Action in after witch recording will be finished.</param>
        public void RecordAction(object snapshotInstance, string actionString, Action<object> actionsToSave)
        {
            RecordBegin(snapshotInstance, actionString);
            actionsToSave?.Invoke(snapshotInstance);
            RecordEnd(snapshotInstance);
        }

        /// <summary>
        /// Adds the action to the history.
        /// </summary>
        /// <param name="action">The action.</param>
        public void AddAction(IUndoAction action)
        {
            if (action == null)
                throw new ArgumentNullException();
            if (!Enabled)
                return;

            UndoOperationsStack.Push(action);
            OnAction(action);
        }

        /// <summary>
        /// Undo last recorded action
        /// </summary>
        public void PerformUndo()
        {
            if (!Enabled || !CanUndo)
                return;

            var action = (IUndoAction)UndoOperationsStack.PopHistory();
            action.Undo();

            OnUndo(action);
        }

        /// <summary>
        /// Redo last undone action
        /// </summary>
        public void PerformRedo()
        {
            if (!Enabled || !CanRedo)
                return;

            var action = (IUndoAction)UndoOperationsStack.PopReverse();
            action.Do();

            OnRedo(action);
        }

        /// <summary>
        /// Called when <see cref="Undo"/> performs action.
        /// </summary>
        /// <param name="action">The action.</param>
        protected virtual void OnAction(IUndoAction action)
        {
            ActionDone?.Invoke(action);
        }

        /// <summary>
        /// Called when <see cref="Undo"/> performs undo action.
        /// </summary>
        /// <param name="action">The action.</param>
        protected virtual void OnUndo(IUndoAction action)
        {
            UndoDone?.Invoke(action);
        }

        /// <summary>
        /// Called when <see cref="Undo"/> performs redo action.
        /// </summary>
        /// <param name="action">The action.</param>
        protected virtual void OnRedo(IUndoAction action)
        {
            RedoDone?.Invoke(action);
        }

        /// <summary>
        /// Clears the history.
        /// </summary>
        public void Clear()
        {
            _snapshots.Clear();
            UndoOperationsStack.Clear();
        }

        /// <inheritdoc />
        public void Dispose()
        {
            UndoDone = null;
            RedoDone = null;
            ActionDone = null;

            Clear();
        }
    }
}
