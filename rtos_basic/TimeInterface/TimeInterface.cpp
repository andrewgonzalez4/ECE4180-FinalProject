
#include "TimeInterface.h"

#include "rtc_api.h"

//#define DEBUG "Time"
#include <cstdio>
#if (defined(DEBUG) && !defined(TARGET_LPC11U24))
#define DBG(x, ...)  std::printf("[DBG %s %3d] "x"\r\n", DEBUG, __LINE__, ##__VA_ARGS__);
#define WARN(x, ...) std::printf("[WRN %s %3d] "x"\r\n", DEBUG, __LINE__, ##__VA_ARGS__);
#define ERR(x, ...)  std::printf("[ERR %s %3d] "x"\r\n", DEBUG, __LINE__, ##__VA_ARGS__);
#define INFO(x, ...) std::printf("[INF %s %3d] "x"\r\n", DEBUG, __LINE__, ##__VA_ARGS__);
#else
#define DBG(x, ...)
#define WARN(x, ...)
#define ERR(x, ...)
#define INFO(x, ...)
#endif

#ifdef WIN32
// Fake it out for Win32 development and testing
struct LPC {
    unsigned long CCR;          // Clock Control register
    unsigned long GPREG0;       // General Purpose Register #0 - 32-bit Battery backed
    unsigned long GPREG1;       // General Purpose Register #1 - 32-bit Battery backed
    unsigned long CALIBRATION;  // Calibration Register
};
struct LPC X;
struct LPC * LPC_RTC = &X;
#define set_time(x) (void)x
#endif


TimeInterface::TimeInterface(EthernetInterface *net)
{
    m_net = net;
    dst = false;
    memset(&dst_pair, 0, sizeof(dst_pair));  // that's enough to keep it from running
}

TimeInterface::~TimeInterface()
{
}

NTPResult TimeInterface::setTime(const char* host, uint16_t port, uint32_t timeout)
{
    NTPResult res;
    
    if (m_net) {
        NTPClient ntp(m_net);
        // int16_t tzomin = get_tzo_min();
        INFO("setTime(%s, %d, %d)\r\n", host, port, timeout);
        res = ntp.setTime(host, port, timeout);
        INFO("  ret: %d\r\n", res);
        if (res == NTP_OK) {
            // if the time was fetched successfully, then
            // let's save the time last set with the local tzo applied
            // and this saves the last time set for later precision
            // tuning.
            set_time(std::time(NULL));
        }
    } else {
        ERR("No connection");
        res = NTP_CONN;
    }
    return res;
}

bool TimeInterface::parseDSTstring(TimeInterface::dst_event_t * result, const char * dstr)
{
    int x;
    dst_event_t test_dst;

    x = atoi(dstr);
    if (x >= 1 && x <= 12) {
        test_dst.MM = x;
        dstr = strchr(dstr, '/');
        if (dstr++) {
            x = atoi(dstr);
            if (x >= 1 && x <= 31) {
                test_dst.DD = x;
                dstr = strchr(dstr, ',');
                if (dstr++) {
                    x = atoi(dstr);
                    if (x >= 0 && x <= 23) {
                        test_dst.hh = x;
                        dstr = strchr(dstr, ':');
                        if (dstr++) {
                            x = atoi(dstr);
                            if (x >= 0 && x <= 59) {
                                test_dst.mm = x;
                                memcpy(result, &test_dst, sizeof(dst_event_t));
                                INFO("parsed: %d/%d %d:%02d", test_dst.MM, test_dst.DD, test_dst.hh, test_dst.mm);
                                return true;
                            }
                        }
                    }
                }
            }
        }
    }
    return false;
}

// parse MM/DD,hh:mm
bool TimeInterface::set_dst(const char * dstStart, const char * dstStop)
{
    dst_event_pair_t test_pair;

    if (parseDSTstring(&test_pair.dst_start, dstStart)
    && parseDSTstring(&test_pair.dst_stop, dstStop)) {
        memcpy(&dst_pair, &test_pair, sizeof(dst_event_pair_t));
        INFO("set_dst from (%s,%s)", dstStart, dstStop);
        return true;
    }
    WARN("failed to set_dst from (%s,%s)", dstStart, dstStop);
    return false;
}

bool TimeInterface::set_dst(bool isdst)
{
    dst = isdst;
    return true;
}

bool TimeInterface::get_dst(void)
{
    return dst;
}

clock_t TimeInterface::clock(void)
{
    return std::clock();
}

time_t TimeInterface::time(time_t * timer)
{
    return std::time(timer);
}

