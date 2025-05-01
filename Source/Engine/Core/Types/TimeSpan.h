// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "BaseTypes.h"
#include "Engine/Core/Formatting.h"
#include "Engine/Core/Templates.h"

/// <summary>
/// Represents the difference between two dates and times.
/// </summary>
API_STRUCT(InBuild, Namespace="System") struct FLAXENGINE_API TimeSpan
{
public:
    // The number of timespan ticks per day.
    static constexpr int64 TicksPerDay = 864000000000;

    // The number of timespan ticks per hour.
    static constexpr int64 TicksPerHour = 36000000000;

    // The number of timespan ticks per millisecond.
    static constexpr int64 TicksPerMillisecond = 10000;

    // The number of timespan ticks per minute.
    static constexpr int64 TicksPerMinute = 600000000;

    // The number of timespan ticks per second.
    static constexpr int64 TicksPerSecond = 10000000;

    // The number of timespan ticks per week.
    static constexpr int64 TicksPerWeek = 6048000000000;

public:
    /// <summary>
    /// Time span in 100 nanoseconds resolution.
    /// </summary>
    int64 Ticks;

public:
    /// <summary>
    /// Empty constructor.
    /// </summary>
    TimeSpan()
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="TimeSpan"/> struct.
    /// </summary>
    /// <param name="ticks">The ticks in 100 nanoseconds resolution.</param>
    TimeSpan(int64 ticks)
        : Ticks(ticks)
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="TimeSpan"/> struct.
    /// </summary>
    /// <param name="days">Amount of days.</param>
    /// <param name="hours">Amount of hours.</param>
    /// <param name="minutes">Amount of minutes.</param>
    TimeSpan(int32 days, int32 hours, int32 minutes)
    {
        Set(days, hours, minutes, 0, 0);
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="TimeSpan"/> struct.
    /// </summary>
    /// <param name="days">Amount of days.</param>
    /// <param name="hours">Amount of hours.</param>
    /// <param name="minutes">Amount of minutes.</param>
    /// <param name="seconds">Amount of seconds.</param>
    TimeSpan(int32 days, int32 hours, int32 minutes, int32 seconds)
    {
        Set(days, hours, minutes, seconds, 0);
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="TimeSpan"/> struct.
    /// </summary>
    /// <param name="days">Amount of days.</param>
    /// <param name="hours">Amount of hours.</param>
    /// <param name="minutes">Amount of minutes.</param>
    /// <param name="seconds">Amount of seconds.</param>
    /// <param name="milliseconds">Amount of milliseconds</param>
    TimeSpan(int32 days, int32 hours, int32 minutes, int32 seconds, int32 milliseconds)
    {
        Set(days, hours, minutes, seconds, milliseconds);
    }

public:
    // Get string
    String ToString() const;

    // Get string with custom formatting. Possible values:
    // a: 11:54:22.097
    // default: 11:54:22.0972244
    String ToString(const char option) const;

public:
    FORCE_INLINE TimeSpan operator+(const TimeSpan& other) const
    {
        return TimeSpan(Ticks + other.Ticks);
    }

    FORCE_INLINE TimeSpan& operator+=(const TimeSpan& other)
    {
        Ticks += other.Ticks;
        return *this;
    }

    FORCE_INLINE TimeSpan operator-() const
    {
        return TimeSpan(-Ticks);
    }

    FORCE_INLINE TimeSpan operator-(const TimeSpan& other) const
    {
        return TimeSpan(Ticks - other.Ticks);
    }

    FORCE_INLINE TimeSpan& operator-=(const TimeSpan& other)
    {
        Ticks -= other.Ticks;
        return *this;
    }

    FORCE_INLINE TimeSpan operator*(float scalar) const
    {
        return TimeSpan((int64)((float)Ticks * scalar));
    }

    FORCE_INLINE TimeSpan& operator*=(float scalar)
    {
        Ticks = (int64)((float)Ticks * scalar);
        return *this;
    }

    FORCE_INLINE bool operator==(const TimeSpan& other) const
    {
        return Ticks == other.Ticks;
    }

    FORCE_INLINE bool operator!=(const TimeSpan& other) const
    {
        return Ticks != other.Ticks;
    }

    FORCE_INLINE bool operator>(const TimeSpan& other) const
    {
        return Ticks > other.Ticks;
    }

    FORCE_INLINE bool operator>=(const TimeSpan& other) const
    {
        return Ticks >= other.Ticks;
    }

    FORCE_INLINE bool operator<(const TimeSpan& other) const
    {
        return Ticks < other.Ticks;
    }

    FORCE_INLINE bool operator<=(const TimeSpan& other) const
    {
        return Ticks <= other.Ticks;
    }

public:
    /// <summary>
    /// Gets the days component of this time span.
    /// </summary>
    FORCE_INLINE int32 GetDays() const
    {
        return (int32)(Ticks / TicksPerDay);
    }

    /// <summary>
    /// Returns a time span with the absolute value of this time span.
    /// </summary>
    FORCE_INLINE TimeSpan GetDuration() const
    {
        return TimeSpan(Ticks >= 0 ? Ticks : -Ticks);
    }

    /// <summary>
    /// Gets the hours component of this time span.
    /// </summary>
    FORCE_INLINE int32 GetHours() const
    {
        return (int32)(Ticks / TicksPerHour % 24);
    }

    /// <summary>
    /// Gets the milliseconds component of this time span.
    /// </summary>
    FORCE_INLINE int32 GetMilliseconds() const
    {
        return (int32)(Ticks / TicksPerMillisecond % 1000);
    }

    /// <summary>
    /// Gets the minutes component of this time span.
    /// </summary>
    FORCE_INLINE int32 GetMinutes() const
    {
        return (int32)(Ticks / TicksPerMinute % 60);
    }

    /// <summary>
    /// Gets the seconds component of this time span.
    /// </summary>
    FORCE_INLINE int32 GetSeconds() const
    {
        return (int32)(Ticks / TicksPerSecond % 60);
    }

    /// <summary>
    /// Gets the total number of days represented by this time span.
    /// </summary>
    FORCE_INLINE double GetTotalDays() const
    {
        return (double)Ticks / TicksPerDay;
    }

    /// <summary>
    /// Gets the total number of hours represented by this time span.
    /// </summary>
    FORCE_INLINE double GetTotalHours() const
    {
        return (double)Ticks / TicksPerHour;
    }

    /// <summary>
    /// Gets the total number of milliseconds represented by this time span.
    /// </summary>
    FORCE_INLINE double GetTotalMilliseconds() const
    {
        return (double)Ticks / TicksPerMillisecond;
    }

    /// <summary>
    /// Gets the total number of minutes represented by this time span.
    /// </summary>
    FORCE_INLINE double GetTotalMinutes() const
    {
        return (double)Ticks / TicksPerMinute;
    }

    /// <summary>
    /// Gets the total number of seconds represented by this time span
    /// </summary>
    FORCE_INLINE float GetTotalSeconds() const
    {
        return static_cast<float>(Ticks) / TicksPerSecond;
    }

public:
    /// <summary>
    /// Creates a time span that represents the specified number of days.
    /// </summary>
    /// <param name="days">The number of days.</param>
    /// <returns>The time span.</returns>
    static TimeSpan FromDays(double days);

    /// <summary>
    /// Creates a time span that represents the specified number of hours.
    /// </summary>
    /// <param name="hours">The number of hours.</param>
    /// <returns>The time span.</returns>
    static TimeSpan FromHours(double hours);

    /// <summary>
    /// Creates a time span that represents the specified number of milliseconds.
    /// </summary>
    /// <param name="milliseconds">The number of milliseconds.</param>
    /// <returns>The time span.</returns>
    static TimeSpan FromMilliseconds(double milliseconds);

    /// <summary>
    /// Creates a time span that represents the specified number of minutes.
    /// </summary>
    /// <param name="minutes">The number of minutes.</param>
    /// <returns>The time span.</returns>
    static TimeSpan FromMinutes(double minutes);

    /// <summary>
    /// Creates a time span that represents the specified number of seconds.
    /// </summary>
    /// <param name="seconds">The number of seconds.</param>
    /// <returns>The time span.</returns>
    static TimeSpan FromSeconds(double seconds);

public:
    /// <summary>
    /// Returns the maximum time span value.
    /// </summary>
    static TimeSpan MaxValue();

    /// <summary>
    /// Returns the minimum time span value.
    /// </summary>
    static TimeSpan MinValue();

    /// <summary>
    /// Returns the zero time span value.
    /// </summary>
    static TimeSpan Zero();

private:
    void Set(int32 days, int32 hours, int32 minutes, int32 seconds, int32 milliseconds);
};

inline TimeSpan operator*(float scalar, const TimeSpan& timespan)
{
    return timespan.operator*(scalar);
}

template<>
struct TIsPODType<TimeSpan>
{
    enum { Value = true };
};

DEFINE_DEFAULT_FORMATTING(TimeSpan, "{:0>2}:{:0>2}:{:0>2}.{:0>7}", v.GetHours(), v.GetMinutes(), v.GetSeconds(), v.Ticks % 10000000);
