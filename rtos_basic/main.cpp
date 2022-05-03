#include "mbed.h"
#include "timeDisplay.h"
#include "uLCD_4DGL.h"
#include "alarmSet.h"
#include "ledSequence.h"
#include "speaker.h"
#include "ultrasonic.h"
#include "motordriver.h"
#include <string>
#include <TimeInterface.h>
#include "rtos.h"

Motor A(p22, p6, p5, 1); // pwm, fwd, rev, can brake 
Motor B(p21, p7, p8, 1); // pwm, fwd, rev, can brake
Thread thread;

DigitalIn hourSet(p13);
DigitalIn minSet(p14);
DigitalIn snooze(p19);

DigitalIn ledButton1(p15);
DigitalIn ledButton2(p16);
DigitalIn ledButton3(p17);
DigitalIn ledButton4(p18);

Serial device(USBTX,USBRX);

timeDisplay timeLCD;
alarmSet alarmSet;
ledSequence LedGame;
speaker speakerPlay;
Timer tSpeaker;
Timer tLED;
Timer tMotor;

string currentTime;
string currentAlarmTime;
string ledColorSeq = " ";
char inputSeq;

int inputCount = 0;
int chosenGame = 0;
int charCount = 0;
int matched = 0;
int dist0 = 0;

void dist(int distance)
{
    //put code here to execute when the distance has changed
    if(distance*0.00328084 < 40) {
    //printf("Distance %f ft\r\n", distance*0.00328084);
    }
}

string replaceChar(string str, char ch1, char ch2) {
  for (int i = 0; i < str.length(); ++i) {
    if (str[i] == ch1)
      str[i] = ch2;
  }

  return str;
}


bool checkSuffix(int A, int B)
{
    // Convert numbers into strings
    char s1[10];
    sprintf(s1, "%d", A);
    char s2[10];
    sprintf(s2, "%d", B);

    // Find the lengths of strings
    // s1 and s2
    int n1 = sizeof(s1)/sizeof(s1[0]);
    int n2 = sizeof(s2)/sizeof(s2[0]);
    // Base Case
    if (n1 < n2) {
        return false;
    }
 
    device.printf("s1 is: %s\n\r", s1);
    device.printf("s2 is: %s\n\r", s2);
    // Traverse the strings s1 & s2
    for (int i = 0; i < n2; i++) {
 
        // If at any index characters
        // are unequals then return false
        if (s1[n1 - i - 1]
            != s2[n2 - i - 1]) {
            return false;
        }
    }
    // Return true
    device.printf("returned true");
    return true;
}

ultrasonic mu(p11, p12, .1, 1, &dist);

void robotMove_thread() 
{
    mu.startUpdates();//start measuring the distance
    tMotor.start();
    while(tMotor.read() <= 10){
            
        if(mu.getCurrentDistance() > 3){
            A.speed(-1);
            B.speed(-1);
            mu.checkDistance();
        } else {
            A.speed(0);
            B.speed(0);
            wait(0.3);
            while(mu.getCurrentDistance() <= 3){
                A.speed(1);
                B.speed(-1);   
            }
             //Thread::wait(0.1);
        } 
        Thread::wait(0.1);
         A.speed(0);
  B.speed(0);
  }
  A.speed(0);
  B.speed(0);
}

int main()
{
    hourSet.mode(PullDown);
    minSet.mode(PullDown);
    snooze.mode(PullDown);

    ledButton1.mode(PullDown);
    ledButton2.mode(PullDown);
    ledButton3.mode(PullDown);
    ledButton4.mode(PullDown);

    timeLCD.setTime();
    tSpeaker.start();
    Thread:wait(0.1);
    tLED.start();
    
    mu.startUpdates();//start measuring the distance
    
    while(1) {
        currentTime = timeLCD.displayTime();
        currentAlarmTime = alarmSet.alarmDisplay();
        if(hourSet==1) {
            alarmSet.hourSet();
        } else if (minSet==1) {
            alarmSet.minuteSet();
        }
        
        string cT = replaceChar(currentTime, ':', '0');
        string aT = replaceChar(currentAlarmTime, ':', '0');
        int val = atoi(cT.c_str());
        int val2 = atoi(aT.c_str());
        device.baud(9600);
        //device.printf("val 1 is %d\n\r", val);
        //device.printf("val 1 is %s\n\r", checkSuffix(std::abs(val-val2), 50));
        bool result = std::abs(val-val2) % 50 == 0;
        if(result){
            device.printf("hhh");
        }
       
        if(result){
            device.baud(9600);
            device.printf("val 1 is %d\n\r", val);
            device.printf("val 2 is %d\n\r", val2);
            thread.start(robotMove_thread);
            
        }
        if (currentAlarmTime.compare(currentTime)==0) {
            
            A.speed(0);
            B.speed(0);
            speakerPlay.speakerInit();
            ledColorSeq=LedGame.ledSelect();

            const char * alarmT = currentAlarmTime.c_str();
            const char * currT = currentTime.c_str();
    
            currentTime = timeLCD.displayTime();
            tLED.stop();
            tLED.reset();
            tLED.start();
            wait(10);
            
            //Test implementation
            while (inputCount < 4) {
                if (matched==ledColorSeq.length()) {
                    speakerPlay.turnOffSpeaker();
                    LedGame.turnOffColor();
                    matched = 0;
                    //ledColorSeq=" ";
                    //inputCount = 0;
                }
                if (ledButton1 == 1) {
                    inputSeq = 'a';
                    if (inputSeq == ledColorSeq[inputCount]) {
                        inputCount++;
                        matched++;  
                        //speakerPlay.playSpeaker();  
                    }
                } else if (ledButton2 == 1) {
                    inputSeq = 'b';
                    if (inputSeq == ledColorSeq[inputCount]) {
                        inputCount++;
                        matched++;  
                        //speakerPlay.playSpeaker();    
                    }
                } else if (ledButton3 == 1) {
                    inputSeq = 'c';
                    if (inputSeq == ledColorSeq[inputCount]) {
                        inputCount++;
                        matched++;  
                        //speakerPlay.playSpeaker();    
                    }
                } else if (ledButton4 == 1) {
                    inputSeq = 'd';
                    if (inputSeq == ledColorSeq[inputCount]) {
                        inputCount++;
                        matched++;  
                        //speakerPlay.playSpeaker();   
                    }
                } else {
                    if(tSpeaker.read()>=7.65){
                        tSpeaker.stop();
                        tSpeaker.reset();
                        tSpeaker.start();
                        speakerPlay.speakerInit();
                    }
                    if(tLED.read()>=3){
                        tLED.stop();
                        tLED.reset();
                        tLED.start();
                        LedGame.ledRepeatSequence(ledColorSeq);
                        //ledColorSeq=LedGame.chooseColor();
                    }
                }
            }
        }
    }
    //Test Run done

}
