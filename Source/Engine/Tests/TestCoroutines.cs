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
        var handle = executor.ExecuteOnce(
            new CoroutineBuilder()
                .ThenRun(CoroutineSuspendPoint.Update, () => { })
                .ThenWaitSeconds(1.0f)
                .ThenRun(CoroutineSuspendPoint.Update, () => { })
                .ThenWaitFrames(1)
                .ThenRun(CoroutineSuspendPoint.Update, () => { })
            );

        Assert.IsNotNull(handle);
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

        var handle1 = executor.ExecuteOnce(coroutineA);
        Assert.IsNotNull(handle1);

        var coroutineB = new CoroutineBuilder()
            .ThenRun(CoroutineSuspendPoint.Update, () => { })
            .ThenWaitSeconds(1.0f)
            .ThenRun(CoroutineSuspendPoint.Update, () => { })
            .ThenWaitFrames(1)
            .ThenRun(CoroutineSuspendPoint.Update, () => { });

        var handle2 = executor.ExecuteOnce(coroutineB);
        Assert.IsNotNull(handle2);
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

        var handle = executor.ExecuteOnce(coroutine);
        Assert.IsNotNull(handle);

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

    [Test]
    public void TestCoroutineWaitForSuspensionPoint()
    {
        BoxedInt counter = new();
        CoroutineExecutor executor = new();

        var coroutine = new CoroutineBuilder()
            .ThenRun(CoroutineSuspendPoint.Update, () => { counter.Value++; })
            .ThenRun(CoroutineSuspendPoint.FixedUpdate, () => { counter.Value++; })
            .ThenRun(CoroutineSuspendPoint.Update, () => { counter.Value++; });

        var handle = executor.ExecuteOnce(coroutine);
        Assert.IsNotNull(handle);

        Assert.AreEqual(0, counter.Value);
        executor.Continue(CoroutineSuspendPoint.Update, 0.0f);
        executor.Continue(CoroutineSuspendPoint.Update, 0.0f);
        Assert.AreEqual(1, counter.Value);
        executor.Continue(CoroutineSuspendPoint.FixedUpdate, 0.0f);
        executor.Continue(CoroutineSuspendPoint.FixedUpdate, 0.0f);
        executor.Continue(CoroutineSuspendPoint.FixedUpdate, 0.0f);
        Assert.AreEqual(2, counter.Value);
        executor.Continue(CoroutineSuspendPoint.Update, 0.0f);
        executor.Continue(CoroutineSuspendPoint.Update, 0.0f);
        Assert.AreEqual(3, counter.Value);
    }

    [Test]
    public void TestCoroutineWaitForCondition()
    {
        BoxedInt counter = new();
        BoxedInt signal = new();
        CoroutineExecutor executor = new();

        var coroutine = new CoroutineBuilder()
            .ThenRun(CoroutineSuspendPoint.Update, () => { counter.Value++; })
            .ThenWaitUntil(() => signal.Value == 1)
            .ThenRun(CoroutineSuspendPoint.Update, () => { counter.Value++; });

        var handle = executor.ExecuteOnce(coroutine);
        Assert.IsNotNull(handle);

        Assert.AreEqual(0, counter.Value);
        executor.Continue(CoroutineSuspendPoint.Update, 0.0f);
        Assert.AreEqual(1, counter.Value);
        executor.Continue(CoroutineSuspendPoint.Update, 0.0f);
        Assert.AreEqual(1, counter.Value);
        signal.Value = 1;
        executor.Continue(CoroutineSuspendPoint.Update, 0.0f);
        Assert.AreEqual(2, counter.Value);
    }
}

#endif
