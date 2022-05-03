# ECE4180-FinalProject - Smart Alarm Clock System
Repo for our final ECE4180 Project

Team Members: Jae Il Kim, Hyeran Park, John Choi, Andrew Gonzalez <br>
**Georgia Institute of Technology**

Watch our presentation and demo: <br>
Presentation: https://docs.google.com/presentation/d/16POngjbWnsNYBKCutOtEM3LxX-DAoaqE/edit?usp=sharing&ouid=115215773048721776062&rtpof=true&sd=true<br>
Demo: https://youtu.be/HpHAYLH7wzI<br>
<br>
These are pictures of our smart alarm clock system device from different angles:<br>
<img src="https://user-images.githubusercontent.com/40806367/166446700-cc966093-318b-4819-b305-fd386f069a74.jpg" width=50% height=50%>
<img src="https://user-images.githubusercontent.com/40806367/166446744-6d52f61f-6276-4f99-be73-137fcd27be62.jpg" width=50% height=50%>
<img src="https://user-images.githubusercontent.com/40806367/166446754-d55fe9c3-40d9-47a3-bb3d-2bf67a45da5b.jpg" width=50% height=50%>
<img src="https://user-images.githubusercontent.com/40806367/166446778-d3781a8c-080f-47ac-80ea-0de2939c82b4.jpg" width=50% height=50%>
<img src="https://user-images.githubusercontent.com/40806367/166446796-a6c12cf1-78c3-4428-bf88-6ff4f0e05dd8.jpg" width=50% height=50%>


