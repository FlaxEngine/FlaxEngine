using FlaxEngine.Utilities;
using System;
using System.Diagnostics.CodeAnalysis;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Threading;

namespace FlaxEngine;

/// <summary>
/// Represents a seed value used for generating a sequence of pseudo-random numbers.
/// </summary>
[StructLayout(LayoutKind.Explicit, Size = 8)]
public struct Seed : IEquatable<Seed>, ISpanFormattable, IUtf8SpanFormattable, ISpanParsable<Seed>, IUtf8SpanParsable<Seed>
{
    private const uint Multiplier = 1_664_525U;
    private const int Increment = 1_013_904_223;

    private static int _sharedState = Environment.TickCount;
    /// <summary>
    /// Gets or sets the shared state used for random number generation across all threads.
    /// </summary>
    public static int SharedState
    {
        get => Volatile.Read(ref _sharedState);
        set => Interlocked.Exchange(ref _sharedState, value);
    }

    [FieldOffset(0)]
    private bool _initialized;

    [FieldOffset(4)]
    private int _current;
    internal int Current
    {
        readonly get => _current;
        set => _current = value;
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="Seed"/> structure.
    /// </summary>
    public Seed() => Init();

    /// <inheritdoc cref="Seed()"/>
    /// <param name="value">
    /// The initial value to assign to the seed. This value determines the starting state of the random number
    /// generator.
    /// </param>
    public Seed(int value)
    {
        _initialized = true;
        _current = value;
    }

    /// <inheritdoc/>
    public readonly bool Equals(Seed other) => _current == other._current;

    /// <inheritdoc/>
    public override readonly bool Equals([NotNullWhen(true)] object obj) => obj is Seed other && Equals(other);

    /// <inheritdoc/>
    public override readonly int GetHashCode() => _current.GetHashCode();

    /// <inheritdoc/>
    public readonly string ToString(string format, IFormatProvider formatProvider)
    {
        return _current.ToString(format, formatProvider);
    }

    /// <inheritdoc/>
    public readonly bool TryFormat(Span<char> destination, out int charsWritten, ReadOnlySpan<char> format, IFormatProvider provider)
    {
        return _current.TryFormat(destination, out charsWritten, format, provider);
    }

    /// <inheritdoc/>
    public readonly bool TryFormat(Span<byte> utf8Destination, out int bytesWritten, ReadOnlySpan<char> format, IFormatProvider provider)
    {
        return _current.TryFormat(utf8Destination, out bytesWritten, format, provider);
    }

    /// <inheritdoc/>
    public static Seed Parse(ReadOnlySpan<char> s, IFormatProvider provider)
    {
        int value = int.Parse(s, provider);
        return new Seed(value);
    }

    /// <inheritdoc/>
    public static bool TryParse(ReadOnlySpan<char> s, IFormatProvider provider, [MaybeNullWhen(false)] out Seed result)
    {
        if (int.TryParse(s, provider, out int value))
        {
            result = new Seed(value);
            return true;
        }

        result = default;
        return false;
    }

    /// <inheritdoc/>
    public static Seed Parse(string s, IFormatProvider provider)
    {
        int value = int.Parse(s, provider);
        return new Seed(value);
    }

    /// <inheritdoc/>
    public static bool TryParse([NotNullWhen(true)] string s, IFormatProvider provider, [MaybeNullWhen(false)] out Seed result)
    {
        if (int.TryParse(s, provider, out int value))
        {
            result = new Seed(value);
            return true;
        }

        result = default;
        return false;
    }

    /// <inheritdoc/>
    public static Seed Parse(ReadOnlySpan<byte> utf8Text, IFormatProvider provider)
    {
        int value = int.Parse(utf8Text, provider);
        return new Seed(value);
    }

    /// <inheritdoc/>
    public static bool TryParse(ReadOnlySpan<byte> utf8Text, IFormatProvider provider, [MaybeNullWhen(false)] out Seed result)
    {
        if (int.TryParse(utf8Text, provider, out int value))
        {
            result = new Seed(value);
            return true;
        }

        result = default;
        return false;
    }

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    private void Init()
    {
        _current = Interlocked.Add(ref _sharedState, Increment);
        _initialized = true;
    }

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    private void MoveNext()
    {
        _current = unchecked((int)((uint)_current * Multiplier + Increment));
    }

    /// <summary>
    /// Sets a <see cref="Seed"/> to a specified value and returns the original value, as an atomic operation.
    /// </summary>
    /// <inheritdoc cref="Interlocked.Exchange(ref long, long)"/>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static Seed Exchange(ref Seed location1, Seed value)
    {
        long result = Interlocked.Exchange(ref Unsafe.As<Seed, long>(ref location1), Unsafe.BitCast<Seed, long>(value));
        return Unsafe.BitCast<long, Seed>(result);
    }

    /// <summary>
    /// Compares two <see cref="Seed"/> values for equality and, if they are equal, replaces the first value, as an atomic operation.
    /// </summary>
    /// <inheritdoc cref="Interlocked.CompareExchange(ref long, long, long)"/>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static Seed CompareExchange(ref Seed location1, Seed value, Seed comparand)
    {
        ref long intLocation = ref Unsafe.As<Seed, long>(ref location1);
        long intValue = Unsafe.BitCast<Seed, long>(value);
        long intComparand = Unsafe.BitCast<Seed, long>(comparand);
        long result = Interlocked.CompareExchange(ref intLocation, intValue, intComparand);
        return Unsafe.BitCast<long, Seed>(result);
    }

    /// <summary>
    /// Compares two <see cref="Seed"/> instances to determine equality.
    /// </summary>
    /// <param name="left">The value to compare with <paramref name="right"/>.</param>
    /// <param name="right">The value to compare with <paramref name="left"/>.</param>
    /// <returns>
    /// <see langword="true"/> if <paramref name="left"/> is equal to <paramref name="right"/>; 
    /// otherwise, <see langword="false"/>.
    /// </returns>
    public static bool operator ==(Seed left, Seed right) => left.Equals(right);

    /// <summary>
    /// Compares two <see cref="Seed"/> instances to determine inequality.
    /// </summary>
    /// <returns>
    /// <see langword="false"/> if <paramref name="left"/> is equal to <paramref name="right"/>; 
    /// otherwise, <see langword="true"/>.
    /// </returns>
    /// <inheritdoc cref="operator ==(Seed, Seed)"/>
    public static bool operator !=(Seed left, Seed right) => !left.Equals(right);

    /// <summary>
    /// Increments the specified Seed instance to advance it to the next state in its sequence.
    /// </summary>
    /// <param name="seed">The Seed instance to increment.</param>
    /// <returns>The incremented Seed instance, reflecting its new state after the operation.</returns>
    public static Seed operator ++(Seed seed)
    {
        if (!seed._initialized)
        {
            seed.Init();
        }

        seed.MoveNext();

        return seed;
    }

    /// <summary>
    /// Explicitly converts a <see cref="Seed"/> instance to an integer, representing the current state of the seed.
    /// </summary>
    /// <param name="value">The <see cref="Seed"/> instance to convert to an integer.</param>
    public static explicit operator int(Seed value) => value._current;

    /// <summary>
    /// Explicitly converts an integer to a <see cref="Seed"/> instance, initializing the seed with the specified integer value.
    /// </summary>
    /// <param name="value">The <see cref="int"/> value to convert to a <see cref="Seed"/> instance.</param>
    public static explicit operator Seed(int value) => new(value);

    /// <summary>
    /// Explicitly converts a <see cref="Seed"/> instance to a single-precision floating-point number in the range [0.0, 1.0].
    /// </summary>
    /// <param name="value">The <see cref="Seed"/> instance to convert to a <see cref="float"/>.</param>
    public static explicit operator float(Seed value)
    {
        uint bits = unchecked((uint)value._current >> 9 | 0x3F800000U);
        return Unsafe.As<uint, float>(ref bits) - 1.0f;
    }
}
