using System;
using System.Collections.Generic;
using System.Diagnostics.CodeAnalysis;
using System.Numerics;
using System.Runtime.CompilerServices;

namespace FlaxEngine;

/// <summary>
/// Provides thread-safe utilities for generating random values, selecting collection elements, and managing state with various distribution biases.
/// </summary>
public static class Random
{
    private readonly struct EnumValueProvider<T> where T : struct, Enum
    {
        private static readonly T[] _values = System.Enum.GetValues<T>();
        public static T Normalize(uint random) => _values[random % (uint)_values.Length];
    }

    private const string EmptyCollectionMessage = "The collection must contain at least one item.";

    [ThreadStatic]
    private static Seed _threadLocal;
    /// <summary>
    /// Gets the default <see cref="Seed"/> instance for the current thread.
    /// </summary>
    public static Seed CurrentSeed => _threadLocal;

    /// <summary>
    /// Generates a pseudo-random floating-point number and updates the local state for the current thread.
    /// </summary>
    /// <remarks>This method may advance the local <see cref="Seed"/> for the current thread to the next state in the sequence.</remarks>
    /// <returns>A new <see cref="float"/> with a value greater than or equal to 0.0 and less than 1.0.</returns>
    public static float Float() => (float)++_threadLocal;

    /// <remarks>This method may advance the local <see cref="Seed"/> for the current thread to the next state in the sequence.</remarks>
    /// <inheritdoc cref="Gaussian(ref Seed, float, float)"/>
    public static float Gaussian(float mean = 0.0f, float standardDeviation = 1.0f) => Gaussian(ref _threadLocal, mean, standardDeviation);

    /// <summary>
    /// Generates a random floating-point value that follows a Gaussian (normal) distribution.
    /// </summary>
    /// <param name="mean">The mean value around which the Gaussian distribution is centered. Defaults to 0.0.</param>
    /// <param name="standardDeviation">The standard deviation that determines the spread of the distribution. Defaults to 1.0.</param>
    /// <returns>
    /// A new <see cref="float"/> sampled from the Gaussian distribution defined by the specified <paramref name="mean"/> 
    /// and <paramref name="standardDeviation"/>.
    /// </returns>
    /// <inheritdoc cref="Vector3(ref Seed)"/>
    /// <param name="seed"/>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static float Gaussian(ref Seed seed, float mean = 0.0f, float standardDeviation = 1.0f)
    {
        return mean + Mathf.Sqrt(-2.0f * Mathf.Log((float)++seed)) * Mathf.Cos(Mathf.TwoPi * (float)++seed) * standardDeviation;
    }

    /// <summary>Generates a pseudo-random integer.</summary>
    /// <remarks>This method may advance the local <see cref="Seed"/> for the current thread to the next state in the sequence.</remarks>
    /// <returns>A new 32-bit signed integer set to a random value.</returns>
    public static int Integer() => (int)++_threadLocal;

    /// <remarks>This method may advance the local <see cref="Seed"/> for the current thread to the next state in the sequence.</remarks>
    /// <inheritdoc cref="Long(ref Seed)"/>
    public static long Long() => Long(ref _threadLocal);

    /// <summary>Generates a pseudo-random integer.</summary>
    /// <returns>A new 64-bit signed integer set to a random value.</returns>
    /// <inheritdoc cref="Vector3(ref Seed)"/>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static long Long(ref Seed seed) => ((int)++seed << 32) | (long)++seed;

    /// <returns>A new 16-bit signed integer set to a random value.</returns>
    /// <inheritdoc cref="Integer()"/>
    public static short Short() => (short)++_threadLocal;

    /// <returns>A new 8-bit unsigned integer set to a random value.</returns>
    /// <inheritdoc cref="Integer()"/>
    public static byte Byte() => (byte)++_threadLocal;

    /// <remarks>This method may advance the local <see cref="Seed"/> for the current thread to the next state in the sequence.</remarks>
    /// <inheritdoc cref="Enum{T}(Seed)"/>
    public static T Enum<T>() where T : unmanaged, Enum => Enum<T>(++_threadLocal);

