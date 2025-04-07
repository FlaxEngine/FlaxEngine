// Copyright (c) Wojciech Figat. All rights reserved.

#include "TimeSpan.h"
#include "String.h"

TimeSpan TimeSpan::FromDays(double days)
{
    ASSERT_LOW_LAYER((days >= MinValue().GetTotalDays()) && (days <= MaxValue().GetTotalDays()));
    return TimeSpan(static_cast<int64>(days * TicksPerDay));
}

TimeSpan TimeSpan::FromHours(double hours)
{
    ASSERT_LOW_LAYER((hours >= MinValue().GetTotalHours()) && (hours <= MaxValue().GetTotalHours()));
    return TimeSpan(static_cast<int64>(hours * TicksPerHour));
}

TimeSpan TimeSpan::FromMilliseconds(double milliseconds)
{
    ASSERT_LOW_LAYER((milliseconds >= MinValue().GetTotalMilliseconds()) && (milliseconds <= MaxValue().GetTotalMilliseconds()));
    return TimeSpan(static_cast<int64>(milliseconds * TicksPerMillisecond));
}

TimeSpan TimeSpan::FromMinutes(double minutes)
{
    ASSERT_LOW_LAYER((minutes >= MinValue().GetTotalMinutes()) && (minutes <= MaxValue().GetTotalMinutes()));
    return TimeSpan(static_cast<int64>(minutes * TicksPerMinute));
}

TimeSpan TimeSpan::FromSeconds(double seconds)
{
    ASSERT_LOW_LAYER((seconds >= MinValue().GetTotalSeconds()) && (seconds <= MaxValue().GetTotalSeconds()));
    return TimeSpan(static_cast<int64>(seconds * TicksPerSecond));
}

TimeSpan TimeSpan::MaxValue()
{
    return TimeSpan(9223372036854775807);
}

TimeSpan TimeSpan::MinValue()
{
    return TimeSpan(-9223372036854775807 - 1);
}

TimeSpan TimeSpan::Zero()
{
    return TimeSpan(0);
}

void TimeSpan::Set(int32 days, int32 hours, int32 minutes, int32 seconds, int32 milliseconds)
{
    const int64 totalMs = 1000 * (60 * 60 * 24 * (int64)days + 60 * 60 * (int64)hours + 60 * (int64)minutes + (int64)seconds) + (int64)milliseconds;
    ASSERT_LOW_LAYER((totalMs >= MinValue().GetTotalMilliseconds()) && (totalMs <= MaxValue().GetTotalMilliseconds()));
    Ticks = totalMs * TicksPerMillisecond;
}

String TimeSpan::ToString() const
{
    return String::Format(TEXT("{}"), *this);
}

String TimeSpan::ToString(const char option) const
{
    switch (option)
    {
    case 'a':
        return String::Format(TEXT("{:0>2}:{:0>2}:{:0>2}.{:0>3}"), GetHours(), GetMinutes(), GetSeconds(), GetMilliseconds());
    default:
        return String::Format(TEXT("{:0>2}:{:0>2}:{:0>2}.{:0>7}"), GetHours(), GetMinutes(), GetSeconds(), Ticks % 10000000);
    }
}