# Table of Contents
1. [Project Idea](#projectidea)
2. [Parts List](#partslist)
3. [Schematic and Diagrams](#schematic)
4. [Source Code](#source)
5. [Future improvements](#future)


## Project Idea <a name="projectidea">
  One of the toughest parts of being a college student is the long hours of working at night and then having to wake up early for a lecture or lab. As more and more people use their cellphones for their original purpose and alarm, it has been discovered that around 68% of teenagers keep their phone within reach at night and 1/3 of teenagers sleep with their phones. This can definitely be an issue for people who tend to snooze their alarms or outright turn them off instead of actually standing up and going about their day. For this reason, our team decided that designing a remote clock with alarm that would move around the room until turned off would be benefitial for these types of people. With this device, the need to use cellphones as alarms would be greatly reduced resulting in less people using their cellphones in bed and in consequence sleeping with them.<br><br>
  The smart alarm clock system is be a moving alarm clock that can be turned off after the user presses the buttons according to the random LED blinking pattern. The alarm clock is placed on top of the robot chassis along with the sonar sensors that help prevent the device bumping into a wall. It moves for a couple minutes before the alarm turns on so that the user has to move in order to reach the robot and turn the alarm off. The LCD displays the current time and enables users to set the alarm time, volume, and music from the SD card. Once the alarm turns on and the user presses the snooze button, LED blinks in a random pattern and the user must press the buttons accordingly to turn the alarm off.
<br><br>
  The team members had to build a robot chassis that walks away, install the sonar sensors and implement code for robot movement regarding sensor outputs, program to show current time and alarm time on the LCD, show random LED blinks, has a snooze button and other buttons, and an alarm function where the alarm music and volume can be set with the sound coming from the speaker and amp.
  
## Parts List <a name="partslist">
  For this project we used the following devices and parts:
  - 1 MBED LPC1768 (https://www.sparkfun.com/products/9564)
  - LCD Display uLCD-144G2 (https://www.sparkfun.com/products/11377)
  - 2 Motors (https://botland.store/geared-dc-angle-motors/2488-dc-motor-dagu-dg01d-with-48-1-gear-45v-with-double-sided-shaft-2pcs-6952581600251.html)
  - 2 Ultrasonic sensors HC-SR04 (https://os.mbed.com/components/HC-SR04/)
  - 7 Pushbuttons (https://www.sparkfun.com/products/97)
  - 1 Speaker (https://www.sparkfun.com/products/11089)
  - Resistors: 330ohm, https://www.sparkfun.com/products/14490
  - Jumper Wires(M/M and M/F): https://www.sparkfun.com/products/124, https://www.sparkfun.com/products/12794
  
  Other Parts:
  - 1 Robot Chassis
  - 4 AA Batteries
  - 1 USB Rechargeable Brick-type Battery
  - 1 Battery Pack
  
## Schematic and Diagrams <a name="schematic">
  Here is the block diagram of the device:<br>
  <img src="https://user-images.githubusercontent.com/40806367/166444569-9c0491dc-1de3-4b54-adcd-34d4604fadbf.png" width=50% height=50%>

## Source Code <a name="source">
  main.ccp<br>
  ```
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
  ```
  <br>
  uLCD_4DGL_main.cpp
  <br>


  ```
  #include "mbed.h"
  #include "uLCD_4DGL.h"

  #define ARRAY_SIZE(X) sizeof(X)/sizeof(X[0])

  //Serial pc(USBTX,USBRX);


  //******************************************************************************************************
  uLCD_4DGL :: uLCD_4DGL(PinName tx, PinName rx, PinName rst) : _cmd(tx, rx),
      _rst(rst)
  #if DEBUGMODE
      ,pc(USBTX, USBRX)
  #endif // DEBUGMODE
  {
      // Constructor
      _cmd.baud(9600);
  #if DEBUGMODE
      pc.baud(115200);

      pc.printf("\n\n\n");
      pc.printf("*********************\n");
      pc.printf("uLCD_4DGL CONSTRUCTOR\n");
      pc.printf("*********************\n");
  #endif

      _rst = 1;    // put RESET pin to high to start TFT screen
      reset();
      cls();       // clear screen
      current_col         = 0;            // initial cursor col
      current_row         = 0;            // initial cursor row
      current_color       = WHITE;        // initial text color
      current_orientation = IS_PORTRAIT;  // initial screen orientation
      current_hf = 1;
      current_wf = 1;
      set_font(FONT_7X8);                 // initial font
  //   text_mode(OPAQUE);                  // initial texr mode
  }

  //******************************************************************************************************
  void uLCD_4DGL :: writeBYTE(char c)   // send a BYTE command to screen
  {

      _cmd.putc(c);
      wait_us(500);  //mbed is too fast for LCD at high baud rates in some long commands

  #if DEBUGMODE
      pc.printf("   Char sent : 0x%02X\n",c);
  #endif

  }

  //******************************************************************************************************
  void uLCD_4DGL :: writeBYTEfast(char c)   // send a BYTE command to screen
  {

      _cmd.putc(c);
      //wait_ms(0.0);  //mbed is too fast for LCD at high baud rates - but not in short commands

  #if DEBUGMODE
      pc.printf("   Char sent : 0x%02X\n",c);
  #endif

  }
  //******************************************************************************************************
  void uLCD_4DGL :: freeBUFFER(void)         // Clear serial buffer before writing command
  {

      while (_cmd.readable()) _cmd.getc();  // clear buffer garbage
  }

  //******************************************************************************************************
  int uLCD_4DGL :: writeCOMMAND(char *command, int number)   // send several BYTES making a command and return an answer
  {

  #if DEBUGMODE
      pc.printf("\n");
      pc.printf("New COMMAND : 0x%02X\n", command[0]);
  #endif
      int i, resp = 0;
      freeBUFFER();
      writeBYTE(0xFF);
      for (i = 0; i < number; i++) {
          if (i<16)
              writeBYTEfast(command[i]); // send command to serial port
          else
              writeBYTE(command[i]); // send command to serial port but slower
      }
      while (!_cmd.readable()) wait_ms(TEMPO);              // wait for screen answer
      if (_cmd.readable()) resp = _cmd.getc();           // read response if any
      switch (resp) {
          case ACK :                                     // if OK return   1
              resp =  1;
              break;
          case NAK :                                     // if NOK return -1
              resp = -1;
              break;
          default :
              resp =  0;                                 // else return   0
              break;
      }
  #if DEBUGMODE
      pc.printf("   Answer received : %d\n",resp);
  #endif

      return resp;
  }

  //**************************************************************************
  void uLCD_4DGL :: reset()    // Reset Screen
  {
      wait_ms(5);
      _rst = 0;               // put RESET pin to low
      wait_ms(5);         // wait a few milliseconds for command reception
      _rst = 1;               // put RESET back to high
      wait(3);                // wait 3s for screen to restart

      freeBUFFER();           // clean buffer from possible garbage
  }
  //******************************************************************************************************
  int uLCD_4DGL :: writeCOMMANDnull(char *command, int number)   // send several BYTES making a command and return an answer
  {

  #if DEBUGMODE
      pc.printf("\n");
      pc.printf("New COMMAND : 0x%02X\n", command[0]);
  #endif
      int i, resp = 0;
      freeBUFFER();
      writeBYTE(0x00); //command has a null prefix byte
      for (i = 0; i < number; i++) {
          if (i<16) //don't overflow LCD UART buffer
              writeBYTEfast(command[i]); // send command to serial port
          else
              writeBYTE(command[i]); // send command to serial port with delay
      }
      while (!_cmd.readable()) wait_ms(TEMPO);              // wait for screen answer
      if (_cmd.readable()) resp = _cmd.getc();           // read response if any
      switch (resp) {
          case ACK :                                     // if OK return   1
              resp =  1;
              break;
          case NAK :                                     // if NOK return -1
              resp = -1;
              break;
          default :
              resp =  0;                                 // else return   0
              break;
      }
  #if DEBUGMODE
      pc.printf("   Answer received : %d\n",resp);
  #endif

      return resp;
  }

  //**************************************************************************
  void uLCD_4DGL :: cls()    // clear screen
  {
      char command[1] = "";

      command[0] = CLS;
      writeCOMMAND(command, 1);
      current_row=0;
      current_col=0;
      current_hf = 1;
      current_wf = 1;
      set_font(FONT_7X8);                 // initial font
  }

  //**************************************************************************
  int uLCD_4DGL :: version()    // get API version
  {

      char command[2] = "";
      command[0] = '\x00';
      command[1] = VERSION;
      return readVERSION(command, 2);
  }

  //**************************************************************************
  void uLCD_4DGL :: baudrate(int speed)    // set screen baud rate
  {
      char command[3]= "";
      writeBYTE(0x00);
      command[0] = BAUDRATE;
      command[1] = 0;
      int newbaud = BAUD_9600;
      switch (speed) {
          case  110 :
              newbaud = BAUD_110;
              break;
          case  300 :
              newbaud = BAUD_300;
              break;
          case  600 :
              newbaud = BAUD_600;
              break;
          case 1200 :
              newbaud = BAUD_1200;
              break;
          case 2400 :
              newbaud = BAUD_2400;
              break;
          case 4800 :
              newbaud = BAUD_4800;
              break;
          case 9600 :
              newbaud = BAUD_9600;
              break;
          case 14400 :
              newbaud = BAUD_14400;
              break;
          case 19200 :
              newbaud = BAUD_19200;
              break;
          case 31250 :
              newbaud = BAUD_31250;
              break;
          case 38400 :
              newbaud = BAUD_38400;
              break;
          case 56000 :
              newbaud = BAUD_56000;
              break;
          case 57600 :
              newbaud = BAUD_57600;
              break;
          case 115200 :
              newbaud = BAUD_115200;
              break;
          case 128000 :
              newbaud = BAUD_128000;
              break;
          case 256000 :
              newbaud = BAUD_256000;
              break;
          case 300000 :
              newbaud = BAUD_300000;
              speed = 272727;
              break;
          case 375000 :
              newbaud = BAUD_375000;
              speed = 333333;
              break;
          case 500000 :
              newbaud = BAUD_500000;
              speed = 428571;
              break;
          case 600000 :
              newbaud = BAUD_600000;
              break;
          case 750000 : //rates over 600000 are not documented, but seem to work
              newbaud = BAUD_750000;
              break;
          case 1000000 :  
              newbaud = BAUD_1000000;
              break;
          case 1500000 :
              newbaud = BAUD_1500000;
              break;
          case 3000000 :
              newbaud = BAUD_3000000;
              break;
          default   :
              newbaud = BAUD_9600;
              speed = 9600;
              break;
      }

      int i, resp = 0;

      freeBUFFER();
      command[1] = char(newbaud >>8);
      command[2] = char(newbaud % 256);
      wait_ms(1);
      for (i = 0; i <3; i++) writeBYTEfast(command[i]);      // send command to serial port
      for (i = 0; i<10; i++) wait_ms(1); 
      //dont change baud until all characters get sent out
      _cmd.baud(speed);                                  // set mbed to same speed
      i=0;
      while ((!_cmd.readable()) && (i<25000)) {
          wait_ms(TEMPO);           // wait for screen answer - comes 100ms after change
          i++; //timeout if ack character missed by baud change
      }
      if (_cmd.readable()) resp = _cmd.getc();           // read response if any
      switch (resp) {
          case ACK :                                     // if OK return   1
              resp =  1;
              break;
          case NAK :                                     // if NOK return -1
              resp = -1;
              break;
          default :
              resp =  0;                                 // else return   0
              break;
      }
  }

  //******************************************************************************************************
  int uLCD_4DGL :: readVERSION(char *command, int number)   // read screen info and populate data
  {

      int i, temp = 0, resp = 0;
      char response[5] = "";

      freeBUFFER();

      for (i = 0; i < number; i++) writeBYTE(command[i]);    // send all chars to serial port

      while (!_cmd.readable()) wait_ms(TEMPO);               // wait for screen answer

      while (_cmd.readable() && resp < ARRAY_SIZE(response)) {
          temp = _cmd.getc();
          response[resp++] = (char)temp;
      }
      switch (resp) {
          case 2 :                                           // if OK populate data and return 1
              revision  = response[0]<<8 + response[1];
              resp      = 1;
              break;
          default :
              resp =  0;                                     // else return 0
              break;
      }
      return resp;
  }

  //****************************************************************************************************
  void uLCD_4DGL :: background_color(int color)              // set screen background color
  {
      char command[3]= "";                                  // input color is in 24bits like 0xRRGGBB

      command[0] = BCKGDCOLOR;

      int red5   = (color >> (16 + 3)) & 0x1F;              // get red on 5 bits
      int green6 = (color >> (8 + 2))  & 0x3F;              // get green on 6 bits
      int blue5  = (color >> (0 + 3))  & 0x1F;              // get blue on 5 bits

      command[1] = ((red5 << 3)   + (green6 >> 3)) & 0xFF;  // first part of 16 bits color
      command[2] = ((green6 << 5) + (blue5 >>  0)) & 0xFF;  // second part of 16 bits color

      writeCOMMAND(command, 3);
  }

  //****************************************************************************************************
  void uLCD_4DGL :: textbackground_color(int color)              // set screen background color
  {
      char command[3]= "";                                  // input color is in 24bits like 0xRRGGBB

      command[0] = TXTBCKGDCOLOR;

      int red5   = (color >> (16 + 3)) & 0x1F;              // get red on 5 bits
      int green6 = (color >> (8 + 2))  & 0x3F;              // get green on 6 bits
      int blue5  = (color >> (0 + 3))  & 0x1F;              // get blue on 5 bits

      command[1] = ((red5 << 3)   + (green6 >> 3)) & 0xFF;  // first part of 16 bits color
      command[2] = ((green6 << 5) + (blue5 >>  0)) & 0xFF;  // second part of 16 bits color

      writeCOMMAND(command, 3);
  }

  //****************************************************************************************************
  void uLCD_4DGL :: display_control(char mode)     // set screen mode to value
  {
      char command[3]= "";

      command[0] = DISPCONTROL;
      command[1] = 0;
      command[2] = mode;

      if (mode ==  ORIENTATION) {
          switch (mode) {
              case LANDSCAPE :
                  current_orientation = IS_LANDSCAPE;
                  break;
              case LANDSCAPE_R :
                  current_orientation = IS_LANDSCAPE;
                  break;
              case PORTRAIT :
                  current_orientation = IS_PORTRAIT;
                  break;
              case PORTRAIT_R :
                  current_orientation = IS_PORTRAIT;
                  break;
          }
      }
      writeCOMMAND(command, 3);
      set_font(current_font);
  }
  //****************************************************************************************************
  void uLCD_4DGL :: display_power(char mode)     // set screen mode to value
  {
      char command[3]= "";

      command[0] = DISPPOWER;
      command[1] = 0;
      command[2] = mode;
      writeCOMMAND(command, 3);
  }
  //****************************************************************************************************
  void uLCD_4DGL :: set_volume(char value)     // set sound volume to value
  {
      char command[2]= "";

      command[0] = SETVOLUME;
      command[1] = value;

      writeCOMMAND(command, 2);
  }


  //******************************************************************************************************
  int uLCD_4DGL :: getSTATUS(char *command, int number)   // read screen info and populate data
  {

  #if DEBUGMODE
      pc.printf("\n");
      pc.printf("New COMMAND : 0x%02X\n", command[0]);
  #endif

      int i, temp = 0, resp = 0;
      char response[5] = "";

      freeBUFFER();

      for (i = 0; i < number; i++) writeBYTE(command[i]);    // send all chars to serial port

      while (!_cmd.readable()) wait_ms(TEMPO);    // wait for screen answer

      while (_cmd.readable() && resp < ARRAY_SIZE(response)) {
          temp = _cmd.getc();
          response[resp++] = (char)temp;
      }
      switch (resp) {
          case 4 :
              resp = (int)response[1];         // if OK populate data
              break;
          default :
              resp =  -1;                      // else return   0
              break;
      }

  #if DEBUGMODE
      pc.printf("   Answer received : %d\n", resp);
  #endif

      return resp;
  }
  ```

## Future Improvements <a name="future">
  For future improvement of this project we would like to pursue a smaller model which can be placed on a table instead of the floor with sensors that are more accurate and update fasters in order to avoid stepping on the robot by mistake. Another improvement could be to house the wiring for the robot inside a nicer chassis and maybe use a more sophisticated turn-off-alarm system to ensure that the user is fully awake when turning the alarm off. 
  
  Also, one of the things that the team noticed when doing testing is that the alarm sound is too low for effective waking up. In order to fix this, the team has planned to integrate an amplifier for our speaker in order to maximize the sound output of our device. 
