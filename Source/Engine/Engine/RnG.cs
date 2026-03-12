using System;
using System.Collections.Generic;
using System.Diagnostics.CodeAnalysis;
using System.Numerics;
using System.Runtime.CompilerServices;
using System.Threading;

namespace FlaxEngine;

/// <summary>
/// Provides thread-safe utilities for generating random values, selecting collection elements, and managing state with various distribution biases.
/// </summary>
public static class Rng
{
    /// <summary>
    /// Represents a snapshot of the random state at a given point in time.
    /// </summary>
    public readonly struct State
    {
        /// <summary>
        /// Gets a signed 32-bit integer representing the current state of the random number generator.
        /// </summary>
        public int Integer { get; }

        /// <summary>
        /// Gets a single-precision floating-point number greater than or equal to 0.0 and less than 1.0.
        /// </summary>
        public float Float => Rng.Float(Integer);

        internal State(int value) => Integer = value;

        /// <inheritdoc cref="Rng.Condition(Chance)"/>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool Condition(Chance chance = Chance.Even) => Rng.Condition(Integer, chance);

        /// <inheritdoc cref="Rng.Fluctuate{T}(T, T)"/>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public T Fluctuate<T>(T value, T maxDeviation) where T : INumberBase<T> => Rng.Fluctuate(value, maxDeviation, Float);

        /// <inheritdoc cref="Rng.UniformRange{T}(T, T)"/>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public T UniformRange<T>(T min, T max) where T : INumberBase<T> => Range(min, max, Float);

        /// <inheritdoc cref="Rng.MedianBiasedRange{T}(T, T)"/>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public T MedianBiasedRange<T>(T min, T max) where T : INumberBase<T> => Rng.MedianBiasedRange(min, max, Float);

        /// <inheritdoc cref="Rng.ExtremesBiasedRange{T}(T, T)"/>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public T ExtremesBiasedRange<T>(T min, T max) where T : INumberBase<T> => Rng.ExtremesBiasedRange(min, max, Float);

        /// <inheritdoc cref="Rng.MaxBiasedRange{T}(T, T)"/>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public T MaxBiasedRange<T>(T min, T max) where T : INumberBase<T> => Rng.MaxBiasedRange(min, max, Float);

        /// <inheritdoc cref="Rng.MinBiasedRange{T}(T, T)"/>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public T MinBiasedRange<T>(T min, T max) where T : INumberBase<T> => Rng.MinBiasedRange(min, max, Float);

        /// <inheritdoc cref="Rng.TryChoose{T}(ReadOnlySpan{T}, out T)"/>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool TryChoose<T>(ReadOnlySpan<T> items, [MaybeNullWhen(false)] out T result) => Rng.TryChoose(items, Float, out result);

        /// <inheritdoc cref="Rng.TryChoose{T}(IList{T}, out T)"/>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool TryChoose<T>(IList<T> items, [MaybeNullWhen(false)] out T result) => Rng.TryChoose(items, Float, out result);

        /// <inheritdoc cref="Rng.TryChoose{T}(IReadOnlyList{T}, out T)"/>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool TryChoose<T>(IReadOnlyList<T> items, [MaybeNullWhen(false)] out T result) => Rng.TryChoose(items, Float, out result);

        /// <inheritdoc cref="Rng.Choose{T}(ReadOnlySpan{T})"/>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public T Choose<T>(ReadOnlySpan<T> items) => Rng.Choose(items, Float);

        /// <inheritdoc cref="Rng.Choose{T}(IList{T})"/>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public T Choose<T>(IList<T> items) => Rng.Choose(items, Float);

        /// <inheritdoc cref="Rng.Choose{T}(IReadOnlyList{T})"/>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public T Choose<T>(IReadOnlyList<T> items) => Rng.Choose(items, Float);
    }

    private const string EmptyCollectionMessage = "The collection must contain at least one item.";

    private static int _sharedState = Environment.TickCount;
    /// <summary>
    /// Gets or sets the shared state used for random number generation across all threads.
    /// </summary>
    /// <remarks>
    /// This state is modified atomically to ensure thread safety when accessed concurrently.
    /// </remarks>
    public static int SharedState
    {
        get => Volatile.Read(ref _sharedState);
        set => Interlocked.Exchange(ref _sharedState, value);
    }

    [ThreadStatic]
    private static int _localState;

    [ThreadStatic]
    private static bool _initialized;

    /// <summary>
    /// Captures a snapshot of the current random state for the calling thread, allowing for consistent random value generation based on that state.
    /// </summary>
    /// <returns>A new instance of <see cref="State"/> representing the current random state.</returns>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static State Snapshot() => new(Integer());

