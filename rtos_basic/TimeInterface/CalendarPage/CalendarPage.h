

#ifndef CALENDARPAGE_H
#define CALENDARPAGE_H

#include <mbed.h>

#define CALENDAR_DATA_MAP_SIZE 37
/// Creates a calendar for a specified Month and Year
///
/// This class is a simple CalendarPage creator. For a specified
/// Month and Year, it will create a DayMap, which is a
/// simple array of entries that can be easily transformed
/// into a traditional CalendarPage.
///
/// This code was pieced together with various online code
/// samples, none of which had any evidence of a coyright 
/// statement.
///
/// @code 
/// CalendarPage c;
/// c.Compute(3, 2018);
/// for (int i=0; i<c.DayMapEntries(); i++) {
///     if (c.DayMap[i] == 0)
///         printf("%4s", "");
///     else
///         printf("%4d", c.DayMap[i]);
/// }
/// printf("\n");
/// @endcode
///
class CalendarPage {
public:
    /// Constructor for the CalendarPage class
    ///
    /// For a single instance, the constructor may be provided
    /// with a Month and Year. For recurring usage, this is
    /// generally not done, and instead the Compute method 
    /// is called with the parameters.
    ///
    /// @code 
    /// CalendarPage c(3, 2018);
    /// for (int i=0; i<c.DayMapEntries(); i++) {
    ///     if (c.DayMap[i] == 0)
    ///         printf("%4s", "");
    ///     else
    ///         printf("%4d", c.DayMap[i]);
    /// }
    /// printf("\n");
    /// @endcode
    ///
    /// @param[in] Month is optional and defaults to 1 (January).
    /// @param[in] Year is optional and defaults to 2018.
    ///
    CalendarPage(uint8_t Month = 1, uint16_t Year = 2018);

    /// Compute the CalendarPage information, when recurring usage
    /// is needed.
    ///
    /// This API can be used when more than a single CalendarPage is
    /// needed. Alternately, if the constructor has no options, 
    /// this API is used to select the Month and Year.
    ///
    /// @note Erroneous parameters may cause unpredictable results.
    ///
    /// @code 
    /// CalendarPage c;
    /// c.Compute(3, 2018);
    /// for (int i=0; i<c.DayMapEntries(); i++) {
    ///     if (c.DayMap[i] == 0)
    ///         printf("%4s", "");
    ///     else
    ///         printf("%4d", c.DayMap[i]);
    /// }
    /// printf("\n");
    /// @endcode
    ///
    /// @param[in] Month defines the month of interest; e.g. 1 = January, ...
    /// @param[in] Year defines the year of interest; e.g. 2018
    /// @returns true if it computed the CalendarPage information.
    ///
    void Compute(uint8_t Month, uint16_t Year);

    /// The CalendarPage information, expressed as an accessible array.
    ///
    /// - There are DayMapEntries() in the Array index 0 to n-1.
    /// - Each entry has either 0, or the day number in it.
    ///
    /// The information format is best represented in the following
    /// visual.
    /// @verbatim
    ///  DayMap[] array indices   |    DayMap populated
    ///   0  1  2  3  4  5  6     |     0  0  0  0  0  0  1
    ///   7  8  9 10 11 12 13     |     2  3  4  5  6  7  8
    ///  14 15 16 17 18 19 20     |     9 10 11 12 13 14 15
    ///  21 22 23 24 25 26 27     |    16 17 18 19 20 21 22
    ///  28 29 30 31 32 33 34     |    23 24 25 26 27 28 29
    ///  35 36                    |    30 31
    /// @endverbatim
    ///
    uint8_t DayMap[CALENDAR_DATA_MAP_SIZE];

    /// Get the number of entries in the DayMap, to limit iterators.
    /// 
    int DayMapEntries() {
        return CALENDAR_DATA_MAP_SIZE;
    }
private:
    int firstday;
    uint8_t month;
    uint16_t year;
    bool verbose;
};




#endif // CALENDARPAGE_H