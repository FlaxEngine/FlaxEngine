// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "BaseTypes.h"
#include "Engine/Core/Formatting.h"
#include "Engine/Core/Templates.h"
#include "Engine/Core/Enums.h"

/// <summary>
/// The days of the week.
/// </summary>
DECLARE_ENUM_EX_7(DayOfWeek, int32, 0, Monday, Tuesday, Wednesday, Thursday, Friday, Saturday, Sunday);

/// <summary>
/// The months of the year.
/// </summary>
DECLARE_ENUM_EX_12(MonthOfYear, int32, 1, January, February, March, April, May, June, July, August, September, October, November, December);

/// <summary>
/// Represents date and time.
/// </summary>
API_STRUCT(InBuild, Namespace="System") struct FLAXENGINE_API DateTime
{
public:
    /// <summary>
    /// Ticks in 100 nanoseconds resolution since January 1, 0001 A.D.
    /// </summary>
    int64 Ticks;

public:
    /// <summary>
    /// Empty constructor.
    /// </summary>
    DateTime()
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="DateTime"/> struct.
    /// </summary>
    /// <param name="ticks">The ticks representing the date and time.</param>
    DateTime(int64 ticks)
        : Ticks(ticks)
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="DateTime"/> struct.
    /// </summary>
    /// <param name="year">The year.</param>
    /// <param name="month">The month.</param>
    /// <param name="day">The day.</param>
    /// <param name="hour">The hour.</param>
    /// <param name="minute">The minute.</param>
    /// <param name="second">The second.</param>
    /// <param name="millisecond">The millisecond.</param>
    DateTime(int32 year, int32 month, int32 day, int32 hour = 0, int32 minute = 0, int32 second = 0, int32 millisecond = 0);

public:
    /// <summary>
    /// Gets the string.
    /// </summary>
    String ToString() const;

    /// <summary>
    /// Gets the string that is valid for filename.
    /// </summary>
    String ToFileNameString() const;

public:
    DateTime operator+(const TimeSpan& other) const;
    DateTime& operator+=(const TimeSpan& other);
    TimeSpan operator-(const DateTime& other) const;
    DateTime operator-(const TimeSpan& other) const;
    DateTime& operator-=(const TimeSpan& other);

    bool operator==(const DateTime& other) const
    {
        return Ticks == other.Ticks;
    }

    bool operator!=(const DateTime& other) const
    {
        return Ticks != other.Ticks;
    }

    bool operator>(const DateTime& other) const
    {
        return Ticks > other.Ticks;
    }

    bool operator>=(const DateTime& other) const
    {
        return Ticks >= other.Ticks;
    }

    bool operator<(const DateTime& other) const
    {
        return Ticks < other.Ticks;
    }

    bool operator<=(const DateTime& other) const
    {
        return Ticks <= other.Ticks;
    }

public:
    /// <summary>
    /// Gets the date part of this date. The time part is truncated and becomes 00:00:00.000.
    /// </summary>
    DateTime GetDate() const;

    /// <summary>
    /// Gets the date components of this date.
    /// </summary>
    /// <param name="year">The year.</param>
    /// <param name="month">The month (1-12).</param>
    /// <param name="day">The day (1-31).</param>
    void GetDate(int32& year, int32& month, int32& day) const;

    /// <summary>
    /// Gets this date's day part (1 to 31).
    /// </summary>
    int32 GetDay() const;

    /// <summary>
    /// Calculates this date's day of the week (Sunday - Saturday).
    /// </summary>
    DayOfWeek GetDayOfWeek() const;

    /// <summary>
    /// Gets this date's day of the year.
    /// </summary>
    int32 GetDayOfYear() const;

    /// <summary>
    /// Gets this date's hour part in 24-hour clock format (0 to 23).
    /// </summary>
    int32 GetHour() const;

    /// <summary>
    /// Gets this date's hour part in 12-hour clock format (1 to 12).
    /// </summary>
    int32 GetHour12() const;

    /// <summary>
    /// Gets the Julian Day for this date.
    /// </summary>
    /// <remarks>The Julian Day is the number of days since the inception of the Julian calendar at noon on Monday, January 1, 4713 B.C.E. The minimum Julian Day that can be represented in DateTime is 1721425.5, which corresponds to Monday, January 1, 0001 in the Gregorian calendar.</remarks>
    double GetJulianDay() const;

    /// <summary>
    /// Gets the Modified Julian day.
    /// </summary>
    /// <remarks>The Modified Julian Day is calculated by subtracting 2400000.5, which corresponds to midnight UTC on November 17, 1858 in the Gregorian calendar.</remarks>
    double GetModifiedJulianDay() const;

    /// <summary>
    /// Gets this date's millisecond part (0 to 999).
    /// </summary>
    int32 GetMillisecond() const;

    /// <summary>
    /// Gets this date's minute part (0 to 59).
    /// </summary>
    int32 GetMinute() const;

    /// <summary>
    /// Gets this date's the month part (1 to 12).
    /// </summary>
    int32 GetMonth() const;

    /// <summary>
    /// Gets the date's month of the year (January to December).
    /// </summary>
    MonthOfYear GetMonthOfYear() const;

    /// <summary>
    /// Gets this date's second part.
    /// </summary>
    int32 GetSecond() const;

    /// <summary>
    /// Gets this date's representation as number of ticks since midnight, January 1, 0001.
    /// </summary>
    int64 GetTicks() const
    {
        return Ticks;
    }

    /// <summary>
    /// Gets the time elapsed since midnight of this date.
    /// </summary>
    TimeSpan GetTimeOfDay() const;

    /// <summary>
    /// Gets this date's year part.
    /// </summary>
    int32 GetYear() const;

public:
    /// <summary>
    /// Gets the number of days in the year and month.
    /// </summary>
    /// <param name="year">The year.</param>
    /// <param name="month">The month.</param>
    /// <returns>The number of days</returns>
    static int32 DaysInMonth(int32 year, int32 month);

    /// <summary>
    /// Gets the number of days in the given year.
    /// </summary>
    /// <param name="year">The year.</param>
    /// <returns>The number of days.</returns>
    static int32 DaysInYear(int32 year);

    /// <summary>
    /// Determines whether the specified year is a leap year.
    /// </summary>
    /// <remarks>A leap year is a year containing one additional day in order to keep the calendar synchronized with the astronomical year.</remarks>
    /// <param name="year">The year.</param>
    /// <returns><c>true</c> if the specified year os a leap year; otherwise, <c>false</c>.</returns>
    static bool IsLeapYear(int32 year);

    /// <summary>
    /// Returns the maximum date value.
    /// </summary>
    /// <remarks>The maximum date value is December 31, 9999, 23:59:59.9999999.</remarks>
    /// <returns>The maximum valid date.</returns>
    static DateTime MaxValue();

    /// <summary>
    /// Returns the minimum date value.
    /// </summary>
    /// <remarks>The minimum date value is January 1, 0001, 00:00:00.0.</remarks>
    /// <returns>The minimum valid date.</returns>
    static DateTime MinValue()
    {
        return DateTime(1, 1, 1, 0, 0, 0, 0);
    }

    /// <summary>
    /// Gets the local date and time on this computer.
    /// </summary>
    /// <remarks>This method takes into account the local computer's time zone and daylight saving settings. For time zone independent time comparisons, and when comparing times between different computers, use NowUTC() instead.</remarks>
    /// <returns>The current date and time.</returns>
    static DateTime Now();

    /// <summary>
    /// Gets the UTC date and time on this computer.
    /// </summary>
    /// <remarks>This method returns the Coordinated Universal Time (UTC), which does not take the local computer's time zone and daylight savings settings into account. It should be used when comparing dates and times that should be independent of the user's locale. To get the date and time in the current locale, use Now() instead.</remarks>
    /// <returns>The current date and time.</returns>
    static DateTime NowUTC();

    /// <summary>
    /// Validates the given components of a date and time value.
    /// </summary>
    /// <param name="year">The year (1 - 9999).</param>
    /// <param name="month">The month (1 - 12).</param>
    /// <param name="day">The day (1 - DaysInMonth(Month)).</param>
    /// <param name="hour">The hour (0 - 23).</param>
    /// <param name="minute">The minute (0 - 59).</param>
    /// <param name="second">The second (0 - 59).</param>
    /// <param name="millisecond">The millisecond (0 - 999).</param>
    /// <returns>True if all the components are valid, false otherwise.</returns>
    static bool Validate(int32 year, int32 month, int32 day, int32 hour, int32 minute, int32 second, int32 millisecond);
};

template<>
struct TIsPODType<DateTime>
{
    enum { Value = true };
};

namespace fmt
{
    template<>
    struct formatter<DateTime, Char>
    {
        template<typename ParseContext>
        auto parse(ParseContext& ctx)
        {
            return ctx.begin();
        }

        template<typename FormatContext>
        auto format(const DateTime& v, FormatContext& ctx)
        {
            int32 year, month, day;
            v.GetDate(year, month, day);
            return fmt::format_to(ctx.out(), basic_string_view<Char>(TEXT("{0}-{1:0>2}-{2:0>2} {3:0>2}:{4:0>2}:{5:0>2}")), year, month, day, v.GetHour(), v.GetMinute(), v.GetSecond());
        }
    };
}
