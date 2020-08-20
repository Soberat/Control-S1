#include <FastLED.h>
#include <Encoder.h>
#include <Control_Surface.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include "Mapping.cpp"
#include "TrackDataHandler.cpp"

// The data pin with the strip connected.
constexpr uint8_t ledpin = 0;
// Total number of leds connected to FastLED
constexpr uint8_t numleds = 44;
// How many CCs are sent to control LEDs
constexpr uint8_t ledCallbacks = 20;


/*
 * Deck inputs:
 * 2x15 muxed down to 2x5 - Gain, High, Mid, Low, Filter, Cue Enable, Volume, selector 1234, play, cue, load, shift
 * 6 - crossfader, volume master, browser, master volume, monitor volume, cruise mode
 * 1 - WS2812
 * 5x2 - encoders
 * 2 - I2C
 */

//TODO check if the library works fine using same pins
//todo debug why doesnt connect after reprogramming
//todo turn off displays and lights if no input detected for some time or some other condition

using namespace MIDI_Notes;

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
 * Traktor outputs midi for Numark on channels 1 and 2. To avoid conflict, don't use CC range [32-77] or simply switch Kontrol output to any channel higher than 2.
 * I'm outputting Numark on different virtual cables so I don't have any conflicts, although it might be a bit overdone.
 */
Adafruit_SSD1306 displayA(128, 64, &Wire, 4);
Adafruit_SSD1306 displayB(128, 64, &Wire, 4);

USBMIDI_Interface midi;
CD74HC4067 mux = {A6, {10, 9, 8, 7}};
//CD74HC4067 mux2 = {A13, {12, 11, 10, 9}};

CRGB colorOff = CRGB(0, 0, 0);

CRGB vuColors[8] = {CRGB::Green, CRGB::Green, CRGB::Green, CRGB::Green, CRGB::Green, CRGB::Yellow, CRGB::Yellow, CRGB::Red};
//CRGB vuColors[8] = {CRGB::DarkBlue, CRGB::DarkBlue, CRGB::DarkBlue, CRGB::DarkBlue, CRGB::DarkBlue, CRGB::DarkOrange, CRGB::DarkOrange, CRGB::DarkOrange};


//Array storing LED information about 8 hotcues, sync/master status and loop status
CRGB deckASelectorLEDS[12] = {colorOff, colorOff, colorOff, colorOff, colorOff, colorOff, colorOff, colorOff, colorOff, colorOff, colorOff, colorOff};
//CRGB deckBSelectorLEDS[12] = {colorOff, colorOff, colorOff, colorOff, colorOff, colorOff, colorOff, colorOff, colorOff, colorOff, colorOff, colorOff};

CRGB cueType(int num) {
    if (num == 0)  return colorOff;           //No cue type
    if (num == 1)  return CRGB::DodgerBlue;   //Cue
    if (num == 2)  return CRGB::DarkOrange;   //Fade In
    if (num == 3)  return CRGB::DarkOrange;   //Fade Out
    if (num == 4)  return CRGB::Gold;         //Load
    if (num == 5)  return CRGB::DarkGray;     //Grid
    if (num == 6)  return CRGB::Green;        //Loop

    return CRGB::Azure;
}

TrackDataHandler deckA(0x02, 0xB0);
TrackDataHandler deckB(0x22, 0xB1);

//Track end warnings don't have to be synchronized between decks, so we need 2 separate timers
//This value is arbitarily chosen to match Traktor's flashing interval
Timer<millis> timerEndA = 790;
Timer<millis> timerEndB = 790;

Timer<millis> timerClipA = 2000;
Timer<millis> timerClipB = 2000;
Timer<millis> timerClipMaster = 2000;

Timer<millis> second = 1000;

//Variables containing information if Track End Warning is active for a deck
bool trackEndA = false;
bool trackEndB = false;

bool deckAClipped = false;
bool deckbClipped = false;
bool masterClipped = false;

