#include "mbed.h"
#include "uLCD_4DGL.h"
#include "timeDisplay.h"
#include <string>

DigitalIn hour(p13);
DigitalIn minute(p14);
DigitalIn set(p19);

uLCD_4DGL timeScreen(p9,p10,p28); // serial tx, serial rx, reset pin;

void timeDisplay::setTime() {
    hour.mode(PullDown);
    minute.mode(PullDown);
    set.mode(PullDown);
    timeScreen.locate(0,0);
    int seconds = 0;
    while (set==0){
        if(hour==1) {
            seconds = seconds + 3600;
            set_time(seconds);
        } else if (minute==1) {
            seconds = seconds + 60;
            set_time(seconds);
        }
        time_t timeSec = time(NULL);
        char buffer[32];
        strftime(buffer, 32, "%I:%M:%S %p\r", localtime(&timeSec));
        timeScreen.printf("%s", buffer);
    }
}
string timeDisplay::displayTime() {
    timeScreen.locate(0,0);
    time_t seconds = time(NULL);
    char buffer[32];
    strftime(buffer, 32, "%I:%M:%S %p", localtime(&seconds));
    timeScreen.printf("%s", buffer);
    return buffer;
}