    /// <summary>
    /// Generates a pseudo-random floating-point number and updates the local state for the current thread.
    /// </summary>
    /// <returns>A single-precision floating-point value greater than or equal to 0.0 and less than 1.0.</returns>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static float Float() => Float(Integer());
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    private static float Float(int state)
    {
        uint bits = unchecked((uint)state >> 9 | 0x3F800000U);
        return Unsafe.As<uint, float>(ref bits) - 1.0f;
    }

    /// <summary>
    /// Generates a random floating-point value that follows a Gaussian (normal) distribution 
    /// and updates the local state for the current thread.
    /// </summary>
    /// <param name="mean">The mean value around which the Gaussian distribution is centered. Defaults to 0.0.</param>
    /// <param name="standardDeviation">The standard deviation that determines the spread of the distribution. Defaults to 1.0.</param>
    /// <returns>A random float sampled from the Gaussian distribution defined by the specified mean and standard deviation.</returns>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static float Gaussian(float mean = 0.0f, float standardDeviation = 1.0f)
    {
        return mean + Mathf.Sqrt(-2.0f * Mathf.Log(Float())) * Mathf.Cos(Mathf.TwoPi * Float()) * standardDeviation;
    }

    /// <summary>
    /// Generates a pseudo-random integer and updates the local state for the current thread.
    /// </summary>
    /// <returns>A 32-bit signed integer.</returns>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static int Integer()
    {
        if (!_initialized)
        {
            _initialized = true;
            _localState = Interlocked.Add(ref _sharedState, 1_013_904_223);
        }
        _localState = AdvanceState((uint)_localState);
        return _localState;
    }

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    internal static int AdvanceState(uint state)
    {
        return unchecked((int)(state * 1_664_525U + 1_013_904_223U));
    }

    /// <summary>
    /// Generates a pseudo-random, three-dimensional vector and updates the local state for the current thread.
    /// </summary>
    /// <returns>
    /// A new <see cref="Float3"/> instance with a length of 1, where each component is a random 
    /// floating-point value.
    /// </returns>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static Float3 Vector3()
    {
        Float3 vector = (new Float3(Float(), Float(), Float()) * 2.0f) - Float3.One;
        vector.Normalize();
        return vector;
    }

    /// <summary>
    /// Generates a pseudo-random, two-dimensional vector and updates the local state for the current thread.
    /// </summary>
    /// <returns>
    /// A new <see cref="Float2"/> instance with a length of 1, where each component is a random 
    /// floating-point value.
    /// </returns>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static Float2 Vector2()
    {
        Float2 vector = (new Float2(Float(), Float()) * 2.0f) - Float2.One;
        vector.Normalize();
        return vector;
    }

    /// <summary>
    /// Generates a pseudo-random color and updates the local state for the current thread.
    /// </summary>
    /// <returns>A <see cref="Color"/> with random hue, saturation, and value components.</returns>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static Color ColorHSV() => ColorHSV(Float(), Float());

    /// <returns>A <see cref="Color"/> with a random hue and value, but a specified saturation.</returns>
    /// <inheritdoc cref="ColorHSV()"/>
    /// <inheritdoc cref="Color.FromHSV(float, float, float, float)"/>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static Color ColorHSV(float saturation) => ColorHSV(saturation, Float());

    /// <returns>A <see cref="Color"/> with a random hue, the specified saturation and value.</returns>
    /// <inheritdoc cref="ColorHSV()"/>
    /// <inheritdoc cref="Color.FromHSV(float, float, float, float)"/>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static Color ColorHSV(float saturation, float value) => Color.FromHSV(Float() * 360.0f, saturation, value);

    /// <summary>
    /// Attempts to select a random <typeparamref name="T"/> from the specified collection using 
    /// a uniformly distributed random float value.
    /// </summary>
    /// <typeparam name="T">The type of the elements contained in the span.</typeparam>
    /// <param name="items">The collection of items from which to select a random element.</param>
    /// <param name="result">
    /// When the method returns <see langword="true"/>, contains the selected item; otherwise, is set to the default
    /// value for type <typeparamref name="T"/>.</param>
    /// <returns><see langword="true"/> if an item was successfully chosen; otherwise, <see langword="false"/>.</returns>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static bool TryChoose<T>(ReadOnlySpan<T> items, [MaybeNullWhen(false)] out T result) => TryChoose(items, Float(), out result);
    private static bool TryChoose<T>(ReadOnlySpan<T> items, float value, [MaybeNullWhen(false)] out T result)
    {
        if (items.IsEmpty)
        {
            result = default;
            return false;
        }

        result = items[Range(0, items.Length, value)];
        return true;
    }