/*
Bank<7> bankA(4); // 4 cue selectors, looper, beatjump, sync 
//Bank<7> bankB(4); // 1 encoder, 2 pushbuttons, 1 encoder button, total range of 28 CCs x2

EncoderSelector<7> selectorA = {bankA, {5, 6}, 4, Wrap::Clamp};
//EncoderSelector<7> selectorB = {bankB, {pin1, pin2}, 4, Wrap::Clamp};

//CC range [0-55] is reserved for the function selectors and buttons
Bankable::CCButton button1A = {
    {bankA, BankType::CHANGE_ADDRESS},
     mux.pin(8),
    {0, CHANNEL_1}};
    
Bankable::CCButton button2A = {
    {bankA, BankType::CHANGE_ADDRESS},
     mux.pin(7),
    {1, CHANNEL_1}};

Bankable::CCButton buttonEncA = {
    {bankA, BankType::CHANGE_ADDRESS},
     4,
    {2, CHANNEL_1}};

Bankable::CCRotaryEncoder encoderA= {
    {bankA, BankType::CHANGE_ADDRESS},
    {7,8},
    {3, CHANNEL_1}};

Bankable::CCButton button1B = {
    {bankB, BankType::CHANGE_ADDRESS},
     pin,
    {28, CHANNEL_1}};
    
Bankable::CCButton button2B = {
    {bankB, BankType::CHANGE_ADDRESS},
     pin,
    {29, CHANNEL_1}};

Bankable::CCButton buttonEncB = {
    {bankB, BankType::CHANGE_ADDRESS},
     pin,
    {30, CHANNEL_1}};

Bankable::CCRotaryEncoder = encoderB {
    {bankB, BankType::CHANGE_ADDRESS},
    {pin1, pin2},
    {31, CHANNEL_1}};    
*/

// Custom callback to handle incoming note events and control the LEDs
class NoteCCFastLEDCallbackRGB : public SimpleNoteCCValueCallback {
    public:
        NoteCCFastLEDCallbackRGB(CRGB *ledcolors) : ledcolors(ledcolors) {}
            
        //TODO clipped + decay
            
        // Called once upon initialization.
        void begin(const INoteCCValue &input) override { updateAll(input); }
    
        /*
         * This function has the job of controlling WS2812 Neopixels
         * In this case we are using multiple Neopixels connected in series to reduce pin usage on the uC
         * They are connected as :
         * State A -> TrackEnd A -> Volume A-> Phase -> Volume B-> Volume Master -> State B-> TrackEnd B
         *  0    ->     1    ->       2       ->    3    ->     4      ->    5    ->      6     ->    7
         * Declaration of CustomNoteValueLED below defines a range of notes starting at a given note to be received and handled by the class.
         * Velocity of the Nth note is stored as index-1 in the input variable
         * Notes need to be one after another for this to work. 
         * 
         * It is crucial that the leds are not cleared with FastLED.clear(), as this introduces a lot of flickering
         * 
         * On Arduino Uno some notes may stay on even after a note signals to turn it off. No idea what's causing this, but the midi note is sent correctly.
         */
        void update(const INoteCCValue &input, uint8_t index) override {
            int value = input.getValue(index);
              switch (index) {
                  case 0:
                      if (value == 1) ledcolors[0] = CRGB::Green;
                      else ledcolors[0] = colorOff;
                      break;
                  case 1:
                      //Case 1 and 7 are responsible for alternating TrackEnd LEDs. Since NoteOn for the warning is received only once, this just tells an external function (trackEndLEDS()) to take care of it in the program loop
                      if (value == 1) {
                        trackEndA = true;
                        timerEndA.begin();
                      }
                      else trackEndA = false;
                      break;
                  case 2 :  //this strip is vertically inverted relative to others, so the code is a bit different
                      for (int i = 7; i >= 0; i--) if (value > 16*i) {
                          ledcolors[9-i] = vuColors[i];
                      } else {
                          ledcolors[9-i] = colorOff;
                      }
                      break;
                  case 3 :
                      //This is responsible for displaying the phase the same way Traktor does, as in center is no phase shift.
                      //Since midi values are 0-127 the code is quite ugly, but it works ;)                  
                      for (int i = 0; i <= 7; i++) ledcolors[i+10] = colorOff;
                      if (value == 63 || value == 0) break; //default value is 63, which means so phase shift. 0 is the default when the device starts without MIDI input, so we ignore it as well
                      else if (value >= 0  && value < 15) for (int i = 0; i <= 3; i++) ledcolors[i+10] = CRGB::Orange;
                      else if (value >= 15 && value < 31) for (int i = 1; i <= 3; i++) ledcolors[i+10] = CRGB::Orange;
                      else if (value >= 31 && value < 47) for (int i = 2; i <= 3; i++) ledcolors[i+10] = CRGB::Orange;
                      else if (value >= 47 && value < 63) ledcolors[13] = CRGB::Orange;
                      else if (value > 63 && value <= 79) ledcolors[14] = CRGB::Orange;
                      else if (value > 79 && value <= 95) for (int i = 4; i <= 5; i++) ledcolors[i+10] = CRGB::Orange;
                      else if (value > 95 && value <= 111) for (int i = 4; i <= 6; i++) ledcolors[i+10] = CRGB::Orange;
                      else if (value > 111) for (int i = 4; i <= 7; i++) ledcolors[i+10] = CRGB::Orange;
                      break;
                  case 4 :
                      for (int i = 0; i <= 7; i++) if (value > 16*i) {
                          ledcolors[18+i] = vuColors[i];
                      } else {
                          ledcolors[18+i] = colorOff;
                      }
                      break;
                  case 5 :
                      for (int i = 0; i <= 7; i++) if (value > 16*i) {
                          ledcolors[26+i] = vuColors[i];
                      } else {
                          ledcolors[26+i] = colorOff;
                      }
                      break;
                  case 6:
                      if (value == 1) ledcolors[34] = CRGB::Green;
                      else ledcolors[34] = colorOff;
                      break;
                  case 7:
                      if (value == 1) {
                        trackEndB = true;
                        timerEndB.begin();
                      }
                      else trackEndB = false;
                      break;

                  case 8: //hotcue 1
                      deckASelectorLEDS[0] = cueType(value);
                      break;
                  case 9: //hotcue 2
                      deckASelectorLEDS[1] = cueType(value);
                      break;
                  case 10: //hotcue 3
                      deckASelectorLEDS[2] = cueType(value);
                      break;
                  case 11: //hotcue 4
                      deckASelectorLEDS[3] = cueType(value);
                      break;
                  case 12: //hotcue 5
                      deckASelectorLEDS[4] = cueType(value);
                      break;
                  case 13: //hotcue 6
                      deckASelectorLEDS[5] = cueType(value);
                      break;
                  case 14: //hotcue 7
                      deckASelectorLEDS[6] = cueType(value);
                      break;
                  case 15: //hotcue 8
                      deckASelectorLEDS[7] = cueType(value);
                      break;
                  case 16: //sync on
                      if (value > 0) deckASelectorLEDS[8] = CRGB::Blue;
                      else deckASelectorLEDS[8] = colorOff;
                      break;
                  case 17: //is master
                      if (value > 0) deckASelectorLEDS[9] = CRGB::Blue;
                      else deckASelectorLEDS[9] = colorOff;
                      break;
                  case 18: //loop in
                      if (value > 0) deckASelectorLEDS[10] = CRGB::Green;
                      else deckASelectorLEDS[10] = colorOff;
                      break;
                  case 19: //loop out
                      if (value > 0) deckASelectorLEDS[11] = CRGB::Green;
                      else deckASelectorLEDS[11] = colorOff;
                      break;
              }
        }        
    private:
        // Pointer to array of FastLED color values for the LEDs
        CRGB *ledcolors;
};
 
