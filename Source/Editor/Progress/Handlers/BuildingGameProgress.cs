// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEngine;

namespace FlaxEditor.Progress.Handlers
{
    /// <summary>
    /// Game building progress reporting handler.
    /// </summary>
    /// <seealso cref="FlaxEditor.Progress.ProgressHandler" />
    public sealed class BuildingGameProgress : ProgressHandler
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="BuildingGameProgress"/> class.
        /// </summary>
        public BuildingGameProgress()
        {
            GameCooker.Event += OnGameCookerEvent;
            GameCooker.Progress += OnGameCookerProgress;
        }

        private void OnGameCookerProgress(string info, float totalProgress)
        {
            if (IsActive)
            {
                if (string.IsNullOrEmpty(info))
                    info = "Building";
                OnUpdate(totalProgress, string.Format("{0} ({1}%)...", info, (int)(totalProgress * 100.0f)));
            }
        }

        private void OnGameCookerEvent(GameCooker.EventType eventType)
        {
            switch (eventType)
            {
            case GameCooker.EventType.BuildStarted:
                OnStart();
                OnUpdate(0, "Starting building game..");
                break;
            case GameCooker.EventType.BuildFailed:
                OnEnd();
                break;
            case GameCooker.EventType.BuildDone:
                OnEnd();
                break;
            default: throw new ArgumentOutOfRangeException(nameof(eventType), eventType, null);
            }
        }
    }
}
