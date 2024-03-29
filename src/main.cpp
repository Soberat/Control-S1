#include "Arduino.h"
#include <FastLED.h>
#include <Wire.h>
#include <SerialFlash.h>
#include <Encoder.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <Control_Surface.h>
#include "TrackDataHandler.h"

#ifdef USB_MIDI16_AUDIO_SERIAL
#include <Audio.h>
//Teensy Audio objects and connections
AudioInputUSB            usb1;
AudioOutputI2S           i2s1;
AudioConnection          patchCord1(usb1, 0, i2s1, 0);
AudioConnection          patchCord2(usb1, 1, i2s1, 1);
#endif

//TODO: Open correct advanced tab when changing banks

/*
 * Deck inputs:
 * 2x15 muxed down to 4 + 2 - Gain, High, Mid, Low, Filter, Cue Enable, Volume, selector 1234, play, cue, load, shift
 * 6 - crossfader, volume master, browser, master volume, monitor volume, cruise mode
 * 1 - WS2812
 * 5x2 - encoders
 * 2 - I2C
 * 3 - I2S
 */

//When using audio in Traktor use Shared Mode instead of Exclusive. Otherwise it's gonna glitch at every buffer size for some reason (after some time at least)

USBMIDI_Interface midi; 

const unsigned char PROGMEM logoTraktor[] = {
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xff, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xff, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xff, 0xff, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xf0, 0x07, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0x80, 0x01, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0x00, 0x00, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xf8, 0x07, 0xf0, 0x1f, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xf0, 0x3f, 0xfc, 0x0f, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xe0, 0xff, 0xff, 0x07, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xc1, 0xff, 0xff, 0x83, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xc3, 0xff, 0xff, 0xc3, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x87, 0xff, 0xff, 0xe1, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x83, 0xff, 0xff, 0xc0, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x01, 0xff, 0xff, 0x80, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x18, 0xff, 0xff, 0x08, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1c, 0x78, 0x1e, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xfe, 0x30, 0x0c, 0x7c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0x00, 0x00, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0x80, 0x01, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xc0, 0x03, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xc0, 0x03, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0x80, 0x01, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0x00, 0x00, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xfe, 0x30, 0x0c, 0x7f, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1c, 0x78, 0x1e, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x1e, 0x18, 0xff, 0xff, 0x18, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x01, 0xff, 0xff, 0x80, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x03, 0xff, 0xff, 0xc0, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x87, 0xff, 0xff, 0xe1, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xc3, 0xff, 0xff, 0xe1, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xc1, 0xff, 0xff, 0xc3, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xe0, 0xff, 0xff, 0x07, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xf0, 0x7f, 0xfe, 0x0f, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xf8, 0x0f, 0xf0, 0x1f, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0x00, 0x00, 0x3f, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xe0, 0x07, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xff, 0xff, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xff, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xff, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

/* !!!
 * Traktor outputs midi for Numark on channels 1 and 2. To avoid conflict, don't use CC range [32-77] or simply switch Control output to any channel higher than 2.
 * I'm outputting Numark on different virtual cables so I don't have any conflicts, although it might be a bit overdone.
 */
Adafruit_SSD1306 displayA(128, 64, &Wire, 4);
Adafruit_SSD1306 displayB(128, 64, &Wire, 4);

TrackDataHandler deckA(0x02, 0xB0);
TrackDataHandler deckB(0x22, 0xB1);

//Track end warnings don't have to be synchronized between decks, so we need 2 separate timers
//This value is arbitarily chosen to match Traktor's flashing interval
Timer<millis> timerEndA = 790;
Timer<millis> timerEndB = 790;

Timer<millis> second = 1000;
Timer<millis> displayDelay = 1000;

Timer<millis> activity = 500;
bool activityNoteToggle = true; // Store current state of note that is being sent to Traktor to track connection status
bool active = true;

//Variables containing information if Track End Warning is active for a deck
bool trackEndA = false;
bool trackEndB = false;

// Input components
// Channel 01 - Deck A
// Channel 02 - Deck B
// Channel 03 - Global

CD74HC4067 mux  = {A7, {A2, A3, A1, A0}};
CD74HC4067 mux2 = {A6, {A2, A3, A1, A0}};

int prevBankASelection = 0;
int prevBankBSelection = 0;

String bankNames[] {"Hotcues 1-2", "Hotcues 3-4", "Hotcues 5-6", "Hotcues 7-8", "Syncing", "Looper", "Tempo", "Beatjump"};

Bank<8> bankA(4); // 4 cue selectors, looper, beatjump, sync, tempo
Bank<8> bankB(4); // 1 encoder, 2 pushbuttons, 1 encoder button, total range of 28 CCs x2

EncoderSelector<8> selectorA {bankA, {0, 1}, 4, Wrap::Clamp};
EncoderSelector<8> selectorB {bankB, {11, 12}, 4, Wrap::Clamp};

//CC range [10-37] is reserved for the function selectors and buttons
Bankable::CCButton button1A = {
    {bankA, BankType::CHANGE_ADDRESS},
     mux.pin(10),
    {10, CHANNEL_1}};
    
Bankable::CCButton button2A = {
    {bankA, BankType::CHANGE_ADDRESS},
     mux.pin(9),
    {11, CHANNEL_1}};

Bankable::CCButton buttonEncA = {
    {bankA, BankType::CHANGE_ADDRESS},
     mux.pin(0),
    {12, CHANNEL_1}};

Bankable::CCRotaryEncoder encoderA= {
    {bankA, BankType::CHANGE_ADDRESS},
    {2,3},
    {13, CHANNEL_1}};

Bankable::CCButton button1B = {
    {bankB, BankType::CHANGE_ADDRESS},
     mux2.pin(9),
    {10, CHANNEL_2}};
    
Bankable::CCButton button2B = {
    {bankB, BankType::CHANGE_ADDRESS},
     mux2.pin(10),
    {11, CHANNEL_2}};

Bankable::CCButton buttonEncB = {
    {bankB, BankType::CHANGE_ADDRESS},
     mux2.pin(13),
    {12, CHANNEL_2}};

Bankable::CCRotaryEncoder encoderB = {
    {bankB, BankType::CHANGE_ADDRESS},
    {6, 7},
    {13, CHANNEL_2}};    

CCPotentiometer potVolumeA = {mux.pin(8), {0, CHANNEL_1}}; 
CCPotentiometer potGainA   = {mux.pin(15), {1, CHANNEL_1}};
CCPotentiometer potHighA   = {mux.pin(14), {2, CHANNEL_1}}; 
CCPotentiometer potMidA    = {mux.pin(13), {3, CHANNEL_1}}; 
CCPotentiometer potLowA    = {mux.pin(12), {4, CHANNEL_1}}; 
CCPotentiometer potFilterA = {mux.pin(11), {5, CHANNEL_1}}; 

CCButton buttonPlayA = {mux.pin(1),{6, CHANNEL_1}};
CCButton buttonCueA = {mux.pin(2),{7, CHANNEL_1}};
CCButton buttonLoadA = {mux.pin(3),{8, CHANNEL_1}};
CCButton buttonCueEnableA = {mux.pin(7), {9, CHANNEL_1}}; 

CCPotentiometer potVolumeB = {mux2.pin(8), {0, CHANNEL_2}};
CCPotentiometer potGainB   = {mux2.pin(2), {1, CHANNEL_2}};
CCPotentiometer potHighB   = {mux2.pin(3), {2, CHANNEL_2}}; 
CCPotentiometer potMidB    = {mux2.pin(4), {3, CHANNEL_2}}; 
CCPotentiometer potLowB    = {mux2.pin(5), {4, CHANNEL_2}}; 
CCPotentiometer potFilterB = {mux2.pin(6), {5, CHANNEL_2}}; 

CCButton buttonPlayB = {mux2.pin(11),{6, CHANNEL_2}};
CCButton buttonCueB  = {mux2.pin(12),{7, CHANNEL_2}};
CCButton buttonLoadB = {mux2.pin(15),{8, CHANNEL_2}};
CCButton buttonCueEnableB = {mux2.pin(14), {9, CHANNEL_2}}; 

CCPotentiometer potXfader  = {mux.pin(4), {0, CHANNEL_3}};
CCPotentiometer potVolumeMonitor ={mux2.pin(0), {1, CHANNEL_3}}; 
CCPotentiometer potVolumeMaster  = {mux2.pin(1), {2, CHANNEL_3}};

CCButton buttonModifier1 = {mux.pin(6), {3, CHANNEL_3}}; //shift 
CCButton buttonModifier2 = {8, {4, CHANNEL_3}}; //browser encoder pushbutton, used only for browser navigation.
CCButton buttonCruise = {13, {5, CHANNEL_3}};

CCRotaryEncoder encoderBrowser = {{4, 5}, {6, CHANNEL_3}};

// LED components
CRGB dimGreen = CRGB(0, 32, 0);
CRGB dimBlue = CRGB(0, 0, 32);

CRGB vuColors[8] = {CRGB::Green, CRGB::Green, CRGB::Green, CRGB::Green, CRGB::Green, CRGB::Yellow, CRGB::Yellow, CRGB::Red};
//CRGB vuColors[8] = {CRGB::DarkBlue, CRGB::DarkBlue, CRGB::DarkBlue, CRGB::DarkBlue, CRGB::DarkBlue, CRGB::DarkOrange, CRGB::DarkOrange, CRGB::DarkOrange};

//Array storing LED information about 8 hotcues, sync/master status and loop status
CRGB deckASelectorLEDS[12] = {CRGB::Black, CRGB::Black, CRGB::Black, CRGB::Black, CRGB::Black, CRGB::Black, CRGB::Black, CRGB::Black, dimBlue, dimBlue, dimGreen, dimGreen};
CRGB deckBSelectorLEDS[12] = {CRGB::Black, CRGB::Black, CRGB::Black, CRGB::Black, CRGB::Black, CRGB::Black, CRGB::Black, CRGB::Black, dimBlue, dimBlue, dimGreen, dimGreen};

// The data pin with the strip connected.
constexpr uint8_t ledpin = 10;
// Total number of leds connected to FastLED
constexpr uint8_t numleds = 50;
// How many CCs are sent to control LEDs
constexpr uint8_t ledCallbacks = 20;

// Return a color based on the type of the cue
CRGB cueType(int num) {
    if (num == 0)  return CRGB(32, 32, 32);   //No cue type
    if (num == 1)  return CRGB::DodgerBlue;   //Cue
    if (num == 2)  return CRGB::DarkOrange;   //Fade In
    if (num == 3)  return CRGB::DarkOrange;   //Fade Out
    if (num == 4)  return CRGB::Gold;         //Load
    if (num == 5)  return CRGB::DarkGray;     //Grid
    if (num == 6)  return CRGB::Green;        //Loop

    return CRGB::Azure;
}

// Custom callback to handle incoming note events and control the LEDs
template <uint8_t RangeLen>
class CustomNoteLED : public MatchingMIDIInputElement<MIDIMessageType::NOTE_ON,
                                                    TwoByteRangeMIDIMatcher> {
    public:
        CustomNoteLED(CRGB *ledcolors, MIDIAddress address) 
          : MatchingMIDIInputElement<MIDIMessageType::NOTE_ON,
                                     TwoByteRangeMIDIMatcher>({address, RangeLen}),
          ledcolors(ledcolors) {}
            
        //TODO clipped
            
        // Called once upon initialization.
        void begin() override {}
    
        /*
         * This function has the job of controlling WS2812 Neopixels
         * In this case we are using multiple Neopixels connected in series to reduce pin usage on the uC
         * They are connected as :
         * Selector A -> Status A -> Volume A -> Cue Enable A -> Cue Enable B -> Volume B -> Phase Shift -> Status B -> Selector B -> Master Out -> Cruise -> Master 'ring'
         * Declaration of CustomNoteValueLED below defines a range of notes starting at a given note to be received and handled by the class.
         * Velocity of the Nth note is stored as index-1 in the input variable
         * Notes need to be one after another for this to work. 
         * 
         * It is crucial that the leds are not cleared with FastLED.clear(), as this introduces a lot of flickering
         * 
         * On Arduino Uno some notes may stay on even after a note signals to turn it off. No idea what's causing this, but the midi note is sent correctly.
         */
        void handleUpdate(typename TwoByteRangeMIDIMatcher::Result match) override {
            int value = match.value;
            switch (match.index) {
                case 0: // Hotcue 1 A
                    deckASelectorLEDS[0] = cueType(value);
                    break;
                case 1: // Hotcue 1 B
                    deckBSelectorLEDS[0] = cueType(value);
                    break;
                case 2: // Hotcue 2 A
                    deckASelectorLEDS[1] = cueType(value);
                    break;
                case 3: // Hotcue 2 B
                    deckBSelectorLEDS[1] = cueType(value);
                    break;
                case 4: // Hotcue 3 A
                    deckASelectorLEDS[2] = cueType(value);
                    break;
                case 5: // Hotcue 3 B
                    deckBSelectorLEDS[2] = cueType(value);
                    break;
                case 6: // Hotcue 4 A
                    deckASelectorLEDS[3] = cueType(value);
                    break;
                case 7: // Hotcue 4 B
                    deckBSelectorLEDS[3] = cueType(value);
                    break;
                case 8: // Hotcue 5 A
                    deckASelectorLEDS[4] = cueType(value);
                    break;
                case 9: // Hotcue 5 B
                    deckBSelectorLEDS[4] = cueType(value);
                    break;
                case 10: // Hotcue 6 A
                    deckASelectorLEDS[5] = cueType(value);
                    break;
                case 11: // Hotcue 6 B
                    deckBSelectorLEDS[5] = cueType(value);
                    break;
                case 12: // Hotcue 7 A
                    deckASelectorLEDS[6] = cueType(value);
                    break;
                case 13: // Hotcue 7 B
                    deckBSelectorLEDS[6] = cueType(value);
                    break;
                case 14: // Hotcue 8 A
                    deckASelectorLEDS[7] = cueType(value);
                    break;
                case 15: // Hotcue 8 B
                    deckBSelectorLEDS[7] = cueType(value);
                    break;
                case 16: // Sync on A
                    if (value > 0) deckASelectorLEDS[8] = CRGB::Blue;
                    else deckASelectorLEDS[8] = dimBlue;
                    break;
                case 17: // Sync on B
                    if (value > 0) deckBSelectorLEDS[8] = CRGB::Blue;
                    else deckBSelectorLEDS[8] = dimBlue;
                    break;
                case 18: // Is master A
                    if (value > 0) deckASelectorLEDS[9] = CRGB::Blue;
                    else deckASelectorLEDS[9] = dimBlue;
                    break;
                case 19: // Is master A
                    if (value > 0) deckBSelectorLEDS[9] = CRGB::Blue;
                    else deckBSelectorLEDS[9] = dimBlue;
                    break;
                case 20: // Loop in A
                    if (value > 0) deckASelectorLEDS[10] = CRGB::Green;
                    else deckASelectorLEDS[10] = dimGreen;
                    break;
                case 21: // Loop in B
                    if (value > 0) deckBSelectorLEDS[10] = CRGB::Green;
                    else deckBSelectorLEDS[10] = dimGreen;
                    break;
                case 22: // Loop out A
                    if (value > 0) deckASelectorLEDS[11] = CRGB::Green;
                    else deckASelectorLEDS[11] = dimGreen;
                    break;
                case 23: // Loop out B
                    if (value > 0) deckBSelectorLEDS[11] = CRGB::Green;
                    else deckBSelectorLEDS[11] = dimGreen;
                    break;                    
                case 24: // Status deck A
                    if (value > 0) ledcolors[2] = CRGB::Green;
                    else ledcolors[2] = CRGB::Black;
                    break;
                case 25: // Status deck B
                    if (value > 0) ledcolors[30] = CRGB::Green;
                    else ledcolors[30] = CRGB::Black;
                    break;                 
                case 26: // Track End A
                    if (value > 0) {
                      trackEndA = true;
                      timerEndA.begin();
                    }
                    else trackEndA = false;
                    break;
                case 27: // Track End B
                    if (value > 0) {
                      trackEndB = true;
                      timerEndB.begin();
                    }
                    else trackEndB = false;
                    break;
                case 28:  // Volume deck A
                    for (int i = 0; i <= 7; i++) if (value > 16*i) {
                        ledcolors[4+i] = vuColors[i];
                    } else {
                        ledcolors[4+i] = CRGB::Black;
                    }
                    break;
                case 29:  // Volume deck B
                    for (int i = 7; i >= 0; i--) if (value > 16*i) {
                        ledcolors[21-i] = vuColors[i];
                    } else {
                        ledcolors[21-i] = CRGB::Black;
                    }
                    break;
                case 30: // Cue Enable A
                    if (value > 0) ledcolors[12] = CRGB::Orange;
                    else ledcolors[12] = CRGB::Black;
                    break;
                case 31: // Cue Enable B
                    if (value > 0) ledcolors[13] = CRGB::Orange;
                    else ledcolors[13] = CRGB::Black;
                    break;
                case 32: // Phase shift indicator
                    //This is responsible for displaying the phase the same way Traktor does, as in center is no phase shift.
                    //Since midi values are 0-127 the code is quite ugly, but it works ;)                  
                    for (int i = 0; i <= 7; i++) ledcolors[22+i] = CRGB::Black;
                    if (value == 63 || value == 0) break; //default value is 63, which means so phase shift. 0 is the default when the device starts without MIDI input, so we ignore it as well
                    else if (value >= 0  && value < 15) for (int i = 0; i <= 3; i++) ledcolors[22+i] = CRGB::Orange;
                    else if (value >= 15 && value < 31) for (int i = 1; i <= 3; i++) ledcolors[22+i] = CRGB::Orange;
                    else if (value >= 31 && value < 47) for (int i = 2; i <= 3; i++) ledcolors[22+i] = CRGB::Orange;
                    else if (value >= 47 && value < 63) ledcolors[25] = CRGB::Orange;
                    else if (value > 63 && value <= 79) ledcolors[26] = CRGB::Orange;
                    else if (value > 79 && value <= 95) for (int i = 4; i <= 5; i++) ledcolors[22+i] = CRGB::Orange;
                    else if (value > 95 && value <= 111) for (int i = 4; i <= 6; i++) ledcolors[22+i] = CRGB::Orange;
                    else if (value > 111) for (int i = 4; i <= 7; i++) ledcolors[22+i] = CRGB::Orange;
                    break;
                case 33: // Master out 
                    for (int i = 7; i >= 0; i--) if (value > 16*i) {
                        ledcolors[41-i] = vuColors[i];
                    } else {
                        ledcolors[41-i] = CRGB::Black;
                    }
                    break;                
                case 34: // Cruise mode status
                    if (value > 0) ledcolors[42] = CRGB::Blue;
                    else ledcolors[42] = CRGB::Black;
                    break;
                case 35: // Master level (position of knob)
                    for (int i = 0; i <= 6; i++) if (value > 21*i) {
                        ledcolors[43+i] = CRGB::DarkBlue;
                    } else {
                        ledcolors[43+i] = CRGB::Black;
                    }
                    break;
            }       
        }
               
    private:
        // Pointer to array of FastLED color values for the LEDs
        CRGB *ledcolors;
};
 
// Define the array of leds and the LED input element
Array<CRGB, numleds> leds {};
CustomNoteLED<numleds> midiled {leds.data, MIDI_Notes::C(-1)};

CCValue activityTracker {{MIDI_Notes::G(9), CHANNEL_3, Cable(3)}};

//Function that changes the channel of TCA9548A I2C multiplexer
void channel(uint8_t bus) {
    Wire.beginTransmission(0x70);
    Wire.write(1 << bus);
    Wire.endTransmission();
}


bool sysExMessageCallback(SysExMessage se) {
    //making sure the data is coming from Traktor and that length corresponds to title data message length (6 ascii + 16 id)
    if (se.data[0] == 0xF0 && se.data[se.length-1] == 0xF7 && se.length == 22) {
        if (se.getCable().getRaw() == 1) {
            deckA.receive(se);
        } else if (se.getCable().getRaw() == 2) {
            deckB.receive(se);
        }
    }
    return false;
}

bool channelMessageCallback(ChannelMessage cm) {
    if (cm.data1 >= 32 && cm.data1 <= 77) {
        if (cm.getChannelCable().getRawCableNumber() == 1) {
            deckA.receive(cm);
        } else if (cm.getChannelCable().getRawCableNumber() == 2) {
            deckB.receive(cm);
        }
    }
    return false;
}


void trackEndLEDS() {
    if (trackEndA && timerEndA) {
        if (operator==(leds[3], CRGB::Black)) leds[3] = CRGB::Red;
        else leds[3] = CRGB::Black;
    } else if (!trackEndA) {
        leds[3] = CRGB::Black;
    }
    
    if (trackEndB && timerEndB) {
        if (operator==(leds[31], CRGB::Black)) leds[31] = CRGB::Red;
        else leds[31] = CRGB::Black;
    } else if (!trackEndB) {  //ensure switching off after NoteOff event
        leds[31] = CRGB::Black;
    }    
}

void selectorLEDS() {
    switch (bankA.getSelection()) {
        case 0:
            leds[0] = deckASelectorLEDS[0];
            leds[1] = deckASelectorLEDS[1];
            break;
        case 1:
            leds[0] = deckASelectorLEDS[2];
            leds[1] = deckASelectorLEDS[3];
            break;
        case 2:
            leds[0] = deckASelectorLEDS[4];
            leds[1] = deckASelectorLEDS[5];
            break;
        case 3:
            leds[0] = deckASelectorLEDS[6];
            leds[1] = deckASelectorLEDS[7];
            break;
        case 4:
            leds[0] = deckASelectorLEDS[8];
            leds[1] = deckASelectorLEDS[9];
            break;
        case 5:
            leds[0] = deckASelectorLEDS[10];
            leds[1] = deckASelectorLEDS[11];
            break;
        case 6:
            leds[0] = CRGB::DarkOrange;
            leds[1] = CRGB::DarkOrange;
            break;
        case 7:
            leds[0] = CRGB::Magenta;
            leds[1] = CRGB::Magenta;
            break;
    }
    
    switch (bankB.getSelection()) {
        case 0:
            leds[32] = deckBSelectorLEDS[0];
            leds[33] = deckBSelectorLEDS[1];
            break;
        case 1:
            leds[32] = deckBSelectorLEDS[2];
            leds[33] = deckBSelectorLEDS[3];
            break;
        case 2:
            leds[32] = deckBSelectorLEDS[4];
            leds[33] = deckBSelectorLEDS[5];
            break;
        case 3:
            leds[32] = deckBSelectorLEDS[6];
            leds[33] = deckBSelectorLEDS[7];
            break;
        case 4:
            leds[32] = deckBSelectorLEDS[8];
            leds[33] = deckBSelectorLEDS[9];
            break;
        case 5:
            leds[32] = deckBSelectorLEDS[10];
            leds[33] = deckBSelectorLEDS[11];
            break;
        case 6:
            leds[32] = CRGB::DarkOrange;
            leds[33] = CRGB::DarkOrange;
            break;
        case 7:
            leds[32] = CRGB::Magenta;
            leds[33] = CRGB::Magenta;
            break;
    }
    
}

//function that handles
void displays() {
    //Since drawing a single screen takes around 30 milliseconds we draw it again only after we know new information is available via newTimeAvailable, newTitleAvailable and newBPMAvailable functions
    //It's fine with 1 display, but the lag is definitely noticable in comparison to no display
    //With 2 displays the delay is unacceptable
 
    if (deckA.newTimeAvailable() || deckA.newTitleAvailable() || deckA.newBPMAvailable() || bankA.getSelection() != prevBankASelection) {
        channel(0);
        displayA.clearDisplay();
        displayA.setCursor(0,0);
        displayA.println("Deck A");

        displayA.setCursor(48, 0);
        displayA.print(deckA.getBPM());
        
        displayA.setCursor(96, 0);
        displayA.println(deckA.getShortTimeString());
    
        displayA.drawFastHLine(0, 9, 128, SSD1306_WHITE);
    
        displayA.setCursor(0, 12);
        displayA.println(deckA.getTitle());

        // Bank line separator, and calculation of centered position of name of the bank
        displayA.drawFastHLine(0, 54, 128, SSD1306_WHITE);       
        int16_t x1, y1;
        uint16_t w, h;        
        displayA.getTextBounds(bankNames[bankA.getSelection()], 0, 0, &x1, &y1, &w, &h);
        displayA.setCursor(64 - w/2, 56);
        displayA.println(bankNames[bankA.getSelection()]);
        prevBankASelection = bankA.getSelection();
        
        displayA.display();
    }
    
    if (deckB.newTimeAvailable() || deckB.newTitleAvailable() || deckB.newBPMAvailable() || bankB.getSelection() != prevBankBSelection) {
        channel(3);
        displayB.clearDisplay();
        displayB.setCursor(0,0);
        displayB.println("Deck B");

        displayB.setCursor(48, 0);
        displayB.print(deckB.getBPM());
    
        displayB.setCursor(96, 0);
        displayB.println(deckB.getShortTimeString());
    
        displayB.drawFastHLine(0, 9, 128, SSD1306_WHITE);
    
        displayB.setCursor(0, 12);
        displayB.println(deckB.getTitle());

        displayB.drawFastHLine(0, 54, 128, SSD1306_WHITE);       
        int16_t x1, y1;
        uint16_t w, h;        
        displayB.getTextBounds(bankNames[bankB.getSelection()], 0, 0, &x1, &y1, &w, &h);
        displayB.setCursor(64 - w/2, 56);
        displayB.println(bankNames[bankB.getSelection()]);
        prevBankBSelection = bankB.getSelection();
        
        displayB.display();
    }
}

void setup() {
  
    #ifdef USB_MIDI16_AUDIO_SERIAL
    AudioMemory(8);
    #endif

    Control_Surface.setMIDIInputCallbacks(channelMessageCallback, sysExMessageCallback, nullptr, nullptr);
    Control_Surface.begin();

    #if defined(ARDUINO_TEENSY40) || defined(ARDUINO_TEENSY41)
    FastLED.addLeds<1, WS2812, ledpin, GRB>(leds.data, numleds);
    #else
    FastLED.addLeds<NEOPIXEL, ledpin>(leds.data, numleds)
    #endif
    FastLED.setCorrection(TypicalPixelString);
    FastLED.setBrightness(32);

    //Neccesary for I2C multiplexer to work correctly
    Wire.begin();
    channel(0);
    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    if(!displayA.begin(SSD1306_SWITCHCAPVCC, 0x3C, false, true)) Serial.println(F("SSD1306 deck A allocation failed"));
    displayA.clearDisplay();
    displayA.drawBitmap(0, 0, logoTraktor, 128, 64, WHITE);
    displayA.display();
    
    channel(3);
    if(!displayB.begin(SSD1306_SWITCHCAPVCC, 0x3C, false, true)) Serial.println(F("SSD1306 deck B allocation failed"));
    displayB.clearDisplay();
    displayB.drawBitmap(0, 0, logoTraktor, 128, 64, WHITE);
    displayB.display();

    displayA.setTextColor(SSD1306_WHITE);    
    displayB.setTextColor(SSD1306_WHITE);

    second.beginNextPeriod();
    activity.begin();
}

void loop() {
    Control_Surface.loop();
    trackEndLEDS();
    selectorLEDS();
    displays();
    //Print debug data every second to make it more readable in a serial monitor
    #ifdef CS1_DEBUG
    if (second) Serial << deckA.debug() << deckB.debug();
    #endif

    if (activity) {
        if (activityNoteToggle) midi.sendControlChange({MIDI_Notes::G(9), CHANNEL_3, Cable(3)}, 1);
        else midi.sendControlChange({MIDI_Notes::G(9), CHANNEL_3, Cable(3)}, 0);

        activityNoteToggle = !activityNoteToggle;

        // Check for change
        if (activityTracker.getDirty()) active = true;
        else active = false;

        activityTracker.clearDirty();
    }

    if (!active) {
        FastLED.clear();
        channel(0);
        displayA.clearDisplay();
        channel(3);
        displayB.clearDisplay();
        deckA.clear();
        deckB.clear();
        trackEndA = false;
        trackEndB = false;
    }
        
    FastLED.show();
}
