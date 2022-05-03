#include "mbed.h"
#include "timeDisplay.h"
#include "alarmSet.h"
#include "ledSequence.h"
#include <string>

Serial pc(USBTX,USBRX);
DigitalOut myled1(LED1);
DigitalOut myled2(LED2);
DigitalOut myled3(LED3);
DigitalOut myled4(LED4);

int leds;
int ledCount=0;
char ledPattern[5];

string ledSequence::ledSelect() 
{
    while(ledCount < 4){
        leds = rand() % 4;
        if (leds==0){
            
            myled1 = 1;
            ledPattern[ledCount] = 'a';
            wait(0.4);
            myled1 = 0;
            
        }else if (leds==1){
            myled2 = 1;
            ledPattern[ledCount] = 'b';
            wait(0.4);
            myled2 = 0;
        }else if (leds==2) {
            myled3 = 1;
            ledPattern[ledCount] = 'c';
            wait(0.4);
            myled3 = 0;
        }else if (leds==3) {
            myled3 = 1;
            ledPattern[ledCount] = 'd';
            wait(0.4);
            myled3 = 0;
        }
        ledCount++;
    }
    ledCount=0;
    return ledPattern;
}

void ledSequence::ledRepeatSequence(string ledPattern)
{
    while(ledCount<4){
        if (ledPattern[ledCount] == 'a'){
            myled1 = 1;
            wait(0.4);
            myled1 = 0;
        }else if (ledPattern[ledCount] == 'b'){
            myled2 = 1;
            wait(0.4);
            myled2 = 0;
        }else if (ledPattern[ledCount] == 'c') {
            myled3 = 1;
            wait(0.4);
            myled3 = 0;
        }else if (ledPattern[ledCount] == 'd') {
            myled4 = 1;
            wait(0.4);
            myled4 = 0;
        }
        ledCount++;
    }
    ledCount=0;
    
}

void ledSequence::turnOffColor()
{
    if (myled1 == 1 || myled2 == 1 || myled3 == 1 || myled4 == 1){
        myled1 = 0;
        myled2 = 0;
        myled3 = 0;
        myled4 = 0;
    }
}


