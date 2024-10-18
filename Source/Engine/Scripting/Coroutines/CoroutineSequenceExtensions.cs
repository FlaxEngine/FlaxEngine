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
        /// Invokes <seealso cref="CoroutineSequence.ThenRun(CoroutineRunnable)"/> with object created using the provided action.
        /// </summary>
        public static CoroutineSequence ThenRun(
            this CoroutineSequence builder,
            Action run)
        {
            CoroutineRunnable runnable = new();
            runnable.OnRun += run;

            builder.ThenRun(runnable);

            return builder;
        }

        /// <summary>
        /// Invokes <seealso cref="CoroutineSequence.ThenWaitUntil(CoroutinePredicate)"/> with object created using the provided condition.
        /// </summary>
        public static CoroutineSequence ThenWaitUntil(
            this CoroutineSequence builder,
            Func<bool> condition)
        {
            CoroutinePredicate predicate = new();
            predicate.OnCheck += (ref bool result) => result = condition();

            builder.ThenWaitUntil(predicate);

            return builder;
        }
    }
}