uint32_t TimeInterface::minutesSinceJan(int mon, int day, int hr, int min)
{
    return (mon * 60 * 24 * 31) + (day * 60 * 24) + (hr * 60) + min;
}

time_t TimeInterface::timelocal(time_t * timer)
{
    time_t privTime;
    struct tm * tminfo;

    if (dst_pair.dst_start.MM) {    // may have to change the dst
        std::time(&privTime);
        tminfo = std::localtime(&privTime);

        uint32_t min_since_jan = minutesSinceJan(tminfo->tm_mon + 1, tminfo->tm_mday, tminfo->tm_hour, tminfo->tm_min);
        uint32_t min_dst_start = minutesSinceJan(dst_pair.dst_start.MM, dst_pair.dst_start.DD, dst_pair.dst_start.hh, dst_pair.dst_start.mm) + get_tzo_min();
        uint32_t min_dst_stop  = minutesSinceJan(dst_pair.dst_stop.MM, dst_pair.dst_stop.DD, dst_pair.dst_stop.hh, dst_pair.dst_stop.mm) + get_tzo_min();

        if (min_since_jan >= min_dst_start && min_since_jan < min_dst_stop) {
            dst = 1;
            //INFO(" is dst: %u - %u - %u", min_since_jan, min_dst_start, min_dst_stop);
        } else {
            dst = 0;
            //INFO("not dst: %u - %u - %u", min_since_jan, min_dst_start, min_dst_stop);
        }
    }
    INFO(" timelocal: %u, %d, %d", std::time(timer), get_tzo_min(), dst);
    return std::time(timer) + get_tzo_min() * 60 + dst * 3600;
}

char * TimeInterface::ctime(const time_t * timer)
{
    char * p = std::ctime(timer);

    if (strlen(p) < sizeof(result)) {
        strcpy(result, p);
        p = strchr(result, '\n');
        if (p)
            *p = '\0';
    } else {
        result[0] = '\0';
    }
    return result;
}

char * TimeInterface::asctime(const struct tm_ex * timeptr)
{
    static const char wday_name[][4] = {
        "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
    };
    static const char mon_name[][4] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    struct tm_ex tmp = *timeptr;
    
    tmp.tm_min += tmp.tm_tzo_min;
    while (tmp.tm_min >= 60) {
        tmp.tm_min -= 60;
        tmp.tm_hour++;
    }
    while (tmp.tm_min < 0) {
        tmp.tm_min += 60;
        tmp.tm_hour--;
    }
    while (tmp.tm_hour >= 24) {
        tmp.tm_wday = (tmp.tm_wday + 1) % 7;
        tmp.tm_mday++;
        tmp.tm_hour -= 24;
    }
    while (tmp.tm_hour < 0) {
        tmp.tm_wday = (tmp.tm_wday + 6) % 7;
        tmp.tm_mday--;
        tmp.tm_hour += 24;
    }
    sprintf(result, "%.3s %.3s%3d %.2d:%.2d:%.2d %d",
            wday_name[tmp.tm_wday % 7],
            mon_name[tmp.tm_mon % 12],
            tmp.tm_mday, tmp.tm_hour,
            tmp.tm_min, tmp.tm_sec,
            1900 + tmp.tm_year);
    return result;
}

struct tm_ex * TimeInterface::localtime(const time_t * timer)
{
    time_t priv = *timer + get_tzo_min() * 60 + dst * 3600;
    struct tm * tmp = std::localtime(&priv);

    tm_ext.tm_sec     = tmp->tm_sec;
    tm_ext.tm_min     = tmp->tm_min;
    tm_ext.tm_hour    = tmp->tm_hour;
    tm_ext.tm_mday    = tmp->tm_mday;
    tm_ext.tm_mon     = tmp->tm_mon;
    tm_ext.tm_year    = tmp->tm_year;
    tm_ext.tm_wday    = tmp->tm_wday;
    tm_ext.tm_yday    = tmp->tm_yday;
    tm_ext.tm_isdst   = tmp->tm_isdst;
    tm_ext.tm_tzo_min = get_tzo_min();
    return &tm_ext;
}

struct tm_ex * TimeInterface::gmtime(const time_t * timer)
{
    struct tm * tmp = std::localtime(timer);