    /// <summary>
    /// Generates a pseudo-random value of the specified enum type.
    /// </summary>
    /// <typeparam name="T">
    /// The enum type for which to generate a random value. Must be an enum.
    /// </typeparam>
    /// <returns>A new <typeparamref name="T"/> set to a random value.</returns>
    /// <inheritdoc cref="Condition(Seed, Chance)"/>
    public static T Enum<T>(Seed seed) where T : struct, Enum
    {
        return EnumValueProvider<T>.Normalize((uint)seed.Current);
    }

    /// <remarks>This method may advance the local <see cref="Seed"/> for the current thread to the next state in the sequence.</remarks>
    /// <inheritdoc cref="Rotation(ref Seed)"/>
    public static Quaternion Rotation() => Rotation(ref _threadLocal);

    /// <summary>
    /// Generates a pseudo-random rotation as a quaternion.
    /// </summary>
    /// <returns>A <see cref="Quaternion"/> representing a rotation derived from random values.</returns>
    /// <inheritdoc cref="Vector3(ref Seed)"/>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static Quaternion Rotation(ref Seed seed)
    {
        Float3 euler = (Vector3(ref seed) * 360.0f) - 180.0f;
        return Quaternion.Euler(euler);
    }

    /// <summary>
    /// Generates a pseudo-random <see cref="Float3"/>.
    /// </summary>
    /// <remarks>This method may advance the local <see cref="Seed"/> for the current thread to the next state in the sequence.</remarks>
    /// <inheritdoc cref="Vector3(ref Seed)"/>
    public static Float3 Vector3() => Vector3(ref _threadLocal);

    /// <summary>
    /// Generates a pseudo-random, three-dimensional vector.
    /// </summary>
    /// <remarks>
    /// This method may advance the value of the specified <paramref name="seed"/> to the 
    /// next state in the sequence.
    /// </remarks>
    /// <returns>
    /// A new <see cref="Float3"/>, where each component is a random floating-point value. 
    /// The vector is normalized to ensure it has a length of 1.
    /// </returns>
    /// <inheritdoc cref="Condition(Seed, Chance)"/>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static Float3 Vector3(ref Seed seed)
    {
        Float3 vector = (new Float3((float)++seed, (float)++seed, (float)++seed) * 2.0f) - Float3.One;
        vector.Normalize();
        return vector;
    }

    /// <remarks>This method may advance the local <see cref="Seed"/> for the current thread to the next state in the sequence.</remarks>
    /// <inheritdoc cref="Vector2(ref Seed)"/>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static Float2 Vector2() => Vector2(ref _threadLocal);

    /// <summary>
    /// Generates a pseudo-random, two-dimensional vector.
    /// </summary>
    /// <returns>
    /// A new <see cref="Float2"/>, where each component is a random floating-point value. 
    /// The vector is normalized to ensure it has a length of 1.
    /// </returns>
    /// <inheritdoc cref="Vector3(ref Seed)"/>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static Float2 Vector2(ref Seed seed)
    {
        Float2 vector = (new Float2((float)++seed, (float)++seed) * 2.0f) - Float2.One;
        vector.Normalize();
        return vector;
    }

    /// <remarks>This method may advance the local <see cref="Seed"/> for the current thread to the next state in the sequence.</remarks>
    /// <inheritdoc cref="ColorHSV(ref Seed)"/>
    public static Color ColorHSV() => ColorHSV(ref _threadLocal);

    /// <remarks>This method may advance the local <see cref="Seed"/> for the current thread to the next state in the sequence.</remarks>
    /// <inheritdoc cref="ColorHSV(ref Seed, float, float, float, float)"/>
    public static Color ColorHSV(float hue = -1.0f, float saturation = -1.0f, float value = -1.0f, float alpha = 1.0f)
    {
        return ColorHSV(ref _threadLocal, hue, saturation, value, alpha);
    }