// Create a type alias for the MIDI Note Input Element that uses
// the custom callback defined above.
template <uint8_t RangeLen>
using CustomNoteValueLED = GenericNoteCCRange<MIDIInputElementCC, RangeLen, NoteCCFastLEDCallbackRGB>;
// Define the array of leds.
Array<CRGB, numleds> leds = {};

CustomNoteValueLED<ledCallbacks> midiled = {ledpin, leds.data};

CCPotentiometer potVolumeA = {mux.pin(13), {56, CHANNEL_1}}; 
CCPotentiometer potGainA   = {A2, {57, CHANNEL_1}};
//CCPotentiometer potLowA    = {mux.pin(13), {58, CHANNEL_1}}; 
//CCPotentiometer potMidA    = {mux.pin(13), {59, CHANNEL_1}}; 
//CCPotentiometer potHighA   = {mux.pin(13), {60, CHANNEL_1}}; 
//CCPotentiometer potFilterA = {mux.pin(13), {61, CHANNEL_1}}; 

CCButton buttonPlayA = {mux.pin(15),{62, CHANNEL_1}};
CCButton buttonCueA = {mux.pin(14),{63, CHANNEL_1}};
CCButton buttonLoadA = {mux.pin(11),{64, CHANNEL_1}};
//CCButton buttonMonitorA = {mux.pin(27), {65, CHANNEL_1}}; 
CCButton buttonModifier1A = {1, {66, CHANNEL_1}}; //shift left side

CCPotentiometer potVolumeB = {mux.pin(2), {67, CHANNEL_1}};
CCPotentiometer potGainB   = {A0, {68, CHANNEL_1}};
//CCPotentiometer potLowB    = {mux.pin(13), {69, CHANNEL_1}}; 
//CCPotentiometer potMidB    = {mux.pin(13), {70, CHANNEL_1}}; 
//CCPotentiometer potHighB   = {mux.pin(13), {71, CHANNEL_1}}; 
//CCPotentiometer potFilterB = {mux.pin(13), {72, CHANNEL_1}}; 

