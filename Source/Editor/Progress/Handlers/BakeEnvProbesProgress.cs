// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.Progress.Handlers
{
    /// <summary>
    /// Environment probes baking progress reporting handler.
    /// </summary>
    /// <seealso cref="FlaxEditor.Progress.ProgressHandler" />
    public sealed class BakeEnvProbesProgress : ProgressHandler
    {
        private int _entriesDone;
        private int _entriesTotal;

        /// <summary>
        /// Initializes a new instance of the <see cref="BakeEnvProbesProgress"/> class.
        /// </summary>
        public BakeEnvProbesProgress()
        {
            Editor.EnvProbeBakeStart += OnEnvProbeBakeStart;
            Editor.EnvProbeBakeEnd += OnEnvProbeBakeEnd;
        }

        private void OnEnvProbeBakeStart(EnvironmentProbe environmentProbe)
        {
            // Check for start event
            if (_entriesTotal == 0)
            {
                _entriesDone = 0;
                OnStart();
            }

            _entriesTotal++;
            UpdateProgress();
        }

        private void OnEnvProbeBakeEnd(EnvironmentProbe environmentProbe)
        {
            _entriesDone++;
            UpdateProgress();

            // Check for end event
            if (_entriesDone >= _entriesTotal)
            {
                _entriesTotal = _entriesDone = 0;
                OnEnd();
            }
        }

        private void UpdateProgress()
        {
            float progress = (float)_entriesDone / _entriesTotal;
            OnUpdate(progress, "Baking environment probes...");
        }
    }
}
