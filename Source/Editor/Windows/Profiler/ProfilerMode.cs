// Copyright (c) Wojciech Figat. All rights reserved.

#if USE_PROFILER
using System;
using System.Collections.Generic;
using FlaxEditor.GUI;
using FlaxEditor.GUI.Tabs;
using FlaxEngine;

namespace FlaxEditor.Windows.Profiler
{
    /// <summary>
    /// Base class for all profiler modes. Implementation collects profiling events and presents it using dedicated UI.
    /// </summary>
    public class ProfilerMode : Tab
    {
        /// <summary>
        /// The shared data container for the profiler modes. Used to reduce calls to profiler tool backend for the same data across different profiler fronted modes.
        /// </summary>
        public struct SharedUpdateData
        {
            private ProfilingTools.ThreadStats[] _cpuEvents;
            private ProfilerGPU.Event[] _gpuEvents;

            /// <summary>
            /// The main stats. Gathered by auto by profiler before profiler mode update.
            /// </summary>
            public ProfilingTools.MainStats Stats;

            /// <summary>
            /// The additional managed memory allocation size during this update. Given value is in bytes.
            /// </summary>
            public int ManagedMemoryAllocation;

            /// <summary>
            /// Gets the collected CPU events by the profiler from local or remote session.
            /// </summary>
            /// <returns>Buffer with events per thread.</returns>
            public ProfilingTools.ThreadStats[] GetEventsCPU()
            {
                return _cpuEvents ?? (_cpuEvents = ProfilingTools.EventsCPU);
            }

            /// <summary>
            /// Gets the collected GPU events by the profiler from local or remote session.
            /// </summary>
            /// <returns>Buffer with rendering events.</returns>
            public ProfilerGPU.Event[] GetEventsGPU()
            {
                return _gpuEvents ?? (_gpuEvents = ProfilingTools.EventsGPU);
            }

            /// <summary>
            /// Begins the data usage. Prepares the container.
            /// </summary>
            public void Begin()
            {
                Stats = ProfilingTools.Stats;
                _cpuEvents = null;
                _gpuEvents = null;
            }

            /// <summary>
            /// Ends the data usage. Cleanups the container.  
            /// </summary>
            public void End()
            {
                _cpuEvents = null;
                _gpuEvents = null;
            }
        }

        /// <summary>
        /// The maximum amount of samples to collect.
        /// </summary>
        public const int MaxSamples = 60 * 10;

        /// <summary>
        /// The minimum event time in ms.
        /// </summary>
        public const double MinEventTimeMs = 0.000000001;

        /// <summary>
        /// Occurs when selected sample gets changed. Profiling window should propagate this change to all charts and view modes.
        /// </summary>
        public event Action<int> SelectedSampleChanged;

        /// <inheritdoc />
        public ProfilerMode(string text)
        : base(text)
        {
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            Clear();

            base.OnDestroy();
        }

        /// <summary>
        /// Initializes this instance.
        /// </summary>
        public virtual void Init()
        {
        }

        /// <summary>
        /// Clears this instance.
        /// </summary>
        public virtual void Clear()
        {
        }

        /// <summary>
        /// Updates this instance. Called every frame if live recording is enabled.
        /// </summary>
        /// <param name="sharedData">The shared data.</param>
        public virtual void Update(ref SharedUpdateData sharedData)
        {
        }

        /// <summary>
        /// Tick called before updating any modes display. Can be used to update per-frame statistics data.
        /// </summary>
        public virtual void UpdateStats()
        {
        }

        /// <summary>
        /// Updates the mode view. Called after init and on selected frame changed.
        /// </summary>
        /// <param name="selectedFrame">The selected frame index.</param>
        /// <param name="showOnlyLastUpdateEvents">True if show only events that happened during the last engine update (excluding events from fixed update or draw event), otherwise show all collected events.</param>
        public virtual void UpdateView(int selectedFrame, bool showOnlyLastUpdateEvents)
        {
        }

        /// <summary>
        /// Called when selected sample gets changed.
        /// </summary>
        /// <param name="frameIndex">Index of the view frame.</param>
        protected virtual void OnSelectedSampleChanged(int frameIndex)
        {
            SelectedSampleChanged?.Invoke(frameIndex);
        }

        /// <summary>
        /// Recycles all table rows to be reused.
        /// </summary>
        /// <param name="table">The table.</param>
        /// <param name="rowsCache">The output cache.</param>
        protected static void RecycleTableRows(Table table, List<Row> rowsCache)
        {
            int idx = 0;
            while (table.Children.Count > idx)
            {
                var child = table.Children[idx];
                if (child is Row row)
                {
                    rowsCache.Add(row);
                    child.Parent = null;
                }
                else
                {
                    idx++;
                }
            }
        }
    }
}
#endif
