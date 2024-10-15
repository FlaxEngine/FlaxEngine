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
        /// Invokes <seealso cref="CoroutineBuilder.ThenRun(CoroutineRunnable)"/> with object created using the provided action.
        /// </summary>
        public static CoroutineBuilder ThenRun(
            this CoroutineBuilder builder,
            Action run)
        {
            CoroutineRunnable runnable = new();
            runnable.OnRun += run;

            builder.ThenRun(runnable);

            return builder;
        }

        /// <summary>
        /// Invokes <seealso cref="CoroutineBuilder.ThenWaitUntil(CoroutinePredicate)"/> with object created using the provided condition.
        /// </summary>
        public static CoroutineBuilder ThenWaitUntil(
            this CoroutineBuilder builder,
            Func<bool> condition)
        {
            CoroutinePredicate predicate = new();
            predicate.OnCheck += (ref bool result) => result = condition();

            builder.ThenWaitUntil(predicate);

            return builder;
        }
    }
}