    tm_ext.tm_sec = tmp->tm_sec;
    tm_ext.tm_min = tmp->tm_min;
    tm_ext.tm_hour = tmp->tm_hour;
    tm_ext.tm_mday = tmp->tm_mday;
    tm_ext.tm_mon = tmp->tm_mon;
    tm_ext.tm_year = tmp->tm_year;
    tm_ext.tm_wday = tmp->tm_wday;
    tm_ext.tm_yday = tmp->tm_yday;
    tm_ext.tm_isdst = tmp->tm_isdst;
    tm_ext.tm_tzo_min = get_tzo_min();
    return &tm_ext;
}

time_t TimeInterface::mktime(struct tm_ex * timeptr)
{
    timeptr->tm_tzo_min = get_tzo_min();
    return std::mktime((struct tm *)timeptr);
}

size_t TimeInterface::strftime(char * ptr, size_t maxsize, const char * format, const struct tm_ex * timeptr)
{
    return std::strftime(ptr, maxsize, format, (struct tm *)timeptr);
}

double TimeInterface::difftime(time_t end, time_t beginning)
{
    return std::difftime(end, beginning);
}



// time zone functions

void TimeInterface::set_time(time_t t, int16_t tzo_min)
{
    time_t tval = t - (tzo_min * 60);
    rtc_init();
    rtc_write(tval);
    LPC_RTC->GPREG1 = tval;
    INFO("set_time(%s)", ctime(&tval));
}

void TimeInterface::set_tzo_min(int16_t tzo_min)
{
    uint16_t th;
    uint32_t treg;

    if (tzo_min >= -720 && tzo_min <= 720) {
        th = (uint16_t)(-tzo_min);
        treg = (th << 16) | (uint16_t)tzo_min;
        LPC_RTC->GPREG0 = treg;
        //printf("set_tzo(%d) %d is %08X\r\n", tzo, th, LPC_RTC->GPREG0);
    }
}

int16_t TimeInterface::get_tzo_min(void)
{
    uint16_t th, tl;

    th = LPC_RTC->GPREG0 >> 16;
    tl = LPC_RTC->GPREG0;
    //printf("get_tzo() is %04X %04X\r\n", th, tl);
    if ((uint16_t)(th + tl) == 0) {
        return tl;
    } else {
        return 0;
    }
}

time_t TimeInterface::get_timelastset(void)
{
    return LPC_RTC->GPREG1;
}

int32_t TimeInterface::get_cal()
{
    int32_t calvalue = LPC_RTC->CALIBRATION & 0x3FFFF;

    if (calvalue & 0x20000) {
        calvalue = -(calvalue & 0x1FFFF);
    }
    return calvalue;
}

void TimeInterface::set_cal(int32_t calibration)
{
    if (calibration) {
        if (calibration < 0) {
            calibration = (-calibration & 0x1FFFF) | 0x20000;
        }
        LPC_RTC->CCR = 0x000001; //(LPC_RTC->CCR & 0x0003);   // Clear CCALEN to enable it
    } else {
        LPC_RTC->CCR = 0x000011; //(LPC_RTC->CCR & 0x0003) | 0x0010;   // Set CCALEN to disable it
    }
    LPC_RTC->CALIBRATION = calibration;
}

bool TimeInterface::adjust_sec(int32_t adjustSeconds)
{
    time_t lastSet = get_timelastset();

    if (lastSet != 0) {
        time_t seconds = time(NULL);    // get "now" according to the rtc
        int32_t delta = seconds - lastSet;
        //int32_t curCal = get_cal();   // calibration might want to leverage the current cal factor.
        int32_t calMAX = 131071;
        int32_t secPerDay = 86400;
        float errSecPerDay;

        // Convert the current calibration and the adjustment into
        // the new calibration value
        // assume it is +10sec and it has been 2days, then the adjustment
        // needs to be +5 sec per day, or one adjustment every 1/5th
        // of a day, or 1 adjustment every 86400/5 counts.
        // delta = now - then (number of elapsed seconds)
        if (adjustSeconds != 0 && delta != 0) {
            int32_t calFactor;

            // Make the clock correct
            seconds = seconds + adjustSeconds;
            set_time(seconds);
            // Compute the calibration factor
            errSecPerDay = (float)adjustSeconds / ((float)(delta)/secPerDay);
            calFactor = (int32_t)((float)secPerDay/errSecPerDay);
            if (abs(calFactor) < calMAX)
                set_cal(calFactor);
        }
        return true;
    } else {
        return false;
    }
}


