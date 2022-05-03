
#ifndef TIMEINTERFACE_H
#define TIMEINTERFACE_H
#include "mbed.h"
#include <ctime>

#include "NTPClient.h"

// Special Registers and their usage:
// GPREG0: 32 bits
//      low word: time zone offset (-720 to +720)
//      high word: 2's complement of low word for integrity checking
// GPREG1: 32 bits
//      time_t value when the clock was last set


extern "C" {
#include "time.h"       // uses some std::time-functions
}

/// The tm_ex structure is patterned after the traditional tm struct, however
/// it adds an element - the time zone offset in minutes. From this, it is then
/// readily able to create a "local time" instead of simply a UTC time.
///
struct tm_ex
{
    int   tm_sec;       ///<! seconds, 0 to 59.
    int   tm_min;       ///<! minutes, 0 to 59.
    int   tm_hour;      ///<! hours,   0 to 23.
    int   tm_mday;      ///<! monthday 1 to 31.
    int   tm_mon;       ///<! month    0 to 11.
    int   tm_year;      ///<! years since 1900.
    int   tm_wday;      ///<! days since sunday 0 to 6.
    int   tm_yday;      ///<! days since 1 Jan 0 to 365.
    int   tm_isdst;     ///<! is daylight savings time.
    int   tm_tzo_min;   ///<! localtime zone offset in minutes (_ex element)
};