CCButton buttonPlayB = {mux.pin(1),{73, CHANNEL_1}};
CCButton buttonCueB  = {mux.pin(0),{74, CHANNEL_1}};
CCButton buttonLoadB = {mux.pin(4),{75, CHANNEL_1}};
//CCButton buttonMonitorB = {mux.pin(13), {76, CHANNEL_1}}; 
CCButton buttonModifier1B = {11, {66, CHANNEL_1}}; //shift right side

CCPotentiometer potXfader  = {mux.pin(3), {78, CHANNEL_1}};
CCPotentiometer potVolumeMaster  = {A1, {79, CHANNEL_1}};
//CCPotentiometer potVolumeMonitor ={mux.pin(13), {80, CHANNEL_1}}; 

CCButton buttonModifier5 = {mux.pin(12), {81, CHANNEL_1}}; //browser encoder pushbutton, used only for browser navigation.
//CCButton buttonCruise = {mux.pin(11), {82, CHANNEL_1}};

CCRotaryEncoder encoderBrowser = {{5, 6}, {83, CHANNEL_1}};


//Function that changes the channel of TCA9548A I2C multiplexer
void channel(uint8_t bus) {
    Wire.beginTransmission(0x70);
    Wire.write(1 << bus);
    Wire.endTransmission();
}


bool sysExMessageCallback(SysExMessage se) {
    //making sure the data is coming from Traktor and that length corresponds to title data message length (6 ascii + 16 id)
    if (se.data[0] == 0xF0 && se.data[se.length-1] == 0xF7 && se.length == 22) {
        if (se.CN == 1) return deckA.receive(se);
        else if (se.CN == 2) return deckB.receive(se);
    } else {        
        return false; //indicate that the message should be handled by the library.
    }
    return false;
}

bool channelMessageCallback(ChannelMessage cm) {
    if (cm.data1 >= 32 && cm.data1 <= 77) {
        if (cm.CN == 1) {
            deckA.receive(cm);
            return true;
        } 
        else if (cm.CN == 2) {
            deckB.receive(cm);
            return true;
        }
    }
    return false;
}


void trackEndLEDS() {
    if (trackEndB && timerEndB) {
        if (leds[35] == colorOff) leds[35] = CRGB::Red;
        else leds[35] = colorOff;
    } else if (!trackEndB) {  //ensure switching off after NoteOff event
        leds[35] = colorOff;
    }

    if (trackEndA && timerEndA) {
        if (leds[1] == colorOff) leds[1] = CRGB::Red;
        else leds[1] = colorOff;
    } else if (!trackEndA) {
        leds[1] = colorOff;
    }
}
/*
void selectorLEDS() {
    switch (bankA.getSelection()) {
        case 0:
            leds[36] = deckASelectorLEDS[0];
            leds[37] = deckASelectorLEDS[1];
            break;
        case 1:
            leds[36] = deckASelectorLEDS[2];
            leds[37] = deckASelectorLEDS[3];
            break;
        case 2:
            leds[36] = deckASelectorLEDS[4];
            leds[37] = deckASelectorLEDS[5];
            break;
        case 3:
            leds[36] = deckASelectorLEDS[6];
            leds[37] = deckASelectorLEDS[7];
            break;
        case 4:
            leds[36] = deckASelectorLEDS[8];
            leds[37] = deckASelectorLEDS[9];
            break;
        case 5:
            leds[36] = CRGB::Lime;
            leds[37] = CRGB::Lime;
            break;
        case 6:
            leds[36] = CRGB::Magenta;
            leds[37] = CRGB::Magenta;
            break;
    }
    
    switch (bankB.getSelection()) {
        case 0:
            leds[X] = deckASelectorLEDS[0];
            leds[X+1] = deckASelectorLEDS[1];
            break;
        case 1:
            leds[X] = deckASelectorLEDS[2];
            leds[X+1] = deckASelectorLEDS[3];
            break;
        case 2:
            leds[X] = deckASelectorLEDS[4];
            leds[X+1] = deckASelectorLEDS[5];
            break;
        case 3:
            leds[X] = deckBCues[6];
            leds[X+1] = deckBCues[7];
            break;
        case 4:
            leds[X] = deckBSync[0];
            leds[X+1] = deckBSync[1];
            break;
        case 5:
            leds[X] = CRGB::Lime;
            leds[X+1] = CRGB::Lime;
            break;
        case 6:
            leds[X] = CRGB::Magenta;
            leds[X+1] = CRGB::Magenta;
            break;
    }
    
}
*/
void setup() {
    potVolumeA.map(Mapping::volumeA);
    potVolumeB.map(Mapping::volumeB);
    potVolumeMaster.map(Mapping::volumeMaster);
    potXfader.map(Mapping::crossfader);
    potGainA.map(Mapping::gainA);
    potGainB.map(Mapping::gainB);
    
    FastLED.addLeds<NEOPIXEL, ledpin>(leds.data, numleds);
    FastLED.setCorrection(TypicalPixelString);
    FastLED.setBrightness(32);
    Control_Surface.setMIDIInputCallbacks(channelMessageCallback, sysExMessageCallback, nullptr);
    Control_Surface.begin();
    second.begin();

    //Neccesary for I2C multiplexer to work correctly
    Wire.begin();
        
    channel(0);
    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    if(!displayA.begin(SSD1306_SWITCHCAPVCC, 0x3C)) Serial.println(F("SSD1306 deck A allocation failed"));
    // Show initial display buffer contents on the screen --
    displayA.clearDisplay();
    displayA.drawBitmap(0, 0, logoTraktor, 128, 64, WHITE);
    displayA.display();

    channel(7);
    if(!displayB.begin(SSD1306_SWITCHCAPVCC, 0x3C)) Serial.println(F("SSD1306 deck B allocation failed"));
    displayB.clearDisplay();
    displayB.drawBitmap(0, 0, logoTraktor, 128, 64, WHITE);
    displayB.display();
    
    delay(2000); // Pause for 2 seconds
    displayA.setTextColor(SSD1306_WHITE);    
    displayB.setTextColor(SSD1306_WHITE);
}

