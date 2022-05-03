/* NTPClient.cpp */
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
 */
#include "mbed.h" //time() and set_time()

#include "EthernetInterface.h"
#include "UDPSocket.h"
#include "Socket.h"
#include "NTPClient.h"


//#define DEBUG "NTPc"

#if (defined(DEBUG))
#include <cstdio>
#define INFO(x, ...) std::printf("[INF %s %4d] "x"\r\n", DEBUG, __LINE__, ##__VA_ARGS__);
#define WARN(x, ...) std::printf("[WRN %s %4d] "x"\r\n", DEBUG, __LINE__, ##__VA_ARGS__);
#define ERR(x, ...)  std::printf("[ERR %s %4d] "x"\r\n", DEBUG, __LINE__, ##__VA_ARGS__);
static void HexDump(const char * title, void * pT, int count)
{
    int i;
    uint8_t * p = (uint8_t *)pT;
    char buf[100] = "0000: ";

    if (*title)
        INFO("%s", title);
    for (i=0; i<count; ) {
        sprintf(buf + strlen(buf), "%02X ", *(p+i));
        if ((++i & 0x0F) == 0x00) {
            INFO("%s", buf);
            if (i < count)
                sprintf(buf, "%04X: ", i);
            else
                buf[0] = '\0';
        }
    }
    if (strlen(buf))
        INFO("%s", buf);
}
#else
//Disable debug
#define INFO(x, ...)
#define WARN(x, ...)
#define ERR(x, ...)
#define HexDump(a,b,c)
#endif


#define NTP_PORT 123
#define NTP_CLIENT_PORT 0       //Random port
#define NTP_TIMESTAMP_DELTA 2208988800ull //Diff btw a UNIX timestamp (Starting Jan, 1st 1970) and a NTP timestamp (Starting Jan, 1st 1900)

#if 0 && MBED_MAJOR_VERSION == 5
#define htonl(x) ((((x) & 0x000000ffUL) << 24) | \
                     (((x) & 0x0000ff00UL) <<  8) | \
                     (((x) & 0x00ff0000UL) >>  8) | \
                     (((x) & 0xff000000UL) >> 24))
#define ntohl(x) htonl(x)
#endif

NTPClient::NTPClient(EthernetInterface * _net) : m_sock()
{
    net = _net;
}


