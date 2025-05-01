// Copyright (c) Wojciech Figat. All rights reserved.

#include "DateTime.h"
#include "TimeSpan.h"
#include "String.h"
#include "Engine/Platform/Platform.h"
#include "Engine/Core/Math/Math.h"

const int32 CachedDaysPerMonth[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
const int32 CachedDaysToMonth[] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 };

DateTime::DateTime(int32 year, int32 month, int32 day, int32 hour, int32 minute, int32 second, int32 millisecond)
{
    ASSERT_LOW_LAYER(Validate(year, month, day, hour, minute, second, millisecond));
    int32 daysSum = 0;
    if (month > 2 && IsLeapYear(year))
        daysSum++;
    year--;
    month--;
    daysSum += year * 365 + year / 4 - year / 100 + year / 400 + CachedDaysToMonth[month] + day - 1;
    Ticks = daysSum * TimeSpan::TicksPerDay
            + hour * TimeSpan::TicksPerHour
            + minute * TimeSpan::TicksPerMinute
            + second * TimeSpan::TicksPerSecond
            + millisecond * TimeSpan::TicksPerMillisecond;
}

DateTime DateTime::GetDate() const
{
    return DateTime(Ticks - Ticks % TimeSpan::TicksPerDay);
}

void DateTime::GetDate(int32& year, int32& month, int32& day) const
{
    // Based on:
    // Fliegel, H. F. and van Flandern, T. C.,
    // Communications of the ACM, Vol. 11, No. 10 (October 1968).
    int32 l = Math::FloorToInt((float)(GetDate().GetJulianDay() + 0.5)) + 68569;
    const int32 n = 4 * l / 146097;
    l = l - (146097 * n + 3) / 4;
    int32 i = 4000 * (l + 1) / 1461001;
    l = l - 1461 * i / 4 + 31;
    int32 j = 80 * l / 2447;
    const int32 k = l - 2447 * j / 80;
    l = j / 11;
    j = j + 2 - 12 * l;
    i = 100 * (n - 49) + i + l;
    year = i;
    month = j;
    day = k;
}

int32 DateTime::GetDay() const
{
    int32 year, month, day;
    GetDate(year, month, day);
    return day;
}

DayOfWeek DateTime::GetDayOfWeek() const
{
    return static_cast<DayOfWeek>((Ticks / TimeSpan::TicksPerDay) % 7);
}

int32 DateTime::GetDayOfYear() const
{
    int32 year, month, day;
    GetDate(year, month, day);
    for (int32 i = 1; i < month; i++)
        day += DaysInMonth(year, i);
    return day;
}

int32 DateTime::GetHour() const
{
    return static_cast<int32>(Ticks / TimeSpan::TicksPerHour % 24);
}

int32 DateTime::GetHour12() const
{
    const int32 hour = GetHour();
    if (hour < 1)
        return 12;
    if (hour > 12)
        return hour - 12;
    return hour;
}

double DateTime::GetJulianDay() const
{
    return 1721425.5 + static_cast<double>(Ticks) / TimeSpan::TicksPerDay;
}

double DateTime::GetModifiedJulianDay() const
{
    return GetJulianDay() - 2400000.5;
}

int32 DateTime::GetMillisecond() const
{
    return static_cast<int32>(Ticks / TimeSpan::TicksPerMillisecond % 1000);
}

int32 DateTime::GetMinute() const
{
    return static_cast<int32>(Ticks / TimeSpan::TicksPerMinute % 60);
}

int32 DateTime::GetMonth() const
{
    int32 year, month, day;
    GetDate(year, month, day);
    return month;
}

MonthOfYear DateTime::GetMonthOfYear() const
{
    return static_cast<MonthOfYear>(GetMonth());
}

int32 DateTime::GetSecond() const
{
    return static_cast<int32>(Ticks / TimeSpan::TicksPerSecond % 60);
}

TimeSpan DateTime::GetTimeOfDay() const
{
    return TimeSpan(Ticks % TimeSpan::TicksPerDay);
}

int32 DateTime::GetYear() const
{
    int32 year, month, day;
    GetDate(year, month, day);
    return year;
}

int32 DateTime::DaysInMonth(int32 year, int32 month)
{
    ASSERT_LOW_LAYER((month >= 1) && (month <= 12));
    if (month == 2 && IsLeapYear(year))
        return 29;
    return CachedDaysPerMonth[month];
}

int32 DateTime::DaysInYear(int32 year)
{
    return IsLeapYear(year) ? 366 : 365;
}

bool DateTime::IsLeapYear(int32 year)
{
    if ((year % 4) == 0)
    {
        return (((year % 100) != 0) || ((year % 400) == 0));
    }
    return false;
}

DateTime DateTime::MaxValue()
{
    return DateTime(3652059 * TimeSpan::TicksPerDay - 1);
}

DateTime DateTime::Now()
{
    int32 year, month, day, dayOfWeek, hour, minute, second, millisecond;
    Platform::GetSystemTime(year, month, dayOfWeek, day, hour, minute, second, millisecond);
    return DateTime(year, month, day, hour, minute, second, millisecond);
}

DateTime DateTime::NowUTC()
{
    int32 year, month, day, dayOfWeek, hour, minute, second, millisecond;
    Platform::GetUTCTime(year, month, dayOfWeek, day, hour, minute, second, millisecond);
    return DateTime(year, month, day, hour, minute, second, millisecond);
}

bool DateTime::Validate(int32 year, int32 month, int32 day, int32 hour, int32 minute, int32 second, int32 millisecond)
{
    return year >= 1 && year <= 999999 && month >= 1 && month <= 12 && day >= 1 && day <= DaysInMonth(year, month) && hour >= 0 && hour <= 23 && minute >= 0 && minute <= 59 && second >= 0 && second <= 59 && millisecond >= 0 && millisecond <= 999;
}

String DateTime::ToString() const
{
    return String::Format(TEXT("{}"), *this);
}

String DateTime::ToFileNameString() const
{
    int32 year, month, day;
    GetDate(year, month, day);
    return String::Format(TEXT("{0}_{1:0>2}_{2:0>2}_{3:0>2}_{4:0>2}_{5:0>2}"), year, month, day, GetHour(), GetMinute(), GetSecond());
}

DateTime DateTime::operator+(const TimeSpan& other) const
{
    return DateTime(Ticks + other.Ticks);
}

DateTime& DateTime::operator+=(const TimeSpan& other)
{
    Ticks += other.Ticks;
    return *this;
}

TimeSpan DateTime::operator-(const DateTime& other) const
{
    return TimeSpan(Ticks - other.Ticks);
}

DateTime DateTime::operator-(const TimeSpan& other) const
{
    return DateTime(Ticks - other.Ticks);
}

DateTime& DateTime::operator-=(const TimeSpan& other)
{
    Ticks -= other.Ticks;
    return *this;
}
