#include "mbed.h"
#include "uLCD_4DGL.h"
#include "alarmSet.h"
#include <string>

uLCD_4DGL alarm(p9,p10,p28);
string alarmTime = "12:00:00 AM";

string alarmSet::alarmDisplay()
{
    alarm.locate(0,5);
    alarm.printf("Alarm time:");
    alarm.locate(0,6);
    alarm.printf("%s",alarmTime);
    return alarmTime;
}
void alarmSet::hourSet()
{
    if ((alarmTime[0] == '0' && alarmTime[1] < '9') || (alarmTime[0] == '1' && alarmTime[1] < '1')) {
        alarmTime[1] = alarmTime[1] + 1;
    } else if (alarmTime[0] == '0' && alarmTime[1] == '9') {
        alarmTime[0] = '1';
        alarmTime[1] = '0';
    } else if (alarmTime[0] == '1' && alarmTime[1] == '1') {
            alarmTime[1] = alarmTime[1] + 1;
            if (alarmTime[9] == 'A') {
                alarmTime[9] = 'P';
            } else if (alarmTime[9] == 'P') {
                alarmTime[9] = 'A';
            }
    } else if (alarmTime[0] == '1'&&alarmTime[1] == '2') {
        alarmTime[0] = '0';
        alarmTime[1] = '1';
    }
}

void alarmSet::minuteSet()
{
    if ((alarmTime[3] < '6' && alarmTime[4] < '9')) {
        alarmTime[4] = alarmTime[4] + 1;
    } else if (alarmTime[3] < '5' && alarmTime[4] == '9') {
        alarmTime[3] = alarmTime[3] + 1;
        alarmTime[4] = '0';
    } else if (alarmTime[3] == '5' && alarmTime[4] == '9') {
        alarmTime[3] = '0';
        alarmTime[4] = '0';
        if ((alarmTime[0] < '1' && alarmTime[1] < '9') || (alarmTime[0] == '1' && alarmTime[1] < '1')){
            alarmTime[1] = alarmTime[1] + 1;
        } else if (alarmTime[0] == '1' && alarmTime[1] == '1') {
            alarmTime[1] = alarmTime[1] + 1;
            if (alarmTime[9] == 'A') {
                alarmTime[9] = 'P';
            } else if (alarmTime[9] == 'P') {
                alarmTime[9] = 'A';
            }
        } else if (alarmTime[0] == '1' && alarmTime[1] == '2') {
            alarmTime[0] = '0';
            alarmTime[1] = '1';
        }            
    }
}