/// TimeInterface class is much like the normal c-style time.h interface, but
/// is extended with time-zone support, and clock-adjustment support (which 
/// permits tuning the clock) for more accuracy. 
///
/// Additionally, strptime was integrated, which can extract the time from
/// a text string. A formatter is used, so it cannot parse an arbitrary string.
///
/// Within this class are the normal time.h methods, simply
/// exposed here for one consistent interface.
///
/// @note This class uses the special battery backed registers
///     GPREG0 and GPREG1 for TimeInterface data.
///
/// @note In mbed library ver 84, the gmtime method is defective,
///     and calls to this function return junk data. The 
///     gmtime method in this library actually uses localtime,
///     but manages the time-zone offset as it does so.
///
/// @code
/// // TimeInterface Architecture and APIs
/// //
/// // +--------+
/// // | clock  |----> clock_t clock()
/// // +--------+
/// // 
/// // +--------+
/// // |        |<------------ setTime(char * server, uint16_t port, uint32_t timeout)
/// // | NTP    |<-----------> Ethernet
/// // |        |----+
/// // +--------+    |
/// //               |
/// // +--------+    |
/// // | RTC    |<---+-------- set_time(time_t t, int16_t tzo)
/// // |        |<------------ adjust_sec(int32_t)
/// // |        |<------------ set_cal(int32_t)
/// // |        |------------> int32_t get_cal()
/// // |        |------------> time_t time(time_t *)
/// // |        |------+
/// // +--------+      |
/// //                 |  
/// // +--------+      |
/// // |        |<---- | <---- set_dst(bool dst)
/// // |        |<---- | <---- set_dst(char * start_dst, char * end_dst)
/// // |        |----- | ----> bool get_dst()
/// // |dst_pair|---+  |  +----------+
/// // +--------+   |  |  |          |
/// //              |  +->|          |
/// // +--------+   +---->|time_local|--------> time_t timelocal(time_t *)     
/// // | tzo    |<--------|          |
/// // |        |         +----------+
/// // |        |<------------ set_tzo_min(int16_t)
/// // |        |------------> int16_t get_tzo_min()
/// // +--------+                                
/// //                                           
/// // +--------+                                   +--------------------------+
/// // | time_t | ---> char * ctime(time_t *) ----> | buffer                   |
/// // | value  |                                   | Www Mmm dd hh:mm:ss yyyy |
/// // +--------+     +- char * asctime(tm_ex *) -> +--------------------------+
/// //      ^  |      |
/// //      |  |      |                                 +-----------------+   
/// //      |  |      +-------------------------------- | tm_ex           |   
/// //      |  |                                        |   .tm_sec       |   
/// //      |  +- tm_ex * gmtime(const time_t *) -----> |   .tm_min       |   
/// //      |  |                                        |   .tm_hour      |   
/// //      |  +- tm_ex * localtime(const time_t *) --> |   .tm_mday      |   
/// //      |                                           |   .tm_mon       |   
/// //      +---- time_t mktime(struct tm_ex *) ------- |   .tm_year      |   
/// //                                                  |   .tm_wday      |   
/// //                                                  |   .tm_yday      |   
/// //  +---------------------------------------------> |   .tm_isdst     |   
/// //  | +-------------------------------------------- |   .tm_tzo_min   |               
/// //  | |                                             +-----------------+               
/// //  | |                                         +--------------------------+
/// //  | +- strftime(char * ptr, ..., tm_ex *) --> | buffer                   |
/// //  +----strptime(char * buf, ..., tm_ex *) --- | Www Mmm dd hh:mm:ss yyyy |
/// //                                              +--------------------------+
/// //      double difftime(time_t end, time_t)
/// //
/// @endcode
///
class TimeInterface
    {
public:
    /// Constructor for the TimeInterface class, which does minimal initialization.
    ///
    /// @param[in] net is optional and provides the EthernetInterface which is
    ///             used if you want to sync to an NTP server
    ///
    /// @code
    /// EthernetInterface net;
    /// TimeInterface ntp(&net);
    /// ...
    ///     ntp.set_tzo_min(-6 * 60);
    ///     if (NTP_OK == ntp.setTime("time.nist.gov", 123, 10)) {
    ///         time_t tNow = ntp.timelocal();
    ///         printf("time is %s\r\n", ntp.ctime(&tNow));
    ///     }
    /// ...
    /// @endcode
    ///
    TimeInterface(EthernetInterface *m_net = NULL);
    
    /// Destructor, normally not used, because it is typically kept for the life
    /// of the program.
    ///
    ~TimeInterface();
    
    /// Gets the system elapsed time in CLOCKS_PER_SEC tics.
    ///
    /// Divide the returned value by CLOCKS_PER_SEC to get time in seconds.
    ///
    /// @code
    /// clock_t tstart, tend;
    /// ...
    ///     tstart = clock();
    ///     // do something long
    ///     tend = clock();
    ///     printf("Elapsed time is %5.3f\r\n", (float)(tend - tstart)/CLOCKS_PER_SEC);
    /// ...
    /// @endcode
    ///
    /// @returns elapsed tics.
    ///
    clock_t clock(void);
    
    /// Gets the current time as a UTC time value, optionally writing it
    /// to a provided buffer.
    ///
    /// This reads the real time clock and returns the current UTC time.
    ///
    /// @code
    /// time_t t_ref1, t_ref2, t_ref3;
    /// t_ref1 = time(NULL); 
    /// t_ref2 = t.time(); 
    /// (void)t.time(&t_ref3);
    /// @endcode
    ///
    /// @param[out] timer is an optional pointer to a time_t value that will 
    ///             be written with the current time. This pointer is ignored 
    ///             when NULL.
    /// @returns the UTC time value.
    ///
    time_t time(time_t * timer = NULL);

    /// Gets the current time as a LOCAL time value, optionally writing it
    /// to a provided buffer.
    ///
    /// This reads the real time clock and returns the current time, adjusted
    /// for the local time zone and daylight savings time.
    ///
    /// @code
    /// time_t t_ref2, t_ref3;
    /// t_ref2 = t.time(); 
    /// t.timelocal(&t_ref3);
    /// @endcode
    ///
    /// @param[out] timer is an optional pointer to a time_t value that will 
    ///     be written. This pointer is ignored when NULL.
    /// @returns the LOCAL time value (UTC adjusted for the LOCAL time zone and dst).
    ///
    time_t timelocal(time_t * timer = NULL);

    /// Convert a time value structure into an ASCII printable time "Www Mmm dd hh:mm:ss yyyy"
    ///
    /// @note Watch out for race conditions as this returns a pointer to a
    ///         shared buffer.
    /// @note Unlike the standard ctime function, this version DOES NOT append 
    ///         a newline character to the buffer.
    ///
    /// @code
    /// time_t tNow = timelocal();
    /// printf("time is %s\r\n", ctime(tNow));
    /// @endcode
    ///
    /// @param[in] timer is a pointer to a time_t value containing the time to convert.
    /// @returns a pointer to a buffer containing the string.
    ///
    char * ctime(const time_t * timer);

    /// Convert a tm_ex structure into an ASCII printable "time Www Mmm dd hh:mm:ss yyyy"
    ///
    /// @note Unlike the standard asctime, this takes a pointer to a tm_ex, which 
    ///         has a time zone offset element.
    ///
    /// @note Watch out for race conditions as this returns a pointer to a
    ///     shared buffer.
    ///
    /// @note Unlike the standard ctime function, this version DOES NOT append 
    ///     a newline character to the buffer.
    ///
    /// @code
    /// time_t tNow = timelocal();
    /// tm_ex * tEx = localtime(&tNow);
    /// printf("Time is %s\r\n", asctime(tEx));
    /// @endcode
    ///
    /// @param[in] timeptr is a pointer to a tm_ex structure containing the time to convert.
    /// @returns a pointer to a private buffer containing the string.
    ///
    char * asctime(const struct tm_ex *timeptr);

    /// Compute the difference in seconds between two time values.
    ///
    /// @code
    /// time_t tstart, tend;
    /// ...
    ///     tstart = time();
    ///     // do some long process now
    ///     tend = time();
    ///     printf("Elapsed time is %5.3f\r\n", tend - tstart);
    /// ...
    /// @endcode
    ///
    /// @param[in] end is the end time to compare to the beginning time.
    /// @param[in] beginning time is compared to the end time.
    /// @return the difference in seconds, as a double.
    ///
    double difftime(time_t end, time_t beginning);
    
    /// Convert the referenced time_t value to a tm_ex structure in UTC/GMT format.
    ///
    /// @note Unlike the standard asctime, this return a pointer to a tm_ex, which 
    ///         has a time zone offset element.
    ///
    /// @note Watch out for race conditions as this returns a pointer to a
    ///     shared buffer.
    ///
    /// @param[in] timer is a pointer to a time_t structure to convert.
    /// @returns pointer to a tm_ex structure.
    ///
    struct tm_ex * gmtime(const time_t * timer);
    
    
    /// Convert the referenced time_t value to a tm structure in local format.
    ///
    /// This method leverages the time zone offset applied with @see set_tzo()
    /// and the daylight savings time flag applied with @see set_dst().
    ///
    /// @note Watch out for race conditions as this returns a pointer to a
    ///     shared buffer.
    ///
    /// @code
    /// time_t tNow = timelocal();
    /// tm_ex * tEx = localtime(&tNow);
    /// @endcode
    ///
    /// @param[in] timer is a pointer to a time_t structure to convert.
    /// @returns pointer to a tm structure.
    ///
    struct tm_ex * localtime(const time_t * timer);
    
    /// Convert a tm_ex structure (an extended time structure) to a time_t
    /// value.
    ///
    /// This function also sets the tzo_min element of the tm_ex structure
    /// from the previously set tzo_min.
    /// 
    /// @param[in] timeptr is a pointer to a tm_ex structure.
    /// @returns the computed time_t value.
    ///
    time_t mktime(struct tm_ex * timeptr);
    
    /// Presents a time value in a user specified format, into a user specified buffer.
    ///
    /// @param[out] ptr is a pointer to the user buffer.
    /// @param[in] maxsize is the size of the user buffer.
    /// @param[in] format is a pointer to the special strftime format specification.
    ///             see format options.
    /// @param[in] timeptr is a pointer to the tm_ex structure.
    /// @returns the total number of characters copied into the buffer.
    ///
    /// format options:
    ///     - %%a  Abbreviated weekday name e.g. Thu
    ///     - %%A  Full weekday name e.g. Thursday
    ///     - %%b  Abbreviated month name e.g. Aug
    ///     - %%B  Full month name e.g. August
    ///     - %%c  Date and time representation e.g. Thu Aug 23 14:55:02 2001
    ///     - %%C  Year divided by 100 and truncated to integer (00-99) e.g. 20
    ///     - %%d  Day of the month, zero-padded (01-31) e.g. 23
    ///     - %%D  Short MM/DD/YY date, equivalent to %%m/%%d/%%y e.g. 08/23/01
    ///     - %%e  Day of the month, space-padded ( 1-31) e.g. 23
    ///     - %%F  Short YYYY-MM-DD date, equivalent to %%Y-%%m-%%d e.g. 2001-08-23
    ///     - %%g  Week-based year, last two digits (00-99) e.g. 01
    ///     - %%G  Week-based year e.g. 2001
    ///     - %%h  Abbreviated month name * (same as %%b) e.g. Aug
    ///     - %%H  Hour in 24h format (00-23) e.g. 14
    ///     - %%I  Hour in 12h format (01-12) e.g. 02
    ///     - %%j  Day of the year (001-366)  e.g. 235
    ///     - %%m  Month as a decimal number (01-12) e.g. 08
    ///     - %%M  Minute (00-59) e.g. 55
    ///     - %%n  New-line character ('\\n')   
    ///     - %%p  AM or PM designation e.g. PM
    ///     - %%r  12-hour clock time e.g. 02:55:02 pm
    ///     - %%R  24-hour HH:MM time, equivalent to %%H:%%M e.g. 14:55
    ///     - %%S  Second (00-61) e.g. 02
    ///     - %%t  Horizontal-tab character ('\t') 
    ///     - %%T  ISO 8601 time format (HH:MM:SS), equivalent to %%H:%%M:%%S e.g. 14:55:02
    ///     - %%u  ISO 8601 weekday as number with Monday as 1 (1-7) e.g. 4
    ///     - %%U  Week number with the first Sunday as the first day of week one (00-53) e.g. 33
    ///     - %%V  ISO 8601 week number (00-53) e.g. 34
    ///     - %%w  Weekday as a decimal number with Sunday as 0 (0-6) e.g. 4
    ///     - %%W  Week number with the first Monday as the first day of week one (00-53) e.g. 34
    ///     - %%x  Date representation e.g. 08/23/01
    ///     - %%X  Time representation e.g. 14:55:02
    ///     - %%y  Year, last two digits (00-99) e.g. 01
    ///     - %%Y  Year e.g. 2001
    ///     - %%z  ISO 8601 offset from UTC in timezone (1 minute=1, 1 hour=100) (e.g. +100)
    ///           If timezone cannot be determined, no characters 
    ///     - %%Z  Timezone name or abbreviation (e.g. CDT)
    ///           If timezone cannot be determined, no characters 
    ///     - %  A % sign
    ///
    size_t strftime(char * ptr, size_t maxsize, const char * format, const struct tm_ex * timeptr);
    

    /// Convert a string, in a defined format, to a time value in a tm_ex structure.
    ///
    /// Most format details leveraged from The Open Group Base Specifications Issue 6
    /// IEEE Std 1003.1, 2004 Edition
    /// Copyright © 2001-2004 The IEEE and The Open Group, All Rights reserved.
    ///
    /// Modifications for mbed, and addition of the timezone format option by D. Smart
    ///
    /// @code
    ///     char timesample[] = "Jan 22 2017 01:32:48 UTC";
    ///     tm_ex tm;
    ///     strptime(timesample, "%b %d %Y %H:%M:%S %Z", &tm);
    /// @endcode
    /// 
    /// @param[in] buf is a pointer to the string to be parsed.
    /// @param[in] format is a pointer to a format string. See the format options.
    /// @param[out] tm is a pointer to a tm_ex struct.
    /// @returns a pointer to the character following the last one parsed, or null on failure
    ///
    /// format options:
    ///     - %%a The day of the week, using the locale's weekday names; either the abbreviated or 
    ///         full name may be specified.
    ///     - %%A Equivalent to %%a.
    ///     - %%b The month, using the locale's month names; either the abbreviated or full name 
    ///         may be specified.
    ///     - %%B Equivalent to %%b.
    ///     - %%c Replaced by the locale's appropriate date and time representation.
    ///     - %%C The century number [00,99]; leading zeros are permitted but not required.
    ///     - %%d The day of the month [01,31]; leading zeros are permitted but not required.
    ///     - %%D The date as %%m / %%d / %%y.
    ///     - %%e Equivalent to %%d.
    ///     - %%h Equivalent to %%b.
    ///     - %%H The hour (24-hour clock) [00,23]; leading zeros are permitted but not required.
    ///     - %%I The hour (12-hour clock) [01,12]; leading zeros are permitted but not required.
    ///     - %%j The day number of the year [001,366]; leading zeros are permitted but not required.
    ///     - %%m The month number [01,12]; leading zeros are permitted but not required.
    ///     - %%M The minute [00,59]; leading zeros are permitted but not required.
    ///     - %%n Any white space.
    ///     - %%p The locale's equivalent of a.m or p.m.
    ///     - %%r 12-hour clock time using the AM/PM notation if t_fmt_ampm is not an empty string 
    ///         in the LC_TIME portion of the current locale; in the POSIX locale, this shall be 
    ///         equivalent to %%I : %%M : %%S %%p.
    ///     - %%R The time as %%H : %%M.
    ///     - %%S The seconds [00,60]; leading zeros are permitted but not required.
    ///     - %%t Any white space.
    ///     - %%T The time as %%H : %%M : %%S.
    ///     - %%U The week number of the year (Sunday as the first day of the week) as a decimal 
    ///         number [00,53]; leading zeros are permitted but not required.
    ///     - %%w The weekday as a decimal number [0,6], with 0 representing Sunday; leading zeros 
    ///         are permitted but not required.
    ///     - %%W The week number of the year (Monday as the first day of the week) as a decimal 
    ///         number [00,53]; leading zeros are permitted but not required.
    ///     - %%x The date, using the locale's date format.
    ///     - %%X The time, using the locale's time format.
    ///     - %%y The year within century. When a century is not otherwise specified, values in 
    ///         the range [69,99] shall refer to years 1969 to 1999 inclusive, and values in the 
    ///         range [00,68] shall refer to years 2000 to 2068 inclusive; leading zeros shall be 
    ///         permitted but shall not be required.
    ///         Note: It is expected that in a future version of IEEE Std 1003.1-2001 
    ///         the default century inferred from a 2-digit year will change. 
    ///         (This would apply to all commands accepting a 2-digit year as input.)
    ///     - %%Y The year, including the century (for example, 1988).
    ///     - %%Z The timezone offset, as a 3-letter sequence. Only a few whole-hour offsets
    ///         have been defined.
    ///     - %% Replaced by %.
    ///
    const char * strptime(const char *buf, char *fmt, struct tm_ex *tm);


    // time zone functions
    
    /// Set the internal RTC (clock) to the time value. 
    ///
    /// The time valueshould be UTC time along with an offset of zero,
    /// which then permits gmtime and localtime to be used appropriately.
    /// Alternately, the time can be in localtime, and the offset is then
    /// used to compute UTC to set the clock.
    ///
    /// @param[in] t should be the UTC time value to set the clock to. If the available 
    ///     time value is local time, the optional time zone offset can
    ///     be provided so the system clock is UTC.
    /// @param[in] tzo is the optional time zone offset in minutes when it is in
    ///     the range of -720 to +720 (-12 hours to + 12 hours). Any
    ///     other value is illegal and no change will be made.
    ///
    void set_time(time_t t, int16_t tzo_min = 0);
    
    /// Set the time zone offset in minutes.
    ///
    /// This API should be used before any other methods that fetch
    /// the RTC info.
    ///
    /// @param[in] tzo is the time zone offset in minutes when it is in
    ///     the range of -720 to +720 (-12 hours to + 12 hours). Any
    ///     other value is illegal and no change will be made.
    ///
    void set_tzo_min(int16_t tzo_min);
    
    /// Get the time zone offset in minutes.
    ///
    /// @returns the time zone offset value in minutes. If the tzo was
    /// never initialized, this returns zero.
    ///
    int16_t get_tzo_min(void);
    
    /// Set the clock for local time to report whether the current
    /// mode is standard or daylight savings time.
    ///
    /// return values for localtime will then be adjusted not only
    /// for the time zone offset, but for dst.
    ///
    /// @param[in] dst is a boolean that should be set when dst is
    ///         the active mode.
    /// @returns true, always.
    ///
    bool set_dst(bool dst);
    
    /// Set the clock for auto-adjust local time based on 
    /// changing to standard or daylight savings time.
    ///
    /// return values for localtime will then be adjusted not only
    /// for the time zone offset, but for dst.
    ///
    /// @param[in] dstStart is a string of the form "mm/dd,hh:mm"
    ///                     representing when DST starts.
    /// @param[in] dstStop  is a string of the form "mm/dd,hh:mm"
    ///                     representing when DST stops.
    /// @returns true if the start and stop pair could be successfully
    ///               parsed.
    ///
    bool set_dst(const char * dstStart, const char * dstStop);
    
    /// Get the current clock mode for daylight savings time.
    ///
    /// @returns true if clock is in dst mode.
    ///
    bool get_dst(void);
    
    /// Get the time value when the clock was last set. This is most
    /// often used in calibration of the clock.
    ///
    /// @returns time last set as a UTC time value.
    ///
    time_t get_timelastset(void);
    
    /// get_cal will return the calibration register value
    ///
    /// This is the raw register value as a signed 32-bit value (even though
    /// it is actually a 17-bit unsigned value with an additional 'direction' flag).
    ///
    /// @returns calibration settings ranging from -131071 to +131071
    ///
    int32_t get_cal();

    /// set_cal will set the calibration register value
    ///
    /// This accepts a signed value to be used to set the calibration
    /// registers. Setting the calibration value to zero disables the
    /// calibration function. 
    ///
    /// It is important to know the register function in order to use 
    /// this command, and this API is normally not used by external
    /// application code. @See AdjustBySeconds for a user-friendly
    /// API.
    ///
    /// @param[in] calibration value to use ranging from -131071 to +131071
    /// @returns nothing
    ///
    void set_cal(int32_t calibration);

    /// adjust_sec adjusts both the time and the calibration by seconds
    ///
    /// This will take a signed value, which is the current adjustment in seconds
    /// to put the clock on the correct time. So, if the clock is behind by
    /// 3 seconds, the value should be +3 to advance the clock accordingly.
    /// It will then adjust the time, and it will attempt to adjust the
    /// calibration factor to make the time more accurate.
    ///
    /// The adjustment can only be made if it has retained when the clock was
    /// last set, in order to know by how much to adjust it. It is also most
    /// accurate if several days have elapsed since the time was set.
    ///
    /// @note The current version only works if the calibration value
    ///       is zero when this adjustment is made.
    /// 
    /// @param[in] adjustSeconds is the signed value by which to adjust the time to
    ///        correct it to the current actual time.
    /// @returns true if the adjustment was made
    /// @returns false if the adjustment could not be made
    ///
    bool adjust_sec(int32_t adjustSeconds);

    /// Set the clock from an internet source (blocking)
    ///
    /// This function is the interface to NTPClient.
    /// Blocks until completion
    ///
    /// @param[in] host NTP server IPv4 address or hostname (will be resolved via DNS)
    /// @param[in] port port to use; defaults to 123
    /// @param[in] timeout waiting timeout in ms (osWaitForever for blocking function, not recommended)
    /// @returns NTP_OK on success, 
    /// @returns NTP_CONN if no network interface
    /// @returns other NTP error code (<0) on failure
    ///
    NTPResult setTime(const char* host, uint16_t port = NTP_DEFAULT_PORT, uint32_t timeout = NTP_DEFAULT_TIMEOUT);

    // ntp interface functions    
private:
    EthernetInterface * m_net;

    typedef struct {
        uint8_t MM;
        uint8_t DD;
        uint8_t hh;
        uint8_t mm;
    } dst_event_t;
    typedef struct {
        dst_event_t dst_start;
        dst_event_t dst_stop;
    } dst_event_pair_t;

    bool parseDSTstring(dst_event_t * result, const char * dstr);
    
    /// Performs a "simple" computation of two dates into minutes.
    ///
    /// Does not account for leap years or which month it is. Is
    /// useful only for comparing which date/time came first, not for
    /// computing the difference between them.
    ///
    /// @return "normalized" minutes since Jan 1 00:00.
    ///
    uint32_t minutesSinceJan(int mon, int day, int hr, int min);

    dst_event_pair_t dst_pair;
    bool dst;           // true in dst mode
    char result[30];    // holds the converted to text time string
    time_t tresult;     // holds the converted time structure.
    struct tm_ex tm_ext;
    };

#endif // TIMEINTERFACE_H