    /// <summary>
    /// Generates a <see cref="Color"/> in the HSV color space using the specified 
    /// <paramref name="hue"/>, <paramref name="saturation"/>, and 
    /// <paramref name="value"/> components.
    /// </summary>
    /// <param name="hue">
    /// <para>The hue component of the color, specified in degrees from 0.0 to 360.0.</para>
    /// <para>A negative value will be replaced with a random hue.</para>
    /// </param>
    /// <param name="saturation">
    /// <para>The saturation component of the color, specified as a value from 0.0 to 1.0.</para>
    /// <para>A negative value will be replaced with a random saturation.</para>
    /// </param>
    /// <param name="value">
    /// <para>The value (brightness) component of the color, specified as a value from 0.0 to 1.0.</para>
    /// <para>A negative value will be replaced with a random value.</para>
    /// </param>
    /// <param name="alpha">
    /// <para>The alpha (opacity) component of the color, specified as a value from 0.0 to 1.0.</para>
    /// <para>A negative value will be replaced with a random alpha.</para>
    /// </param>
    /// <returns>A new <see cref="Color"/> with the specified <paramref name="hue"/>, 
    /// <paramref name="saturation"/>, and <paramref name="value"/> components.
    /// </returns>
    /// <inheritdoc cref="Vector3(ref Seed)"/>
    /// <param name="seed"/>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static Color ColorHSV(ref Seed seed, float hue = -1.0f, float saturation = -1.0f, float value = -1.0f, float alpha = 1.0f)
    {
        hue = hue < 0.0f ? Hue(ref seed) : hue;
        saturation = saturation < 0.0f ? (float)++seed : saturation;
        value = value < 0.0f ? (float)++seed : value;
        alpha = alpha < 0.0f ? (float)++seed : alpha;
        return Color.FromHSV(hue, saturation, value, alpha);
    }

    /// <summary>
    /// Generates a pseudo-random <see cref="Color"/> in the HSV color space.
    /// </summary>
    /// <returns>A <see cref="Color"/> with random hue, saturation, and value components.</returns>
    /// <inheritdoc cref="Vector3(ref Seed)"/>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static Color ColorHSV(ref Seed seed)
    {
        float hue = Hue(ref seed);
        float saturation = (float)++seed;
        float value = (float)++seed;
        return Color.FromHSV(hue, saturation, value);
    }

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    private static float Hue(ref Seed seed) => (float)++seed * 360.0f;

    /// <remarks>This method may advance the local <see cref="Seed"/> for the current thread to the next state in the sequence.</remarks>
    /// <inheritdoc cref="TryChoose{T}(Seed, ReadOnlySpan{T}, out T)"/>
    public static bool TryChoose<T>(ReadOnlySpan<T> items, [MaybeNullWhen(false)] out T result) => TryChoose(++_threadLocal, items, out result);

    /// <summary>
    /// Attempts to select a <typeparamref name="T"/> from <paramref name="items"/> using 
    /// a uniformly distributed, pseudo-random, floating-point number.
    /// </summary>
    /// <typeparam name="T">The type of the elements contained in the span.</typeparam>
    /// <param name="items">The collection of items from which to select a random element.</param>
    /// <param name="result">
    /// When the method returns <see langword="true"/>, contains the selected item; otherwise, is set to the default
    /// value for type <typeparamref name="T"/>.</param>
    /// <returns><see langword="true"/> if an item was successfully chosen; otherwise, <see langword="false"/>.</returns>
    /// <inheritdoc cref="Condition(Seed, Chance)"/>
    /// <param name="seed"/>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static bool TryChoose<T>(Seed seed, ReadOnlySpan<T> items, [MaybeNullWhen(false)] out T result)
    {
        if (items.IsEmpty)
        {
            result = default;
            return false;
        }

        result = items[UniformRange(++seed, items.Length)];
        return true;
    }

    /// <inheritdoc cref="TryChoose{T}(ReadOnlySpan{T}, out T)"/>
    public static bool TryChoose<T>(IList<T> items, [MaybeNullWhen(false)] out T result) => TryChoose(++_threadLocal, items, out result);

    /// <remarks>This method may advance the local <see cref="Seed"/> for the current thread to the next state in the sequence.</remarks>
    /// <inheritdoc cref="TryChoose{T}(Seed, IList{T}, int, int, out T)"/>
    public static bool TryChoose<T>(IList<T> items, int offset, int length, [MaybeNullWhen(false)] out T result)
    {
        return TryChoose(++_threadLocal, items, offset, length, out result);
    }

