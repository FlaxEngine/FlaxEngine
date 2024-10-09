// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#if FLAX_TESTS

using System;
using NUnit.Framework;

namespace FlaxEngine.Tests;

[TestFixture]
public class TestCoroutines
{
    [Test]
    public void TestCoroutineBuilder()
    {
        CoroutineExecutor executor = new();
        executor.Execute(
            new CoroutineBuilder()
                .ThenRun(CoroutineSuspensionPointIndex.Update, () => { })
                .ThenWaitSeconds(1.0f)
                .ThenRun(CoroutineSuspensionPointIndex.Update, () => { })
                .ThenWaitFrames(1)
                .ThenRun(CoroutineSuspensionPointIndex.Update, () => { })
            );
    }
}

#endif
