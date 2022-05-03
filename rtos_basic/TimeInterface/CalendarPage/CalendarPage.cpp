

#include "CalendarPage.h"


CalendarPage::CalendarPage(uint8_t Month, uint16_t Year) {
    month = Month;
    year = Year;
    verbose = false;
    Compute(month, year);
}


// 37 Elements with Array index 0 to 36,
//    populated with 0 or the day number 
//
//  0  1  2  3  4  5  6     |                       1
//  7  8  9 10 11 12 13     |     2  3  4  5  6  7  8
// 14 15 16 17 18 19 20     |     9 10 11 12 13 14 15
// 21 22 23 24 25 26 27     |    16 17 18 19 20 21 22
// 28 29 30 31 32 33 34     |    23 24 25 26 27 28 29
// 35 36                    |    30 31
//
void CalendarPage::Compute(uint8_t Month, uint16_t Year) {
    const int DaysInMonth[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    const char * MonthName[] = { "", "January", "February", "March", "April", "May", "June",
        "July", "August", "September", "October", "November", "December" };
    const char * DayName[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
    int NumberOfDaysInMonth;
    int FirstDayOfMonth = 0;
    int DayOfWeekCounter = 0;
    int DateCounter = 1;
    int index = 0;
    int day = 1;

    month = Month;
    year = Year;
    int y = year - (14 - month) / 12;
    int m = month + 12 * ((14 - month) / 12) - 2;

    firstday = (day + y + y / 4 - y / 100 + y / 400 + (31 * m / 12)) % 7;
    if ( (month == 2) && (year % 400 == 0 || (year % 4 == 0 && year % 100 != 0)) ) {
        NumberOfDaysInMonth = 29;
    } else {
        NumberOfDaysInMonth = DaysInMonth[month];
    }

    if (verbose) {
        printf("%20s %d\r\n", MonthName[month], year);
        for (int d = 0; d < 7; d++) {
            printf("%4s", DayName[d]);
        }
        printf("\r\n");
    }

    memset(DayMap, 0, sizeof(DayMap));
    for (FirstDayOfMonth = 0; FirstDayOfMonth < firstday; ++FirstDayOfMonth) {
        if (verbose)
            printf("%4s", "");
        DayMap[index++] = 0;
    }

    int tempfirstday = firstday;
    DateCounter = 1;
    DayOfWeekCounter = tempfirstday;
    //This loop represents the date display and will continue to run until
    //the number of days in that month have been reached
    for (DateCounter = 1; DateCounter <= NumberOfDaysInMonth; ++DateCounter) {
        DayMap[index++] = DateCounter;
        if (verbose)
            printf("%4d", DateCounter);
        ++DayOfWeekCounter;
        if (DayOfWeekCounter > 6 && DateCounter != NumberOfDaysInMonth) {
            if (verbose) 
                printf("\r\n");
            DayOfWeekCounter = 0;
        }
    }
    if (verbose)
        printf("\r\n");
    tempfirstday = DayOfWeekCounter + 1;
}