    /// <param name="offset">
    /// The zero-based index at which to start the selection range within 
    /// the <paramref name="items"/> collection.
    /// </param>
    /// <param name="length">
    /// The number of elements in the selection range starting from the 
    /// <paramref name="offset"/>.
    /// </param>
    /// <inheritdoc cref="TryChoose{T}(Seed, ReadOnlySpan{T}, out T)"/>
    /// <param name="seed"/>
    /// <param name="items"/>
    /// <param name="result"/>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static bool TryChoose<T>(Seed seed, IList<T> items, int offset, int length, [MaybeNullWhen(false)] out T result)
    {
        if (items is null or { Count: 0 } || !IsRangeWithinBounds(offset, length, items.Count, out int end))
        {
            result = default;
            return false;
        }

        result = items[UniformRange(seed, end, offset)];
        return true;
    }

    /// <inheritdoc cref="TryChoose{T}(Seed, ReadOnlySpan{T}, out T)"/>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static bool TryChoose<T>(Seed seed, IList<T> items, [MaybeNullWhen(false)] out T result)
    {
        if (items is null or { Count: 0 })
        {
            result = default;
            return false;
        }

        result = items[UniformRange(seed, items.Count)];
        return true;
    }

    /// <inheritdoc cref="TryChoose{T}(IList{T}, out T)"/>
    public static bool TryChoose<T>(IReadOnlyList<T> items, [MaybeNullWhen(false)] out T result)
    {
        return TryChoose(++_threadLocal, items, out result);
    }

    /// <inheritdoc cref="TryChoose{T}(IList{T}, int, int, out T)"/>
    public static bool TryChoose<T>(IReadOnlyList<T> items, int offset, int length, [MaybeNullWhen(false)] out T result)
    {
        return TryChoose(++_threadLocal, items, offset, length, out result);
    }

    /// <inheritdoc cref="TryChoose{T}(Seed, IList{T}, int, int, out T)"/>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static bool TryChoose<T>(Seed seed, IReadOnlyList<T> items, int offset, int length, [MaybeNullWhen(false)] out T result)
    {
        if (items is null or { Count: 0 } || !IsRangeWithinBounds(offset, length, items.Count, out int end))
        {
            result = default;
            return false;
        }

        result = items[UniformRange(seed, end, offset)];
        return true;
    }

    /// <inheritdoc cref="TryChoose{T}(Seed, ReadOnlySpan{T}, out T)"/>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static bool TryChoose<T>(Seed seed, IReadOnlyList<T> items, [MaybeNullWhen(false)] out T result)
    {
        if (items is null or { Count: 0 })
        {
            result = default;
            return false;
        }

        result = items[UniformRange(seed, items.Count)];
        return true;
    }

    /// <remarks>This method may advance the local <see cref="Seed"/> for the current thread to the next state in the sequence.</remarks>
    /// <inheritdoc cref="Choose{T}(Seed, ReadOnlySpan{T})"/>
    public static T Choose<T>(ReadOnlySpan<T> items) => Choose(++_threadLocal, items);

    /// <summary>
    /// Selects a <typeparamref name="T"/> from <paramref name="items"/> using 
    /// a uniformly distributed, pseudo-random, floating-point number.
    /// </summary>
    /// <typeparam name="T">The type of elements contained in the collection.</typeparam>
    /// <param name="items">The collection of items from which to select a random element. Cannot be empty.</param>
    /// <returns>A randomly selected <typeparamref name="T"/> from the collection.</returns>
    /// <exception cref="ArgumentException">Thrown when the <paramref name="items"/> collection is empty.</exception>
    /// <inheritdoc cref="Vector3(ref Seed)"/>
    /// <param name="seed"/>
    public static T Choose<T>(Seed seed, ReadOnlySpan<T> items)
    {
        return items.IsEmpty ? throw new ArgumentException(EmptyCollectionMessage, nameof(items)) : items[UniformRange(seed, items.Length)];
    }

    /// <remarks>This method may advance the local <see cref="Seed"/> for the current thread to the next state in the sequence.</remarks>
    /// <inheritdoc cref="Choose{T}(Seed, IList{T})"/>
    public static T Choose<T>(IList<T> items) => Choose(++_threadLocal, items);