NTPResult NTPClient::setTime(const char* host, uint16_t port, uint32_t timeout)
{
#ifdef DEBUG
    time_t ctTime;
    ctTime = time(NULL);
    INFO("Time is currently (UTC): %s", ctime(&ctTime));
#endif

#if 0 && MBED_MAJOR_VERSION == 5

    //
    // MBED OS 5
    //

    struct NTPPacket pkt;

    SocketAddress nist;
    int ret_gethostbyname = net->gethostbyname(host, &nist);
    INFO("gethostbyname(%s) returned %d", host, ret_gethostbyname);
    if (ret_gethostbyname < 0) {
        return NTP_DNS;     // Network error on DNS lookup
    }

    nist.set_port(port);
    INFO("set_port(%d)", port);

    time_t tQueryTime = time(NULL);
    //
    //Prepare NTP Packet for the query:
    //
    pkt.li = 0;             //Leap Indicator : No warning
    pkt.vn = 4;             //Version Number : 4
    pkt.mode = 3;           //Client mode
    pkt.stratum = 0;        //Not relevant here
    pkt.poll = 0;           //Not significant as well
    pkt.precision = 0;      //Neither this one is
    pkt.rootDelay = 0;      //Or this one
    pkt.rootDispersion = 0; //Or that one
    pkt.refId = 0;          //...
    pkt.refTm_s = 0;
    pkt.origTm_s = 0;
    pkt.rxTm_s = 0;
    pkt.txTm_s = NTP_TIMESTAMP_DELTA + tQueryTime; //WARN: We are in LE format, network byte order is BE
    pkt.refTm_f = pkt.origTm_f = pkt.rxTm_f = pkt.txTm_f = 0;
    HexDump("NTP Post", (uint8_t *)&pkt, sizeof(NTPPacket));
    pkt.txTm_s = htonl(pkt.txTm_s);

    // Contact the server
    UDPSocket sock;
    nsapi_error_t ret = sock.open(net);
    INFO("sock.open(...) returned %d", ret);
    sock.set_timeout(timeout);   //Set timeout, non-blocking and wait using select

    // Send the query
    int ret_send = sock.sendto(nist, (void *)&pkt, sizeof(NTPPacket));
    INFO("sock.sendto(...) returned %d", ret_send);
    
    SocketAddress source;
    
    // Set the inEndpoint address property
    source.set_ip_address(nist.get_ip_address());
    const int n = sock.recvfrom(&source, (void *)&pkt, sizeof(NTPPacket));
    uint32_t destTimeStamp = NTP_TIMESTAMP_DELTA + time(NULL);
    INFO("recvfrom(...) returned %d", n);

    if (pkt.stratum == 0) { //Kiss of death message : Not good !
        ERR("Kissed to death!");
        sock.close();
        return NTP_PRTCL;
    }

    HexDump("NTP Info", (uint8_t *)&pkt, sizeof(NTPPacket));

    //Correct Endianness
    pkt.refTm_s = ntohl( pkt.refTm_s );
    pkt.refTm_f = ntohl( pkt.refTm_f );
    pkt.origTm_s = ntohl( pkt.origTm_s );
    pkt.origTm_f = ntohl( pkt.origTm_f );
    pkt.rxTm_s = ntohl( pkt.rxTm_s );
    pkt.rxTm_f = ntohl( pkt.rxTm_f );
    pkt.txTm_s = ntohl( pkt.txTm_s );
    pkt.txTm_f = ntohl( pkt.txTm_f );

    #ifdef DEBUG
    const char *ModeList[] = {
        "reserved", "symmetric active", "symmetric passive", "client",
        "server", "broadcast", "reserved for NTP ctrl", "reserved for priv use"
    };
    INFO("  pkt.li (Leap Ind)  %d", pkt.li);
    INFO("  pkt.vn (Vers #)    %d", pkt.vn);
    INFO("  pkt.mode           %d, mode %s", pkt.mode, ModeList[pkt.mode]);
    INFO("  pkt.stratum        %d, 0=kiss-o'-death, 1=prim, 2=secd", pkt.stratum);
    INFO("  pkt.poll           %d", pkt.poll);
    INFO("  pkt.precision      %d", pkt.precision);
    INFO("  pkt.rootDelay      %d", pkt.rootDelay);
    INFO("  pkt.rootDispersion %d", pkt.rootDispersion);
    INFO("  pkt.refId          %08X, %u", pkt.refId, pkt.refId);
    INFO("  pkt.refTm_s        %08X, %u, ref time (last set)", pkt.refTm_s, pkt.refTm_s);
    INFO("  pkt.origTm_s       %08X, %u, time sent from client", pkt.origTm_s, pkt.origTm_s);
    INFO("  pkt.rxTm_s         %08X, %u, time rcvd at server", pkt.rxTm_s, pkt.rxTm_s);
    INFO("  pkt.txTm_s         %08X, %u, time sent from server", pkt.txTm_s, pkt.txTm_s);
    INFO("  pkt.refTm_f        %08X, %u, fraction", pkt.refTm_f, pkt.refTm_f);
    #endif
    
    ret = sock.close();
    INFO("sock.close() returned %d", ret);

    if (n == sizeof(NTPPacket)) {

        // Modification by David Smart
        // The setTime function was computing the offset incorrectly as the value was promoted to 64-bit.
        // The side effect was that a negative offset ended up as a very large positive (e.g. jump from 
        // 2016 to 2084). This change revises that computation.
        int64_t offset = (((int64_t)pkt.rxTm_s - pkt.origTm_s) + ((int64_t)pkt.txTm_s - destTimeStamp))/2;
        set_time( time(NULL) + offset );
    } else {
        ERR("bad return from recvfrom() %d", n);
        if (n < 0) {
            // Network error
            return NTP_CONN;
        } else {
            // No or partial data returned
            return NTP_PRTCL;
        }
    }

#else   // MBED OS 2

    //
    // MBED OS 2
    //

    struct NTPPacket pkt;

    Endpoint nist;
    int ret_gethostbyname = nist.set_address(host, port);
    INFO("gethostbyname(%s) returned %d", host, ret_gethostbyname);
    if (ret_gethostbyname < 0) {
        m_sock.close();
        return NTP_DNS;     // Network error on DNS lookup
    }
    INFO("nist: %s:%d", nist.get_address(), nist.get_port());

    //Create & bind socket
    INFO("Binding socket");
    m_sock.bind(0); //Bind to a random port
    m_sock.set_blocking(false, timeout); //Set not blocking

    time_t tQueryTime = time(NULL);
    //
    //Prepare NTP Packet for the query:
    //
    pkt.li = 0;             //Leap Indicator : No warning
    pkt.vn = 4;             //Version Number : 4
    pkt.mode = 3;           //Client mode
    pkt.stratum = 0;        //Not relevant here
    pkt.poll = 0;           //Not significant as well
    pkt.precision = 0;      //Neither this one is
    pkt.rootDelay = 0;      //Or this one
    pkt.rootDispersion = 0; //Or that one
    pkt.refId = 0;          //...
    pkt.refTm_s = 0;
    pkt.origTm_s = 0;
    pkt.rxTm_s = 0;
    pkt.txTm_s = NTP_TIMESTAMP_DELTA + tQueryTime; //WARN: We are in LE format, network byte order is BE
    pkt.refTm_f = pkt.origTm_f = pkt.rxTm_f = pkt.txTm_f = 0;
    HexDump("NTP Post", (uint8_t *)&pkt, sizeof(NTPPacket));
    pkt.txTm_s = htonl(pkt.txTm_s);

    // Contact the server
    // UDPSocket sock;
    // nsapi_error_t ret = sock.open(net);
    // INFO("sock.open(...) returned %d", ret);
    // sock.set_timeout(timeout);   //Set timeout, non-blocking and wait using select

    // Send the query
    int ret = m_sock.sendTo(nist, (char*)&pkt, sizeof(NTPPacket));
    INFO("m_sock.sendto(...) returned %d", ret);
    if (ret < 0 ) {
        ERR("Could not send packet");
        m_sock.close();
        return NTP_CONN;
    }

    //Read response
    Endpoint inEndpoint;
    // Set the inEndpoint address property
    inEndpoint.set_address(nist.get_address(), 0);
    INFO(" inEndpoint instantiated: %s.", inEndpoint.get_address());

    int loopLimit = 20;  // semi-randomly selected so it doesn't hang forever here...
    do {
        ret = m_sock.receiveFrom( inEndpoint, (char*)&pkt, sizeof(NTPPacket) );
        if(ret < 0) {
            ERR("Could not receive packet, err: %d", ret);
            m_sock.close();
            return NTP_CONN;
        }
        INFO(".");
        loopLimit--;
    } while( strcmp(nist.get_address(), inEndpoint.get_address()) != 0 && loopLimit > 0);

    if(ret < (int)sizeof(NTPPacket)) { //TODO: Accept chunks
        ERR("Receive packet size does not match, rcvd %d, expected %d", ret, (int)sizeof(NTPPacket));
        m_sock.close();
        return NTP_PRTCL;
    }

    if (pkt.stratum == 0) { //Kiss of death message : Not good !
        ERR("Kissed to death!");
        m_sock.close();
        return NTP_PRTCL;
    }

    HexDump("NTP Info", (uint8_t *)&pkt, sizeof(NTPPacket));

    //Correct Endianness
    pkt.refTm_s = ntohl( pkt.refTm_s );
    pkt.refTm_f = ntohl( pkt.refTm_f );
    pkt.origTm_s = ntohl( pkt.origTm_s );
    pkt.origTm_f = ntohl( pkt.origTm_f );
    pkt.rxTm_s = ntohl( pkt.rxTm_s );
    pkt.rxTm_f = ntohl( pkt.rxTm_f );
    pkt.txTm_s = ntohl( pkt.txTm_s );
    pkt.txTm_f = ntohl( pkt.txTm_f );

    //Compute offset, see RFC 4330 p.13
    uint32_t destTm_s = (NTP_TIMESTAMP_DELTA + time(NULL));

    // Modification by David Smart
    // The setTime function was computing the offset incorrectly as the value was promoted to 64-bit.
    // The side effect was that a negative offset ended up as a very large positive (e.g. jump from 
    // 2016 to 2084). This change revises that computation.
    int64_t offset = (((int64_t)pkt.rxTm_s - pkt.origTm_s) + ((int64_t)pkt.txTm_s - destTm_s)) / 2; //Avoid overflow
    set_time(time(NULL) + offset);

    #ifdef DEBUG
    const char *ModeList[] = {
        "reserved", "symmetric active", "symmetric passive", "client",
        "server", "broadcast", "reserved for NTP ctrl", "reserved for priv use"
    };
    INFO("  pkt.li (Leap Ind)  %d", pkt.li);
    INFO("  pkt.vn (Vers #)    %d", pkt.vn);
    INFO("  pkt.mode           %d, mode %s", pkt.mode, ModeList[pkt.mode]);
    INFO("  pkt.stratum        %d, 0=kiss-o'-death, 1=prim, 2=secd", pkt.stratum);
    INFO("  pkt.poll           %d", pkt.poll);
    INFO("  pkt.precision      %d", pkt.precision);
    INFO("  pkt.rootDelay      %d", pkt.rootDelay);
    INFO("  pkt.rootDispersion %d", pkt.rootDispersion);
    INFO("  pkt.refId          %08X, %u", pkt.refId, pkt.refId);
    INFO("  pkt.refTm_s        %08X, %u, ref time (last set)", pkt.refTm_s, pkt.refTm_s);
    INFO("  pkt.origTm_s       %08X, %u, time sent from client", pkt.origTm_s, pkt.origTm_s);
    INFO("  pkt.rxTm_s         %08X, %u, time rcvd at server", pkt.rxTm_s, pkt.rxTm_s);
    INFO("  pkt.txTm_s         %08X, %u, time sent from server", pkt.txTm_s, pkt.txTm_s);
    INFO("  pkt.refTm_f        %08X, %u, fraction", pkt.refTm_f, pkt.refTm_f);
    INFO("  offset             %08X, %u", destTm_s, destTm_s);
    #endif

    m_sock.close();

#endif // OS version

    #ifdef DEBUG
    ctTime = time(NULL);
    INFO("      ctime:         %s", ctime(&ctTime));
    #endif
    return NTP_OK;
}