    /// <inheritdoc cref="TryChoose{T}(ReadOnlySpan{T}, out T)"/>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static bool TryChoose<T>(IList<T> items, [MaybeNullWhen(false)] out T result) => TryChoose(items, Float(), out result);
    private static bool TryChoose<T>(IList<T> items, float value, [MaybeNullWhen(false)] out T result)
    {
        if (items is null or { Count: 0 })
        {
            result = default;
            return false;
        }

        result = items[Range(0, items.Count, value)];
        return true;
    }

    /// <inheritdoc cref="TryChoose{T}(ReadOnlySpan{T}, out T)"/>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static bool TryChoose<T>(IReadOnlyList<T> items, [MaybeNullWhen(false)] out T result) => TryChoose(items, Float(), out result);
    private static bool TryChoose<T>(IReadOnlyList<T> items, float value, [MaybeNullWhen(false)] out T result)
    {
        if (items is null or { Count: 0 })
        {
            result = default;
            return false;
        }

        result = items[Range(0, items.Count, value)];
        return true;
    }

    /// <summary>
    /// Selects a random <typeparamref name="T"/> from the specified collection using a uniformly distributed random float value.
    /// </summary>
    /// <typeparam name="T">The type of elements contained in the collection.</typeparam>
    /// <param name="items">The collection of items from which to select a random element. Cannot be <see langword="null"/> or empty.</param>
    /// <returns>A randomly selected <typeparamref name="T"/> from the collection.</returns>
    /// <exception cref="ArgumentException">Thrown when the <paramref name="items"/> collection is empty.</exception>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static T Choose<T>(ReadOnlySpan<T> items) => Choose(items, Float());
    private static T Choose<T>(ReadOnlySpan<T> items, float value)
    {
        return items.IsEmpty ? throw new ArgumentException(EmptyCollectionMessage, nameof(items)) : items[Range(0, items.Length, value)];
    }

    /// <inheritdoc cref="Choose{T}(ReadOnlySpan{T})"/>
    /// <exception cref="ArgumentNullException">Thrown when the <paramref name="items"/> collection is <see langword="null"/>.</exception>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static T Choose<T>(IList<T> items) => Choose(items, Float());
    private static T Choose<T>(IList<T> items, float value)
    {
        int count = items?.Count ?? throw new ArgumentNullException(nameof(items));
        return count > 0 ? items[Range(0, items.Count, value)] : throw new ArgumentException(EmptyCollectionMessage, nameof(items));
    }

    /// <inheritdoc cref="Choose{T}(ReadOnlySpan{T})"/>
    /// <exception cref="ArgumentNullException">Thrown when the <paramref name="items"/> collection is <see langword="null"/>.</exception>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static T Choose<T>(IReadOnlyList<T> items) => Choose(items, Float());
    private static T Choose<T>(IReadOnlyList<T> items, float value)
    {
        int count = items?.Count ?? throw new ArgumentNullException(nameof(items));
        return count > 0 ? items[Range(0, items.Count, value)] : throw new ArgumentException(EmptyCollectionMessage, nameof(items));
    }

    /// <summary>
    /// Evaluates a probabilistic condition based on the specified <paramref name="chance"/>.
    /// </summary>
    /// <param name="chance">
    /// The probability of the condition being <see langword="true"/>. 
    /// Defaults to <see cref="Chance.Even"/> if not specified.
    /// </param>
    /// <returns>
    /// <see langword="true"/> if the condition is met according to the evaluated chance; 
    /// otherwise, <see langword="false"/>.
    /// </returns>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static bool Condition(Chance chance = Chance.Even) => Condition(Integer(), chance);
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    private static bool Condition(int value, Chance chance) => unchecked((uint)value) < (uint)chance;

    /// <summary>
    /// Applies a random fluctuation to the specified value within the range defined by the maximum deviation.
    /// </summary>
    /// <param name="value">The base value to which the random fluctuation will be applied.</param>
    /// <param name="maxDeviation">The maximum allowable deviation from the base value. Determines the range within which the value may fluctuate.</param>
    /// <returns>A value that is randomly adjusted from the original value by an amount within the specified maximum deviation.</returns>
    /// <typeparam name="T">Specifies the numeric type of the value and maximum deviation. Must implement <see cref="INumberBase{TSelf}"/>.</typeparam>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static T Fluctuate<T>(T value, T maxDeviation) where T : INumberBase<T> => Fluctuate(value, maxDeviation, Float());
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    private static T Fluctuate<T>(T value, T maxDeviation, float state) where T : INumberBase<T>
    {
        return value + (T.CreateTruncating(state.ToSignedRange()) * maxDeviation);
    }

