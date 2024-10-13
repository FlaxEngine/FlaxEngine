// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#if FLAX_TESTS

using System;
using NUnit.Framework;

namespace FlaxEngine.Tests;

[TestFixture]
public class TestCoroutines
{
    class BoxedInt { public int Value; }

    [Test]
    public void TestCoroutineBuilder()
    {
        CoroutineExecutor executor = new();
        executor.ExecuteOnce(
            new CoroutineBuilder()
                .ThenRun(CoroutineSuspendPoint.Update, () => { })
                .ThenWaitSeconds(1.0f)
                .ThenRun(CoroutineSuspendPoint.Update, () => { })
                .ThenWaitFrames(1)
                .ThenRun(CoroutineSuspendPoint.Update, () => { })
            );
    }

    [Test]
    public void TestCoroutineSwitching()
    {
        CoroutineExecutor executor = new();

        var coroutineA = new CoroutineBuilder()
            .ThenRun(CoroutineSuspendPoint.Update, () => { })
            .ThenWaitSeconds(1.0f)
            .ThenRun(CoroutineSuspendPoint.Update, () => { })
            .ThenWaitFrames(1)
            .ThenRun(CoroutineSuspendPoint.Update, () => { });

        executor.ExecuteOnce(coroutineA);
           
        var coroutineB = new CoroutineBuilder()
            .ThenRun(CoroutineSuspendPoint.Update, () => { })
            .ThenWaitSeconds(1.0f)
            .ThenRun(CoroutineSuspendPoint.Update, () => { })
            .ThenWaitFrames(1)
            .ThenRun(CoroutineSuspendPoint.Update, () => { });

        executor.ExecuteOnce(coroutineB);
    }

    [Test]
    public void TestCoroutineTimeAccumulation()
    {
        BoxedInt          counter  = new();
        CoroutineExecutor executor = new();

        var coroutine = new CoroutineBuilder()
            .ThenRun(CoroutineSuspendPoint.Update, () => { counter.Value++; })
            .ThenWaitSeconds(1.0f)
            .ThenRun(CoroutineSuspendPoint.Update, () => { counter.Value++; })
            .ThenWaitFrames(3)
            .ThenRun(CoroutineSuspendPoint.Update, () => { counter.Value++; });

        executor.ExecuteOnce(coroutine);

        Assert.AreEqual(0, counter.Value);
        executor.Continue(CoroutineSuspendPoint.Update, 0.0f); // Total 0.0s
        Assert.AreEqual(1, counter.Value);
        executor.Continue(CoroutineSuspendPoint.Update, 0.3f); // Total 0.3s
        Assert.AreEqual(1, counter.Value);
        executor.Continue(CoroutineSuspendPoint.Update, 0.3f); // Total 0.6s
        Assert.AreEqual(1, counter.Value);
        executor.Continue(CoroutineSuspendPoint.Update, 0.3f); // Total 0.9s
        Assert.AreEqual(1, counter.Value);
        executor.Continue(CoroutineSuspendPoint.Update, 0.3f); // Total 1.2s
        Assert.AreEqual(2, counter.Value);
    }

    //TODO Test waiting for a suspension point.
    //TODO Test waiting for a condition.
}

#endif
