// Copyright (c) Wojciech Figat. All rights reserved.

#include "Engine/Core/Types/DateTime.h"
#include <ThirdParty/catch2/catch.hpp>

TEST_CASE("DateTime")
{
    SECTION("Test Convertion")
    {
        constexpr int year = 2023;
        constexpr int month = 12;
        constexpr int day = 16;
        constexpr int hour = 23;
        constexpr int minute = 50;
        constexpr int second = 13;
        constexpr int millisecond = 5;
        const DateTime dt1(year, month, day, hour, minute, second, millisecond);
        CHECK(dt1.GetYear() == year);
        CHECK(dt1.GetMonth() == month);
        CHECK(dt1.GetDay() == day);
        CHECK(dt1.GetHour() == hour);
        CHECK(dt1.GetMinute() == minute);
        CHECK(dt1.GetSecond() == second);
        CHECK(dt1.GetMillisecond() == millisecond);
    }
}