    /// <remarks>This method may advance the local <see cref="Seed"/> for the current thread to the next state in the sequence.</remarks>
    /// <inheritdoc cref="Choose{T}(Seed, IList{T}, int, int)"/>
    public static T Choose<T>(IList<T> items, int offset, int length)
    {
        return Choose(++_threadLocal, items, offset, length);
    }

    /// <param name="offset">
    /// The zero-based index at which to start the selection range within 
    /// the <paramref name="items"/> collection.
    /// </param>
    /// <param name="length">
    /// The number of elements in the selection range starting from the 
    /// <paramref name="offset"/>.
    /// </param>
    /// <inheritdoc cref="Choose{T}(Seed, ReadOnlySpan{T})"/>
    /// <param name="seed"/>
    /// <param name="items"/>
    /// <exception cref="ArgumentNullException">Thrown when the <paramref name="items"/> collection is <see langword="null"/>.</exception>
    /// <exception cref="ArgumentOutOfRangeException">Thrown when the offset and length are out of bounds.</exception>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static T Choose<T>(Seed seed, IList<T> items, int offset, int length)
    {
        ThrowIfNullOrEmpty(items, out int count);
        ThrowIfOutOfRange(offset, length, count, out int end);
        return items[UniformRange(seed, end, offset)];
    }

    /// <inheritdoc cref="Choose{T}(Seed, ReadOnlySpan{T})"/>
    /// <exception cref="ArgumentNullException">Thrown when the <paramref name="items"/> collection is <see langword="null"/>.</exception>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static T Choose<T>(Seed seed, IList<T> items)
    {
        ThrowIfNullOrEmpty(items, out int count);
        return items[UniformRange(seed, count)];
    }

    /// <inheritdoc cref="Choose{T}(IList{T})"/>
    public static T Choose<T>(IReadOnlyList<T> items) => Choose(++_threadLocal, items);

    /// <inheritdoc cref="Choose{T}(IList{T}, int, int)"/>
    public static T Choose<T>(IReadOnlyList<T> items, int offset, int length) => Choose(++_threadLocal, items, offset, length);

    /// <inheritdoc cref="Choose{T}(Seed, IList{T}, int, int)"/>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static T Choose<T>(Seed seed, IReadOnlyList<T> items, int offset, int length)
    {
        ThrowIfNullOrEmpty(items, out int count);
        ThrowIfOutOfRange(offset, length, count, out int end);
        return items[UniformRange(seed, end, offset)];
    }

    /// <inheritdoc cref="Choose{T}(Seed, ReadOnlySpan{T})"/>
    /// <exception cref="ArgumentNullException">Thrown when the <paramref name="items"/> collection is <see langword="null"/>.</exception>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static T Choose<T>(Seed seed, IReadOnlyList<T> items)
    {
        ThrowIfNullOrEmpty(items, out int count);
        return items[UniformRange(seed, count)];
    }

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    private static bool IsRangeWithinBounds(int start, int length, int count, out int end)
    {
        uint uEnd = unchecked((uint)start + (uint)length);
        end = (int)uEnd;
        return uEnd <= count;
    }

    [DoesNotReturn, MethodImpl(MethodImplOptions.AggressiveInlining)]
    private static void ThrowIfOutOfRange(int start, int length, int count, out int end,
        [CallerArgumentExpression(nameof(start))] string startName = "",
        [CallerArgumentExpression(nameof(length))] string lengthName = "")
    {
        if (IsRangeWithinBounds(start, length, count, out end))
        {
            return;
        }

        string message = $"The specified {startName} and {lengthName} must define a valid range within the collection.";
        throw new ArgumentOutOfRangeException(lengthName, message);
    }

    [DoesNotReturn, MethodImpl(MethodImplOptions.AggressiveInlining)]
    private static void ThrowIfNullOrEmpty<T>([MaybeNull] IList<T> list, out int count, 
        [CallerArgumentExpression(nameof(list))] string parameterName = "")
    {
        count = list?.Count ?? throw new ArgumentNullException(parameterName);
        if (count == 0)
        {
            throw new ArgumentException(EmptyCollectionMessage, parameterName);
        }
    }