//function that handles
void displays() {
    //Since drawing a single screen takes around 30 milliseconds we draw it again only after we know new information is available via newTimeAvailable, newTitleAvailable and newBPMAvailable functions
    //It's fine with 1 display, but the lag is definitely noticable in comparison to no display
    //With 2 displays the delay is unacceptable
    if (deckA.newTimeAvailable() || deckA.newTitleAvailable() || deckA.newBPMAvailable()) {
        channel(0);
        displayA.clearDisplay();
        displayA.setCursor(0,0);
        displayA.println("Deck A");

        displayA.setCursor(48, 0);
        displayA.print(deckA.getBPM());
        
        displayA.setCursor(96, 0);
        displayA.println(deckA.getShortTimeString());
    
        displayA.drawLine(0, 9, 127, 9, SSD1306_WHITE);
    
        displayA.setCursor(0, 12);
        displayA.println(deckA.getTitle());
        displayA.display();
    }

    if (deckB.newTimeAvailable() || deckB.newTitleAvailable() || deckB.newBPMAvailable()) {
        channel(7);
        displayB.clearDisplay();
        displayB.setCursor(0,0);
        displayB.println("Deck B");

        displayB.setCursor(48, 0);
        displayB.print(deckB.getBPM());
    
        displayB.setCursor(96, 0);
        displayB.println(deckB.getShortTimeString());
    
        displayB.drawLine(0, 9, 127, 9, SSD1306_WHITE);
    
        displayB.setCursor(0, 12);
        displayB.println(deckB.getTitle());
        displayB.display();
    }
}

void loop() {
    Control_Surface.loop();
    trackEndLEDS();
    //selectorLEDS();
    displays();    
    
    if (second) {
        Serial << dec << "Deck A: Title: " << deckA.getTitle().replace("\n", " - ") << "  " << "BPM: " << deckA.getBPM() << "  " << "Time: " << (deckA.getTime().minutes < 10 ? "0" : "") << deckA.getTime().minutes << ":" << (deckA.getTime().seconds < 10 ? "0" : "") << deckA.getTime().seconds << "  " << "Tempo d: " << deckA.getTempo() << endl;
        Serial << dec << "Deck B: Title: " << deckB.getTitle().replace("\n", " - ") << "  " << "BPM: " << deckB.getBPM() << "  " << "Time: " << (deckB.getTime().minutes < 10 ? "0" : "") << deckB.getTime().minutes << ":" << (deckB.getTime().seconds < 10 ? "0" : "") << deckB.getTime().seconds << "  " << "Tempo d: " << deckB.getTempo() << endl;
    }
    FastLED.show();
}