// #############################################################################
/*
 * Enhancement to use a custom tm_ex struct and the time zone by D. Smart
 *  %Z
 *
 * Copyright (c) 1994 Powerdog Industries.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in the documentation and/or other materials provided with the
 *    distribution.
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgement:
 *      This product includes software developed by Powerdog Industries.
 * 4. The name of Powerdog Industries may not be used to endorse or
 *    promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY POWERDOG INDUSTRIES ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE POWERDOG INDUSTRIES BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define asizeof(a)      (sizeof (a) / sizeof ((a)[0]))

struct dtconv {
    char    *abbrev_month_names[12];
    char    *month_names[12];
    char    *abbrev_weekday_names[7];
    char    *weekday_names[7];
    char    *time_format;
    char    *sdate_format;
    char    *dtime_format;
    char    *am_string;
    char    *pm_string;
    char    *ldate_format;
    char    *zone_names[10];
    int8_t  zone_offsets[10];
};

static const struct dtconv    En_US = {
    {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    },
    {
        "January", "February", "March", "April",
        "May", "June", "July", "August",
        "September", "October", "November", "December"
    },
    { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" },
    {
        "Sunday", "Monday", "Tuesday", "Wednesday",
        "Thursday", "Friday", "Saturday"
    },
    "%H:%M:%S",
    "%m/%d/%y",
    "%a %b %e %T %Z %Y",
    "AM",
    "PM",
    "%A, %B, %e, %Y",
    { "UTC", "EST", "CST", "MST", "PST", "YST", "CAT", "HST", "CET", "EET", },
    {     0,    -5,    -6,    -7,    -8,    -9,   -10,   -10,    +1,    +2, },
};


#ifndef isprint
#define in_range(c, lo, up)  ((uint8_t)c >= lo && (uint8_t)c <= up)
#define isprint(c)           in_range(c, 0x20, 0x7f)
#define isdigit(c)           in_range(c, '0', '9')
#define isxdigit(c)          (isdigit(c) || in_range(c, 'a', 'f') || in_range(c, 'A', 'F'))
#define islower(c)           in_range(c, 'a', 'z')
#define isspace(c)           (c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t' || c == '\v')
#endif


const char * TimeInterface::strptime(const char *buf, char *fmt, struct tm_ex *tm)
{
    char c, *ptr;
    int i, len;
    bool fSet_wday = false;     // so we can notice if the wday was set
    
    ptr = fmt;
    while (*ptr != 0) {
        if (*buf == 0)
            break;

        c = *ptr++;

        if (c != '%') {
            if (isspace(c))
                while (*buf != 0 && isspace(*buf))
                    buf++;
            else if (c != *buf++)
                return 0;
            continue;
        }

        c = *ptr++;
        switch (c) {
            case 0:
            case '%':
                if (*buf++ != '%')
                    return 0;
                break;

            case 'C':
                buf = strptime(buf, En_US.ldate_format, tm);
                if (buf == 0)
                    return 0;
                break;

            case 'c':
                buf = strptime(buf, "%x %X", tm);
                if (buf == 0)
                    return 0;
                break;

            case 'D':
                buf = strptime(buf, "%m/%d/%y", tm);
                if (buf == 0)
                    return 0;
                break;

            case 'R':
                buf = strptime(buf, "%H:%M", tm);
                if (buf == 0)
                    return 0;
                break;

            case 'r':
                buf = strptime(buf, "%I:%M:%S %p", tm);
                if (buf == 0)
                    return 0;
                break;

            case 'T':
                buf = strptime(buf, "%H:%M:%S", tm);
                if (buf == 0)
                    return 0;
                break;

            case 'X':
                buf = strptime(buf, En_US.time_format, tm);
                if (buf == 0)
                    return 0;
                break;

            case 'x':
                buf = strptime(buf, En_US.sdate_format, tm);
                if (buf == 0)
                    return 0;
                break;

            case 'j':
                if (!isdigit(*buf))
                    return 0;

                for (i = 0; *buf != 0 && isdigit(*buf); buf++) {
                    i *= 10;
                    i += *buf - '0';
                }
                if (i > 365)
                    return 0;

                tm->tm_yday = i;
                break;

            case 'M':
            case 'S':
                if (*buf == 0 || isspace(*buf))
                    break;

                if (!isdigit(*buf))
                    return 0;

                for (i = 0; *buf != 0 && isdigit(*buf); buf++) {
                    i *= 10;
                    i += *buf - '0';
                }
                if (i > 59)
                    return 0;

                if (c == 'M')
                    tm->tm_min = i;
                else
                    tm->tm_sec = i;

                if (*buf != 0 && isspace(*buf))
                    while (*ptr != 0 && !isspace(*ptr))
                        ptr++;
                break;

            case 'H':
            case 'I':
            case 'k':
            case 'l':
                if (!isdigit(*buf))
                    return 0;

                for (i = 0; *buf != 0 && isdigit(*buf); buf++) {
                    i *= 10;
                    i += *buf - '0';
                }
                if (c == 'H' || c == 'k') {
                    if (i > 23)
                        return 0;
                } else if (i > 11)
                    return 0;

                tm->tm_hour = i;

                if (*buf != 0 && isspace(*buf))
                    while (*ptr != 0 && !isspace(*ptr))
                        ptr++;
                break;

            case 'p':
                len = strlen(En_US.am_string);
                if (strncasecmp(buf, En_US.am_string, len) == 0) {
                    if (tm->tm_hour > 12)
                        return 0;
                    if (tm->tm_hour == 12)
                        tm->tm_hour = 0;
                    buf += len;
                    break;
                }

                len = strlen(En_US.pm_string);
                if (strncasecmp(buf, En_US.pm_string, len) == 0) {
                    if (tm->tm_hour > 12)
                        return 0;
                    if (tm->tm_hour != 12)
                        tm->tm_hour += 12;
                    buf += len;
                    break;
                }

                return 0;

            case 'A':
            case 'a':
                for (i = 0; i < asizeof(En_US.weekday_names); i++) {
                    len = strlen(En_US.weekday_names[i]);
                    if (strncasecmp(buf,
                                    En_US.weekday_names[i],
                                    len) == 0)
                        break;

                    len = strlen(En_US.abbrev_weekday_names[i]);
                    if (strncasecmp(buf,
                                    En_US.abbrev_weekday_names[i],
                                    len) == 0)
                        break;
                }
                if (i == asizeof(En_US.weekday_names))
                    return 0;
                fSet_wday = true;
                tm->tm_wday = i;
                buf += len;
                break;

            case 'd':
            case 'e':
                if (!isdigit(*buf))
                    return 0;

                for (i = 0; *buf != 0 && isdigit(*buf); buf++) {
                    i *= 10;
                    i += *buf - '0';
                }
                if (i > 31)
                    return 0;

                tm->tm_mday = i;

                if (*buf != 0 && isspace(*buf))
                    while (*ptr != 0 && !isspace(*ptr))
                        ptr++;
                break;

            case 'B':
            case 'b':
            case 'h':
                for (i = 0; i < asizeof(En_US.month_names); i++) {
                    len = strlen(En_US.month_names[i]);
                    if (strncasecmp(buf,
                                    En_US.month_names[i],
                                    len) == 0)
                        break;

                    len = strlen(En_US.abbrev_month_names[i]);
                    if (strncasecmp(buf,
                                    En_US.abbrev_month_names[i],
                                    len) == 0)
                        break;
                }
                if (i == asizeof(En_US.month_names))
                    return 0;

                tm->tm_mon = i;
                buf += len;
                break;

            case 'm':
                if (!isdigit(*buf))
                    return 0;

                for (i = 0; *buf != 0 && isdigit(*buf); buf++) {
                    i *= 10;
                    i += *buf - '0';
                }
                if (i < 1 || i > 12)
                    return 0;

                tm->tm_mon = i - 1;

                if (*buf != 0 && isspace(*buf))
                    while (*ptr != 0 && !isspace(*ptr))
                        ptr++;
                break;

            case 'Y':
            case 'y':
                if (*buf == 0 || isspace(*buf))
                    break;

                if (!isdigit(*buf))
                    return 0;

                for (i = 0; *buf != 0 && isdigit(*buf); buf++) {
                    i *= 10;
                    i += *buf - '0';
                }
                if (c == 'Y')
                    i -= 1900;
                if (i < 0)
                    return 0;

                tm->tm_year = i;

                if (*buf != 0 && isspace(*buf))
                    while (*ptr != 0 && !isspace(*ptr))
                        ptr++;
                break;
            case 'Z':
                for (i = 0; i < asizeof(En_US.zone_names); i++) {
                    len = strlen(En_US.zone_names[i]);
                    if (strncasecmp(buf,
                                    En_US.zone_names[i],
                                    len) == 0)
                        break;
                }
                if (i == asizeof(En_US.zone_names))
                    return 0;
                tm->tm_tzo_min = En_US.zone_offsets[i] * 60;
                buf += len;
                break;
        }
    }
    if (!fSet_wday) {
        if (mktime(tm) == (time_t)-1)
            tm->tm_wday = 7;
    }
    return buf;
}
