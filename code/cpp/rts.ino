#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <Wire.h>      //I2C library
#include <RtcDS3231.h> //RTC library
#include <EEPROM.h>
// CONSTANTS
//---------------------------------
#define BUTTON_L 3
#define BUTTON_R 2
#define SDA A4
#define SLC A5
#define FET_OUT 9
#define POT_IN A2
#define BUTTON_LED_OUT 6
#define BUTTON_TIME 3000    // Time after which a click is considered as a long click
#define BLINK_DURATION 1000 // Blink duration of digits when setting time/date
#define SCREEN_WIDTH 128    // OLED display width, in pixels
#define SCREEN_HEIGHT 32    // OLED display height, in pixels
#define OLED_RESET -1       // Reset pin # (or -1 if sharing Arduino reset pin)
// calendar image
const static unsigned char PROGMEM calendar[] = {48, 12, 48, 12, 127, 254, 255, 255, 255, 255, 192, 3, 219, 109, 219, 109, 192, 1, 203, 109, 219, 109, 192, 1, 219, 109, 219, 109, 96, 2, 63, 252};
// bell image
const static unsigned char PROGMEM bell[] = {1, 128, 1, 128, 3, 192, 6, 96, 12, 16, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 48, 12, 112, 14, 96, 6, 96, 14, 60, 60, 3, 192};
// clockhour image
const static unsigned char PROGMEM clockhour[] = {3, 192, 12, 48, 16, 8, 33, 132, 65, 130, 65, 130, 129, 129, 129, 129, 129, 1, 130, 1, 68, 3, 72, 2, 32, 4, 16, 8, 12, 48, 3, 192};
// smiley image
const static unsigned char PROGMEM smiley[] = {0, 15, 240, 0, 0, 127, 254, 0, 1, 255, 255, 128, 3, 240, 31, 192, 7, 0, 1, 224, 14, 0, 0, 240, 28, 240, 15, 120, 57, 8, 16, 188, 48, 96, 6, 28, 112, 240, 15, 14, 112, 240, 15, 14, 96, 240, 15, 6, 224, 240, 15, 7, 224, 96, 6, 7, 224, 0, 0, 7, 224, 0, 0, 7, 226, 0, 0, 71, 227, 128, 3, 199, 227, 248, 31, 199, 225, 255, 255, 135, 97, 255, 255, 134, 112, 255, 255, 6, 112, 127, 254, 14, 56, 63, 252, 12, 56, 31, 248, 28, 28, 15, 240, 56, 14, 0, 0, 112, 7, 0, 0, 224, 3, 192, 3, 192, 1, 240, 15, 128, 0, 127, 254, 0, 0, 31, 248, 0};
// hourglass image
const static unsigned char hourglass[] PROGMEM = {63, 254, 31, 252, 16, 4, 11, 232, 11, 232, 11, 200, 4, 144, 2, 32, 2, 32, 4, 16, 8, 8, 8, 8, 8, 8, 16, 4, 31, 252, 63, 254};
// days per month
const unsigned char daysPerMonth[12] = {31, 30, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
// Functions
boolean adjustDST(RtcDateTime &time_);
unsigned char readButtons();
unsigned char readButtons_setTime();
void setDuration(unsigned char &curDuration);
void setTime(unsigned char &curHour, unsigned char &curMin, boolean setAlarm = false);
void setDate(unsigned short &curYear, unsigned char &curMonth, unsigned char &curDay);
void u8gInfo(char info[]);
void displayInfo(char info[], const unsigned char *picture);
void displayInfo(char info[], short blink_index[]);
void displayInfo(char info[], const unsigned char *picture, short blink_index[]);
void displayInfo(char info[], char info2[]);
void displayInfo(char info[], char info2[], const unsigned char *picture);
// VARIABLES
//---------------------------------
// Initialize flags
boolean buttonActive = false;    // if a button is clicked
boolean longPressActive = false; // if long click
boolean button1Active = false;   // if button 1 clicked
boolean button2Active = false;   // if button 2 clicked
boolean mainLightOn = false;     // if main led on
boolean updateTime = false;      // if new time has been set
boolean updateAlarm = false;     // if new alarm has been set
boolean alarmEnabled = false;
boolean alarmRunning = false;
boolean isDST = false;    // if daylight saving time is on
boolean displayOn = true; // whether or not display on screen
// Initialize counters
unsigned short potLevel;       // pot level
unsigned long buttonTimer = 0; // current click duration
unsigned char currentMenu = 0; // current choosen menu
char choice = '0';             // current user choice in menu (day, month, year, hour,...)
long remainingTime;            // remaining time before alarm (in seconds)
unsigned short currentYearClock;
char currentMonthClock;
char currentDayClock;
char currentMinClock;
char currentHourClock;
char currentMinAlarm;
char currentHourAlarm;
unsigned char currentDurationAlarm;
boolean adjustDST(RtcDateTime &time_)
{
  // ********************* Calculate offset for Sunday *********************
  boolean DST;
  int y = time_.Year();        // DS3231 uses two digit year (required here)
  int x = (y + y / 4 + 2) % 7; // remainder will identify which day of month
  // is Sunday by subtracting x from the one
  // or two week window.  First two weeks for March
  // and first week for November
  // *********** Test DST: BEGINS on 2nd Sunday of March @ 2:00 AM *********
  if (time_.Month() == 3 && time_.Day() == (14 - x) && time_.Hour() >= 2)
  {
    DST = true; // Daylight Savings Time is TRUE (add one hour)
  }
  if ((time_.Month() == 3 && time_.Day() > (14 - x)) || time_.Month() > 3)
  {
    DST = true;
  }
  // ************* Test DST: ENDS on 1st Sunday of Nov @ 2:00 AM ************
  if (time_.Month() == 11 && time_.Day() == (7 - x) && time_.Hour() >= 2)
  {
    DST = false; // daylight savings time is FALSE (Standard time)
  }
  if ((time_.Month() == 11 && time_.Day() > (7 - x)) || time_.Month() > 11 || time_.Month() < 3)
  {
    DST = false;
  }
  if (DST == true) // Test DST and add one hour if = 1 (TRUE)
  {
    time_ = time_ + 3600;
  }
  return DST;
}
void displayInfo(char info[])
{
  // ********************* Displays a text string *********************
  display.clearDisplay();
  display.setTextSize(2);                         // 2:1 pixel scale
  display.setTextColor(WHITE);                    // Draw white text
  display.setCursor(4 * (10 - strlen(info)), 15); // Centers the text horizontally
  display.println(info);
  return;
}
void displayInfo(char info[], char info2[])
{
  // ********************* Displays two strings *********************
  display.clearDisplay();
  display.setTextSize(2);                         // 2:1 pixel scale
  display.setTextColor(WHITE);                    // Draw white text
  display.setCursor(4 * (10 - strlen(info)), 15); // Centers the text horizontally
  display.println(info);
  display.setTextSize(1);                        // Normal 1:1 pixel scale
  display.setCursor(4 * (10 - strlen(info)), 0); // Centers the text horizontally
  display.println(info2);
  return;
}
void displayInfo(char info[], char info2[], const unsigned char *picture)
{
  // ********************* Displays two strings and a picture *********************
  displayInfo(info, info2);
  display.drawBitmap(
      (display.width() - 16 - 10),
      (display.height() - 16 - 8),
      picture, 16, 16, 1);
  return;
}
void displayInfo(char info[], const unsigned char *picture)
{
  // ********************* Displays one string and a picture *********************
  displayInfo(info);
  display.drawBitmap(
      (display.width() - 16 - 10),
      (display.height() - 16 - 8),
      picture, 16, 16, 1);
  return;
}
void displayInfo(char info[], short blink_index[])
{
  // ********************* Displays a string and blinks specified digits *********************
  char info_cp[20];
  strcpy(info_cp, info);
  if ((millis() % BLINK_DURATION) > BLINK_DURATION / 2)
  {
    for (int i = blink_index[0]; i <= blink_index[1]; i++)
      info_cp[i] = ' '; // to blink we replace temporarily the digits with spaces
  }
  displayInfo(info_cp);
}
void displayInfo(char info[], const unsigned char *picture, short blink_index[])
{
  // ********************* Displays a string, a picture and blinks specified digits *********************
  displayInfo(info, blink_index);
  display.drawBitmap(
      (display.width() - 16 - 10),
      (display.height() - 16 - 8),
      picture, 16, 16, 1);
}
void welcome()
{
  display.clearDisplay();
  for (int posx = 0; posx <= 128 - 32; posx += 8)
  {
    display.drawBitmap(
        (posx),
        (0),
        smiley, 32, 32, 1);
    display.display();
    delay(100);
    display.clearDisplay();
  }
  for (int posx = 128 - 32; posx >= 0; posx -= 8)
  {
    display.drawBitmap(
        (posx),
        (0),
        smiley, 32, 32, 1);
    display.display();
    delay(100);
    display.clearDisplay();
  }
  delay(2000);
}
unsigned char readButtons_setTime()
{
  // ********************* Reads the two buttons in the set time context *********************
  unsigned char button_press = 0;
  // Possibilities are 00 : None - None, 10 : Press - None, 01 : None - Press, 11 : Press - Press
  //////////////////////////////////////////////////////////
  // Read buttons
  if (digitalRead(BUTTON_L) == LOW && digitalRead(BUTTON_R) == LOW)
  {
    button_press = 11;
  }
  else if (digitalRead(BUTTON_L) == LOW)
  {
    button_press = 10;
  }
  else if (digitalRead(BUTTON_R) == LOW)
  {
    button_press = 1;
  }
  else
  {
    button_press = 0;
  }
  //////////////////////////////////////////////////////////
  return button_press;
}
unsigned char readButtons()
{
  // ********************* Reads the two buttons in the standard menu context *********************
  unsigned char button_press = 0;
  // Possibilities are 00 : None - None, 01 : None - ShortPress, 10 : ShortPress - None,
  // 11 : ShortPress - ShortPress, 02 : None - LongPress, 20 : LongPress - None, 22 : LongPress - LongPress
  //////////////////////////////////////////////////////////
  // Read buttons
  if (digitalRead(BUTTON_L) == LOW)
  {
    if (buttonActive == false)
    {
      buttonActive = true;
      buttonTimer = millis();
    }
    button1Active = true;
  }
  if (digitalRead(BUTTON_R) == LOW)
  {
    if (buttonActive == false)
    {
      buttonActive = true;
      buttonTimer = millis();
    }
    button2Active = true;
  }
  if ((buttonActive == true) && ((unsigned long)(millis() - buttonTimer) > BUTTON_TIME) && (longPressActive == false))
  {
    longPressActive = true;
    if ((button1Active == true) && (button2Active == true))
    {
      button_press = 22;
    }
    else if ((button1Active == true) && (button2Active == false))
    {
      button_press = 20;
    }
    else
    {
      button_press = 2;
    }
  }
  if ((buttonActive == true) && (digitalRead(BUTTON_L) == HIGH) && (digitalRead(BUTTON_R) == HIGH))
  {
    if (longPressActive == true)
    {
      longPressActive = false;
    }
    else
    {
      if ((button1Active == true) && (button2Active == true))
      {
        button_press = 11;
      }
      else if ((button1Active == true) && (button2Active == false))
      {
        button_press = 10;
      }
      else
      {
        button_press = 1;
      }
    }
    buttonActive = false;
    button1Active = false;
    button2Active = false;
  }
  return button_press;
}
void setup()
{
  // Setup code that is run only once
  // Serial.begin(9600);  //Starts serial connection
  // Serial.setTimeout(2000);
  pinMode(BUTTON_LED_OUT, OUTPUT); // Controls button led brightness
  pinMode(FET_OUT, OUTPUT);        // Controls main led brightness
  analogWrite(FET_OUT, 0);         // turn off main led
  pinMode(BUTTON_R, INPUT_PULLUP); // Input right button
  pinMode(BUTTON_L, INPUT_PULLUP); // Input left button
  // Initialize RTC module
  rtcObject.Begin(); // Starts I2C
  // Retrieve user specifications from EEPROM
  if (EEPROM.read(0) == 126)
  { // Indicates that it has been written already
    currentHourAlarm = EEPROM.read(1);
    currentMinAlarm = EEPROM.read(2);
    currentDurationAlarm = EEPROM.read(3);
  }
  else
  { // Set defaults, since nothing in memory
    currentHourAlarm = 7;
    currentMinAlarm = 30;
    currentDurationAlarm = 30;
  }
  currentMenu = 0; // start with menu zero
                   // Start small welcoming animation (smiley)
  welcome();
}
void setDuration(unsigned char &curDuration)
{
  // ********************* Sets the duration of the alarm *********************
  unsigned char button_press = readButtons_setTime(); // Read buttons
  // Left button => decrease value, right button => increase value
  // Both buttons => Accept value and pursue
  if (button_press == 1)
  {
    curDuration += 1;
  }
  else if (button_press == 10)
  {
    curDuration -= 1;
  }
  else if (button_press == 11)
  {
    currentMenu = 0;    // Return to main menu
    updateAlarm = true; // Flag to inform that alarm has been updated
  }
  // Display current duration value and hourglass image
  char str[3];       // declare a string as an array of chars
  sprintf(str, "%d", //%d allows to print an integer to the string
          curDuration);
  if (displayOn)
  {
    displayInfo(str, hourglass);
    display.display();
  }
  return;
}
void setDate(unsigned short &curYear, char &curMonth, char &curDay)
{
  // ********************* Sets the current date *********************
  short blink_index[2];                               // will be updated
  unsigned char button_press = readButtons_setTime(); // Read buttons
  if (choice == 'y')
  {                     // year setup step
    blink_index[0] = 0; // blink from digit 0
    blink_index[1] = 3; //        to digit 3
    if (button_press == 1)
    {
      curYear += 1;
    }
    else if (button_press == 10)
    {
      curYear -= 1;
    }
    else if (button_press == 11)
    {
      choice = 'm'; // jump to month setup step
      delay(1000);  // wait a bit to avoid fast clicks
    }
  }
  else if (choice == 'm')
  {                     // month setup step
    blink_index[0] = 5; // blink from digit 5
    blink_index[1] = 6; //        to digit 6
    if (button_press == 1)
    {
      curMonth += 1;
      if (curMonth > 12)
      {
        curMonth = 1; // avoid month > 12
      }
    }
    else if (button_press == 10)
    {
      curMonth -= 1;
      if (curMonth < 1)
      {
        curMonth = 12; // avoid month < 1
      }
    }
    else if (button_press == 11)
    {
      choice = 'd'; // jump to day setup step
      delay(1000);  // wait a bit to avoid fast clicks
    }
  }
  else if (choice == 'd')
  {
    blink_index[0] = 8; // blink from digit 8
    blink_index[1] = 9; //        to digit 9
    if (button_press == 1)
    {
      curDay += 1;
      if (curDay > daysPerMonth[curMonth])
      {
        curDay = 1; // avoid impossible day
      }
    }
    else if (button_press == 10)
    {
      curDay -= 1;
      if (curDay < 1)
      {
        curDay = 1; // avoid day < 1
      }
    }
    else if (button_press == 11)
    {
      choice = 'h';    // jump to hour setup step
      delay(1000);     // wait a bit to avoid fast clicks
      currentMenu = 2; // go to menu 2 (time setup)
    }
  }
  // Create date string in the format YYYY-MM-DD, ex. 2019-06-30
  char str[11];                  // declare a string as an array of chars
  sprintf(str, "%04d-%02d-%02d", //%d allows to print an integer to the string
          curYear,
          curMonth,
          curDay);
  if (displayOn)
  {
    displayInfo(str, blink_index);
    display.display();
  }
  return;
}
void setTime(char &curHour, char &curMin, boolean setAlarm = false)
{
  short blink_index[2];
  unsigned char button_press = readButtons_setTime();
  if (choice == 'h')
  {
    blink_index[0] = 0;
    blink_index[1] = 1;
    if (button_press == 1)
    {
      curHour += 1;
      if (curHour > 23)
      {
        curHour = 0;
      }
    }
    else if (button_press == 10)
    {
      curHour -= 1;
      if (curHour < 0)
      {
        curHour = 23;
      }
    }
    else if (button_press == 11)
    {
      choice = 'm';
      delay(1000);
    }
  }
  else if (choice == 'm')
  {
    blink_index[0] = 3;
    blink_index[1] = 4;
    if (button_press == 1)
    {
      curMin += 1;
      if (curMin > 59)
      {
        curMin = 0;
      }
    }
    else if (button_press == 10)
    {
      curMin -= 1;
      if (curMin < 0)
      {
        curMin = 59;
      }
    }
    else if (button_press == 11)
    {
      if (setAlarm)
      {
        currentMenu = 4;
      }
      else
      {
        updateTime = true;
        currentMenu = 0;
      }
    }
  }
  char str[11];             // declare a string as an array of chars
  sprintf(str, "%02d:%02d", //%d allows to print an integer to the string
          curHour,          // get hour method
          curMin            // get minute method
  );
  if (displayOn)
  {
    displayInfo(str, clockhour, blink_index);
    display.display();
  }
  return;
}
void loop()
{
  // Main code that is run repeatedly
  // ADJUSTING TIME AND ALARM
  //--------------------------------
  // Start by updating RTC module if new time has been chosen
  if (updateTime)
  {
    RtcDateTime currentTime = RtcDateTime(currentYearClock, currentMonthClock, currentDayClock,
                                          currentHourClock, currentMinClock, 0); // define date and time object
    if (isDST)
    { // correct for DST if needed
      currentTime -= 3600;
    }
    rtcObject.SetDateTime(currentTime);
    updateTime = false;
  }
  // Also update alarm in EEPROM if new alarm setup has been chosen
  if (updateAlarm)
  {
    EEPROM.write(1, currentHourAlarm);
    EEPROM.write(2, currentMinAlarm);
    EEPROM.write(3, currentDurationAlarm);
    updateAlarm = false;
  }
  // ADJUSTING BUTTON LED AND OLED BRIGHTNESS
  //--------------------------------
  // Now we read the value of the button led pot
  // We get average of 5 measurements
  int niter = 5;
  float potLevel = 0;
  for (int i = 0; i < niter; i++)
  {
    potLevel += float(analogRead(POT_IN));
    delay(5);
  }
  potLevel /= niter;
  // Normalize pot level between 0 and 1
  float level = (float)potLevel / 1023.;
  level = pow(level, 2); // Use power of two to get more smooth transition
  // if pot level below a certain value, we disable oled screen
  if (level < 0.02)
  {
    displayOn = false;
  }
  else
  {
    displayOn = true;
  }
  // Finally set button led brightness value
  analogWrite(BUTTON_LED_OUT, int(level * 255));
  // Also control OLED brightness (doesn't work very well)
  display.ssd1306_command(0x81);
  display.ssd1306_command(int(level * 160)); // max 160
  display.ssd1306_command(0xD9);
  display.ssd1306_command(int(level * 34)); // max 34
  // MANAGE ALARM
  //--------------------------------
  RtcDateTime currentTime = rtcObject.GetDateTime(); // get the time from the RTC
  // COrrect  for daylight saving time
  isDST = adjustDST(currentTime); // Get DST
  if (currentMenu != 1 && currentMenu != 2)
  { // menu 1 and 2 set the date and time manually, so don't update from RTC
    currentYearClock = currentTime.Year();
    currentMonthClock = currentTime.Month();
    currentDayClock = currentTime.Day();
    currentHourClock = currentTime.Hour();
    currentMinClock = currentTime.Minute();
  }
  // Compute remaining time until alarm starts in seconds
  remainingTime = 3600L * (int(currentHourAlarm) - int(currentHourClock)) + 60L * (int(currentMinAlarm) - int(currentMinClock)) - int(currentTime.Second());
  if (remainingTime < 0)
  {
    remainingTime += (24 * 3600L);
  }
  // Alarm starts only if time until specified alarm time is less than specified duration
  // and in main menu (user is not changing settings) and alarm is enabled
  if ((remainingTime < currentDurationAlarm * 60 && currentMenu == 0 && alarmEnabled))
  {
    // Start to light the led
    float level = 1. - float(remainingTime) / (60 * currentDurationAlarm); // fraction of time remaining until alarm
    level = 255 * pow(level, 2);                                           // power of 2 for smoother transition
    if (level < 1)
    {
      level = 1; // ensure minimal brightness
    }
    analogWrite(FET_OUT, level);
    alarmRunning = true;
  }
  else
  { // if alarm is running continue to light LED at max brightness even after alarm time has passed
    // (it makes no sense to stop it immediately as person might not yet have woken up)
    if (alarmRunning)
    {
      analogWrite(FET_OUT, 255);
    }
  }
  // MANAGE MENUS
  //--------------------------------
  if (currentMenu == 0)
  {
    // Main menu
    unsigned char buttonPress = readButtons();
    // 4 possibilities
    if (buttonPress == 10)
    { // short press left
      // Toggle on/off light
      if (mainLightOn)
      {
        analogWrite(FET_OUT, 0);
        mainLightOn = false;
      }
      else
      {
        analogWrite(FET_OUT, 255);
        mainLightOn = true;
      }
    }
    else if (buttonPress == 1)
    { // short press right
      // Toggle on/off alarm
      if (alarmEnabled)
      {
        alarmEnabled = false;
        // turn off lamp
        analogWrite(FET_OUT, 0);
      }
      else
      {
        alarmEnabled = true;
      }
    }
    else if (buttonPress == 20)
    { // long press left
      // Jump into date selection menu
      choice = 'y'; // Start with year selection
      delay(1000);
      currentMenu = 1;
    }
    else if (buttonPress == 2)
    { // short press right
      // Jump into alarm selection menu
      choice = 'h'; // Start with hour
      currentMenu = 3;
      delay(1000);
    }
    // DISPLAY INFO
    //--------------------------------
    char str[5];                // declare a string as an array of chars
    sprintf(str, "%02d:%02d",   //%d allows to print an integer to the string
            currentTime.Hour(), // get hour method
            currentTime.Minute());
    if (displayOn)
    {
      if (alarmEnabled)
      {                            // time + bell image
        char str2[5];              // declare a string as an array of chars
        sprintf(str2, "%02d:%02d", //%d allows to print an integer to the string
                currentHourAlarm,  // get hour method
                currentMinAlarm    // get minute method
        );
        displayInfo(str, str2, bell);
      }
      else
      { // only time
        displayInfo(str);
      }
    }
    else
    { // Display empty string to show nothing on screen
      displayInfo("");
    }
    display.display();
    // UPDATE USER CHANGES
    //--------------------------------
    if (updateTime)
    { // set new time on RTC
      RtcDateTime currentTime = RtcDateTime(currentYearClock, currentMonthClock, currentDayClock,
                                            currentHourClock, currentMinClock, 0); // define date and time object
      if (isDST)
      {
        currentTime -= 3600;
      }
      rtcObject.SetDateTime(currentTime);
      updateTime = false;
    }
    if (updateAlarm)
    { // write alarm setup in EEPROM
      EEPROM.write(1, currentHourAlarm);
      EEPROM.write(2, currentMinAlarm);
      EEPROM.write(3, currentDurationAlarm);
      updateAlarm = false;
    }
    if (!alarmEnabled)
    {
      // turn off alarm if it is started
      if (alarmRunning)
      {
        alarmRunning = false;
      }
    }
  }
  // THINGS TO DO IN OTHER MENUS
  //--------------------------------
  else if (currentMenu == 1)
  {
    // set clock
    setDate(currentYearClock, currentMonthClock, currentDayClock);
  }
  else if (currentMenu == 2)
  {
    setTime(currentHourClock, currentMinClock, false);
  }
  else if (currentMenu == 3)
  {
    setTime(currentHourAlarm, currentMinAlarm, true);
  }
  else if (currentMenu == 4)
  {
    setDuration(currentDurationAlarm);
  }
  delay(200);
}