    [DoesNotReturn, MethodImpl(MethodImplOptions.AggressiveInlining)]
    private static void ThrowIfNullOrEmpty<T>([MaybeNull] IReadOnlyList<T> list, out int count, 
        [CallerArgumentExpression(nameof(list))] string parameterName = "")
    {
        count = list?.Count ?? throw new ArgumentNullException(parameterName);
        if (count == 0)
        {
            throw new ArgumentException(EmptyCollectionMessage, parameterName);
        }
    }

    /// <inheritdoc cref="Condition(Seed, Chance)"/>
    /// <inheritdoc cref="Integer()"/>
    public static bool Condition(Chance chance = Chance.Even) => Condition(++_threadLocal, chance);

    /// <summary>
    /// Evaluates a probabilistic condition based on the specified <paramref name="chance"/>.
    /// </summary>
    /// <param name="chance">
    /// The probability of the condition being <see langword="true"/>. 
    /// Defaults to <see cref="Chance.Even"/> if not specified.
    /// </param>
    /// <param name="seed">The <see cref="Seed"/> instance to use for generating the random value.</param>
    /// <returns>
    /// <see langword="true"/> if the condition is met according to the evaluated chance; 
    /// otherwise, <see langword="false"/>.
    /// </returns>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static bool Condition(Seed seed, Chance chance = Chance.Even) => unchecked((uint)seed.Current) < (uint)chance;

    /// <inheritdoc cref="Fluctuate{T}(Seed, T, T)"/>
    /// <inheritdoc cref="Integer()"/>
    public static T Fluctuate<T>(T value, T maxDeviation) where T : INumberBase<T> => Fluctuate(++_threadLocal, value, maxDeviation);

    /// <summary>
    /// Applies a random fluctuation to the specified value within the range defined by the maximum deviation.
    /// </summary>
    /// <param name="value">The base value to which the random fluctuation will be applied.</param>
    /// <param name="maxDeviation">The maximum allowable deviation from the base value. Determines the range within which the value may fluctuate.</param>
    /// <returns>A value that is randomly adjusted from the original value by an amount within the specified maximum deviation.</returns>
    /// <typeparam name="T">Specifies the numeric type of the value and maximum deviation. Must implement <see cref="INumberBase{TSelf}"/>.</typeparam>
    /// <inheritdoc cref="Condition(Seed, Chance)"/>
    /// <param name="seed"/>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static T Fluctuate<T>(Seed seed, T value, T maxDeviation) where T : INumberBase<T>
    {
        float factor = ToSignedRange((float)seed);
        return value + (T.CreateTruncating(factor) * maxDeviation);
    }


    /// <remarks>This method may advance the local <see cref="Seed"/> for the current thread to the next state in the sequence.</remarks>
    /// <inheritdoc cref="Shuffle{T}(Span{T}, ref Seed)"/>
    public static void Shuffle<T>(this Span<T> items) => Shuffle(items, ref _threadLocal);

    /// <summary>
    /// Randomly rearranges the elements within the specified collection in place.
    /// </summary>
    /// <typeparam name="T">The type of elements contained in the collection.</typeparam>
    /// <param name="items">A span of items to shuffle. The span must contain at least one element.</param>
    /// <returns/>
    /// <inheritdoc cref="Vector3(ref Seed)"/>
    /// <param name="seed"/>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static void Shuffle<T>(this Span<T> items, ref Seed seed)
    {
        for (int i = items.Length - 1; i > 0; i--)
        {
            int j = UniformRange(++seed, i + 1);
            (items[i], items[j]) = (items[j], items[i]);
        }
    }

    /// <remarks>This method may advance the local <see cref="Seed"/> for the current thread to the next state in the sequence.</remarks>
    /// <inheritdoc cref="Shuffle{T}(IList{T}, ref Seed)"/>
    public static void Shuffle<T>(this IList<T> items) => Shuffle(items, ref _threadLocal);