    /// <summary>
    /// Randomly rearranges the elements within the specified collection in place.
    /// </summary>
    /// <typeparam name="T">The type of elements contained in the collection.</typeparam>
    /// <param name="items">A span of items to shuffle. The span must contain at least one element.</param>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static void Shuffle<T>(this Span<T> items)
    {
        for (int i = items.Length - 1; i > 0; i--)
        {
            int j = Range(0, i + 1, Float());
            (items[i], items[j]) = (items[j], items[i]);
        }
    }

    /// <inheritdoc cref="Shuffle{T}(Span{T})"/>
    /// <exception cref="ArgumentNullException">Thrown when the <paramref name="items"/> collection is <see langword="null"/>.</exception>
    public static void Shuffle<T>(this IList<T> items)
    {
        ArgumentNullException.ThrowIfNull(items);
        for (int i = items.Count - 1; i > 0; i--)
        {
            int j = Range(0, i + 1, Float());
            (items[i], items[j]) = (items[j], items[i]);
        }
    }

    /// <summary>
    /// Generates a random <typeparamref name="T"/> that is uniformly distributed within the specified range.
    /// </summary>
    /// <typeparam name="T">
    /// The type of the values defining the range. 
    /// Must implement <see cref="INumberBase{TSelf}"/>.
    /// </typeparam>
    /// <param name="min">
    /// The inclusive lower bound of the range. 
    /// The generated value will be greater than or equal to this value.
    /// </param>
    /// <param name="max">
    /// The exclusive upper bound of the range. 
    /// The generated value will be less than this value.
    /// </param>
    /// <returns>
    /// A random <typeparamref name="T"/> that is greater than or equal to <paramref name="min"/> 
    /// and less than <paramref name="max"/>.
    /// </returns>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static T UniformRange<T>(T min, T max) where T : INumberBase<T>
    {
        return Range(min, max, Float());
    }

    /// <summary>
    /// Generates a random <typeparamref name="T"/> that is biased towards the median of the specified range.
    /// </summary>
    /// <inheritdoc cref="UniformRange{T}(T, T)"/>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static T MedianBiasedRange<T>(T min, T max) where T : INumberBase<T> => MedianBiasedRange(min, max, Float());
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    private static T MedianBiasedRange<T>(T min, T max, float seed) where T : INumberBase<T>
    {
        float s = seed.ToSignedRange();
        float biasedSeed = s * Mathf.Abs(s);
        float normalizedSeed = (biasedSeed + 1.0f) * 0.5f;
        return Range(min, max, normalizedSeed);
    }

    /// <summary>
    /// Generates a random <typeparamref name="T"/> that is biased towards the extremes (minimum and maximum) of the specified range.
    /// </summary>
    /// <inheritdoc cref="UniformRange{T}(T, T)"/>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static T ExtremesBiasedRange<T>(T min, T max) where T : INumberBase<T> => ExtremesBiasedRange(min, max, Float());
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    private static T ExtremesBiasedRange<T>(T min, T max, float state) where T : INumberBase<T>
    {
        float s = state.ToSignedRange();
        float abs = Mathf.Abs(s);
        float outward = 1.0f - ((1.0f - abs) * (1.0f - abs));
        float biasedSeed = MathF.CopySign(outward, s);
        float normalizedSeed = (biasedSeed + 1.0f) * 0.5f;
        return Range(min, max, normalizedSeed);
    }

    /// <summary>
    /// Generates a random <typeparamref name="T"/> that is biased towards the maximum value of the specified range.
    /// </summary>
    /// <inheritdoc cref="UniformRange{T}(T, T)"/>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static T MaxBiasedRange<T>(T min, T max) where T : INumberBase<T> => MaxBiasedRange(min, max, Float());
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    private static T MaxBiasedRange<T>(T min, T max, float state) where T : INumberBase<T>
    {
        return Range(max, min, state * state);
    }

    /// <summary>
    /// Generates a random <typeparamref name="T"/> that is biased towards the minimum value of the specified range.
    /// </summary>
    /// <inheritdoc cref="UniformRange{T}(T, T)"/>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static T MinBiasedRange<T>(T min, T max) where T : INumberBase<T> => MinBiasedRange(min, max, Float());
    private static T MinBiasedRange<T>(T min, T max, float state) where T : INumberBase<T>
    {
        return Range(min, max, state * state);
    }

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    private static T Range<T>(T min, T max, float state) where T : INumberBase<T>
    {
        double dMin = double.CreateTruncating(min);
        double dMax = double.CreateTruncating(max);
        double result = dMin + (state * (dMax - dMin));
        return T.CreateTruncating(result);
    }

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    private static float ToSignedRange(this float state) => (state * 2.0f) - 1.0f;
}
