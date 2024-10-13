// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine
{
    public static class CoroutineExtensions
    {
        public static CoroutineBuilder ThenRun(
            this CoroutineBuilder builder,
            CoroutineSuspendPoint executionPoint,
            Action run)
        {
            CoroutineRunnable runnable = new()
            {
                ExecutionPoint = executionPoint
            };

            runnable.OnRun += run;

            builder.ThenRun(runnable);

            return builder;
        }
    }
}