    /// <inheritdoc cref="Shuffle{T}(Span{T})"/>
    /// <returns/>
    /// <inheritdoc cref="Vector3(ref Seed)"/>
    /// <exception cref="ArgumentNullException">Thrown when the <paramref name="items"/> collection is <see langword="null"/>.</exception>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static void Shuffle<T>(this IList<T> items, ref Seed seed)
    {
        ArgumentNullException.ThrowIfNull(items);
        for (int i = items.Count - 1; i > 0; i--)
        {
            int j = UniformRange(++seed, i + 1);
            (items[i], items[j]) = (items[j], items[i]);
        }
    }

    /// <remarks>This method may advance the local <see cref="Seed"/> for the current thread to the next state in the sequence.</remarks>
    /// <inheritdoc cref="UniformRange{T}(Seed, T, T)"/>
    public static T UniformRange<T>(T min, T max) where T : IFloatingPoint<T>
    {
        return UniformRange(++_threadLocal, max, min);
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
    /// <inheritdoc cref="Condition(Seed, Chance)"/>
    /// <param name="seed"/>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static T UniformRange<T>(Seed seed, T max, T min = default) where T : IFloatingPoint<T>
    {
        return Range(min, max, T.CreateTruncating((float)seed));
    }

    /// <remarks>This method may advance the local <see cref="Seed"/> for the current thread to the next state in the sequence.</remarks>
    /// <inheritdoc cref="UniformRange(Seed, int, int)"/>
    public static int UniformRange(int max, int min = default) => UniformRange(++_threadLocal, max, min);

    /// <summary>
    /// Generates a pseudo-random integer that is uniformly distributed within the specified range.
    /// </summary>
    /// <returns>An integer greater than or equal to <paramref name="min"/> and less than <paramref name="max"/>.</returns>
    /// <exception cref="ArgumentOutOfRangeException">Thrown if <paramref name="min"/> is greater than or equal to <paramref name="max"/>.</exception>
    /// <inheritdoc cref="UniformRange{T}(Seed, T, T)"/>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static int UniformRange(Seed seed, int max, int min = default)
    {
        uint randomValue = unchecked((uint)seed.Current);
        return UniformRange(randomValue, min, max, 32);
    }

    /// <remarks>This method may advance the local <see cref="Seed"/> for the current thread to the next state in the sequence.</remarks>
    /// <inheritdoc cref="UniformRange(ref Seed, long, long)"/>
    public static long UniformRange(long max, long min = default) => UniformRange(ref _threadLocal, max, min);

    /// <inheritdoc cref="UniformRange(Seed, int, int)"/>
    /// <inheritdoc cref="Vector2(ref Seed)"/>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static long UniformRange(ref Seed seed, long max, long min = default)
    {
        ulong randomValue = unchecked((ulong)Long(ref seed));
        return UniformRange(randomValue, min, max, 64);
    }

    /// <remarks>This method may advance the local <see cref="Seed"/> for the current thread to the next state in the sequence.</remarks>
    /// <inheritdoc cref="UniformRange(Seed, short, short)"/>
    public static short UniformRange(short max, short min = default) => UniformRange(++_threadLocal, max, min);

    /// <inheritdoc cref="UniformRange(Seed, int, int)"/>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static short UniformRange(Seed seed, short max, short min = default)
    {
        ushort randomValue = unchecked((ushort)(short)seed);
        return UniformRange(randomValue, min, max, 16);
    }

    /// <remarks>This method may advance the local <see cref="Seed"/> for the current thread to the next state in the sequence.</remarks>
    /// <inheritdoc cref="UniformRange(Seed, byte, byte)"/>
    public static byte UniformRange(byte max, byte min = default) => UniformRange(++_threadLocal, max, min);

    /// <inheritdoc cref="UniformRange(Seed, int, int)"/>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static byte UniformRange(Seed seed, byte max, byte min = default)
    {
        byte randomValue = (byte)seed;
        return UniformRange<byte, ushort>(randomValue, min, max, 8);
    }

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    private static TValue UniformRange<TValue, TWidened>(TWidened random, TValue min, TValue max, int shift)
        where TValue : struct, IBinaryInteger<TValue>
        where TWidened : struct, IBinaryInteger<TWidened>
    {
        ArgumentOutOfRangeException.ThrowIfGreaterThanOrEqual(min, max);
        TWidened range = TWidened.CreateTruncating(++max - min);
        return min + TValue.CreateTruncating((random * range) >> shift);
    }

    /// <remarks>This method may advance the local <see cref="Seed"/> for the current thread to the next state in the sequence.</remarks>
    /// <inheritdoc cref="MedianBiasedRange{T}(T, T, Seed)"/>
    public static T MedianBiasedRange<T>(T min, T max) where T : IFloatingPoint<T>
    {
        return MedianBiasedRange(min, max, ++_threadLocal);
    }

    /// <summary>
    /// Generates a random <typeparamref name="T"/> that is biased towards the median of the specified range.
    /// </summary>
    /// <inheritdoc cref="UniformRange{T}(Seed, T, T)"/>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    private static T MedianBiasedRange<T>(T min, T max, Seed seed) where T : IFloatingPoint<T>
    {
        float s = ToSignedRange((float)seed);
        float biasedSeed = s * Mathf.Abs(s);
        T normalizedSeed = T.CreateTruncating((biasedSeed + 1.0f) * 0.5f);
        return Range(min, max, normalizedSeed);
    }

    /// <remarks>This method may advance the local <see cref="Seed"/> for the current thread to the next state in the sequence.</remarks>
    /// <inheritdoc cref="ExtremesBiasedRange{T}(T, T, Seed)"/>
    public static T ExtremesBiasedRange<T>(T min, T max) where T : IFloatingPoint<T> => ExtremesBiasedRange(min, max, ++_threadLocal);

    /// <summary>
    /// Generates a random <typeparamref name="T"/> that is biased towards the extremes (minimum and maximum) of the specified range.
    /// </summary>
    /// <inheritdoc cref="UniformRange{T}(Seed, T, T)"/>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static T ExtremesBiasedRange<T>(T min, T max, Seed seed) where T : IFloatingPoint<T>
    {
        float s = ToSignedRange((float)seed);
        float abs = Mathf.Abs(s);
        float outward = 1.0f - ((1.0f - abs) * (1.0f - abs));
        float biasedSeed = MathF.CopySign(outward, s);
        T normalizedSeed = T.CreateTruncating((biasedSeed + 1.0f) * 0.5f);
        return Range(min, max, normalizedSeed);
    }

    /// <remarks>This method may advance the local <see cref="Seed"/> for the current thread to the next state in the sequence.</remarks>
    /// <inheritdoc cref="MaxBiasedRange{T}(T, T, Seed)"/>
    public static T MaxBiasedRange<T>(T min, T max) where T : IFloatingPoint<T> => MaxBiasedRange(min, max, ++_threadLocal);

    /// <summary>
    /// Generates a random <typeparamref name="T"/> that is biased towards the maximum value of the specified range.
    /// </summary>
    /// <inheritdoc cref="UniformRange{T}(Seed, T, T)"/>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static T MaxBiasedRange<T>(T min, T max, Seed seed) where T : IFloatingPoint<T>
    {
        float factor = (float)seed; 
        return Range(max, min, T.CreateTruncating(factor * factor));
    }

    /// <remarks>This method may advance the local <see cref="Seed"/> for the current thread to the next state in the sequence.</remarks>
    /// <inheritdoc cref="MinBiasedRange{T}(T, T, Seed)"/>
    public static T MinBiasedRange<T>(T min, T max) where T : IFloatingPoint<T> => MinBiasedRange(min, max, ++_threadLocal);

    /// <summary>
    /// Generates a random <typeparamref name="T"/> that is biased towards the minimum value of the specified range.
    /// </summary>
    /// <inheritdoc cref="UniformRange{T}(Seed, T, T)"/>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static T MinBiasedRange<T>(T min, T max, Seed seed) where T : IFloatingPoint<T>
    {
        float factor = (float)seed;
        return Range(min, max, T.CreateTruncating(factor * factor));
    }

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    private static T Range<T>(T min, T max, T factor) where T : IFloatingPoint<T> => min + (factor * (++max - min));

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    private static float ToSignedRange(this float state) => (state * 2.0f) - 1.0f;
}
