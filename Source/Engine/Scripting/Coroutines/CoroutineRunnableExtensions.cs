// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine
{
    /// <summary>
    /// Utitlity class for coroutines managed scripting.
    /// </summary>
    public static class CoroutineExtensions
    {
        /// <summary>
        /// Wraps the arguments for <seealso cref="CoroutineBuilder.ThenRun(CoroutineRunnable)"/>.
        /// </summary>
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

        /// <summary>
        /// Wraps the arguments for <seealso cref="CoroutineBuilder.ThenWaitUntil(CoroutinePredicate)"/>.
        /// </summary>
        public static CoroutineBuilder ThenWaitUntil(
            this CoroutineBuilder builder,
            Func<bool> condition)
        {
            CoroutinePredicate predicate = new()
            {
                ExecutionPoint = CoroutineSuspendPoint.Update,
            };

            predicate.OnCheck += (ref bool result) => result = condition();

            builder.ThenWaitUntil(predicate);

            return builder;
        }
    }
}
