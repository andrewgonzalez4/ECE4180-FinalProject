/* NTPClient.h */
/* Copyright (C) 2012 mbed.org, MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 * and associated documentation files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute,
 * sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

/** \file
NTP Client header file
*/

#ifndef NTPCLIENT_H_
#define NTPCLIENT_H_

#include <cstdint>

using std::uint8_t;
using std::uint16_t;
using std::uint32_t;

#include "EthernetInterface.h"
#include "UDPSocket.h"

#define NTP_DEFAULT_PORT 123
#define NTP_DEFAULT_TIMEOUT 4000

///NTP client results
enum NTPResult {
    NTP_OK = 0,     ///<Success
    NTP_DNS,        ///<Could not resolve name
    NTP_PRTCL,      ///<Protocol error
    NTP_TIMEOUT,    ///<Connection timeout
    NTP_CONN,       ///<Connection error
};

/** NTP Client to update the mbed's RTC using a remote time server
*
*/
class NTPClient
{
public:
    /**
    Instantiate the NTP client
    * @param[in] net is a pointer to the EthernetInterface
    */
    NTPClient(EthernetInterface * net = NULL);

    /**Get current time (blocking)
    Update the time using the server host
    Blocks until completion
    @param[in] host NTP server IPv4 address or hostname (will be resolved via DNS)
    @param[in] port port to use; defaults to 123
    @param[in] timeout waiting timeout in ms (osWaitForever for blocking function, not recommended)
    @return 0 on success, NTP error code (<0) on failure
    */
    NTPResult setTime(const char* host, uint16_t port = NTP_DEFAULT_PORT, uint32_t timeout = NTP_DEFAULT_TIMEOUT); //Blocking

private:
    EthernetInterface * net;
    
    /// The NTP Packet, as defined in RFC 4330 for Simple NTP
    ///
    /// @caution We are in Little Endian locally, and Big Endian at the Network
    ///         so some transformations are required.
    ///
    struct NTPPacket { 
        /// Mode is a 3-bit number indicating the protocol mode
        ///
        /// values
        ///     - 0 = reserved, 
        ///     - 1 = symmetric active
        ///     - 2 = symmetric passive
        ///     - 3 = client
        ///     - 4 = server
        ///     - 5 = broadcast
        ///     - 6 = reserved for NTP control message
        ///     - 7 = reserved for private use
        ///
        unsigned mode : 3;
        
        /// Version Number
        ///
        /// The current version number for NTP/SNTP is 4
        ///
        unsigned vn : 3;    

        /// Leap Indicator
        ///
        /// values
        ///     - 0 = no warning, 
        ///     - 1 = last min had 61s, 
        ///     - 2 = last min had 59s, 
        ///     - 3 = alarm - clock not in sync
        ///
        unsigned li : 2;    

        /// This is indicates the stratum
        ///
        /// values
        ///     - 0 = kiss-o'-death
        ///     - 1 = primary reference
        ///     - 2-15 = secondary reference
        ///     - 16-255 = reserved
        ///
        uint8_t stratum;
        
        /// Poll interval
        ///
        /// This is an exponent of two, where the resulting value is the max
        /// interval between successive messages in seconds.
        ///
        uint8_t poll;
        
        /// Precision of the clock
        ///
        /// This is an eight-bit signed integer used as an exponent of
        /// two, where the resulting value is the precision of the system clock
        /// in seconds.  This field is significant only in server messages, where
        /// the values range from -6 for mains-frequency clocks to -20 for
        /// microsecond clocks found in some workstations.
        ///
        int8_t precision;
        //32 bits header

        /// Root Delay
        /// 
        /// This is a 32-bit signed fixed-point number indicating the
        /// total roundtrip delay to the primary reference source, in seconds
        /// with the fraction point between bits 15 and 16.  Note that this
        /// variable can take on both positive and negative values, depending on
        /// the relative time and frequency offsets.  This field is significant
        /// only in server messages, where the values range from negative values
        /// of a few milliseconds to positive values of several hundred
        /// milliseconds.
        ///
        int32_t rootDelay;
        
        /// Root Dispersion: This is a 32-bit unsigned fixed-point number
        /// indicating the maximum error due to the clock frequency tolerance, in
        /// seconds with the fraction point between bits 15 and 16.  This field
        /// is significant only in server messages, where the values range from
        /// zero to several hundred milliseconds.
        ///
        uint32_t rootDispersion;
        
        /// This idenfies a particular clock source
        ///
        uint32_t refId;

        /// Reference Timestamp identified when the clock was last set.
        /// 
        /// It is a 64-bit value, this is in whole seconds
        ///
        uint32_t refTm_s;
        /// Reference Timestamp identified when the clock was last set.
        /// 
        /// It is a 64-bit value, this is in fractions of a second
        ///
        uint32_t refTm_f;

        /// Originate Timestamp identified when the request departed the client.
        /// 
        /// It is a 64-bit value, this is in whole seconds
        ///
        uint32_t origTm_s;
        /// Originate Timestamp identified when the request departed the client.
        /// 
        /// It is a 64-bit value, this is in fractions of a second
        ///
        uint32_t origTm_f;

        /// Receive Timestamp identified when the request arrived at the server.
        /// 
        /// It is a 64-bit value, this is in whole seconds
        ///
        uint32_t rxTm_s;
        /// Receive Timestamp identified when the request arrived at the server.
        /// 
        /// It is a 64-bit value, this is in fractions of a second
        ///
        uint32_t rxTm_f;

        /// Transmit Timestamp identified when the request departed the client/server.
        /// 
        /// It is a 64-bit value, this is in whole seconds
        ///
        uint32_t txTm_s;
        /// Transmit Timestamp identified when the request departed the client/server.
        /// 
        /// It is a 64-bit value, this is in fractions of a second
        ///
        uint32_t txTm_f;
    } __attribute__ ((packed));

    UDPSocket m_sock;
};


#endif /* NTPCLIENT_H_ */
