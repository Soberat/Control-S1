#include <FastLED.h>
#include <Encoder.h>
#include <Control_Surface.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include "Mapping.cpp"
#include "TrackDataHandler.cpp"

using namespace MIDI_Notes;

//TODO try and add displays
//TODO add missing pots and buttons


/* !!!
 * Traktor outputs midi for Numark on channels 1 and 2. To avoid conflict, don't use CC range [32-77] or simply switch Kontrol output to any channel higher than 2.
 */
Adafruit_SSD1306 display(128, 64, &Wire, 4);

USBMIDI_Interface midi;
CD74HC4067 mux = {A6, {12, 11, 10, 9}};

CRGB colorRed = CRGB(255, 0, 0);
CRGB colorGreen = CRGB(0, 255, 0);
CRGB colorYellow = CRGB(255, 255, 0);
CRGB colorOrange = CRGB(255, 128, 0);
CRGB colorBlue = CRGB(0, 96, 255);
CRGB colorOff = CRGB(0, 0, 0);

CRGB vuColors[8] = {colorGreen, colorGreen, colorGreen, colorGreen, colorGreen, colorYellow, colorYellow, colorRed};
//CRGB vuColors[8] = {colorBlue, colorBlue, colorBlue, colorBlue, colorBlue, colorOrange, colorOrange, colorOrange};

TrackDataHandler deckA(0x02, 0xB0);
TrackDataHandler deckB(0x22, 0xB1);

//Track end warnings don't have to be synchronized between decks, so we need 2 separate timers
//This value is arbitarily chosen to match Traktor's flashing interval
Timer<millis> timerEndA = 790;
Timer<millis> timerEndB = 790;

Timer<millis> timerPotMaster = 3000;
Timer<millis> timerPotMonitor = 3000;

Timer<millis> second = 1000;

//Variables containing information if Track End Warning is active for a deck
bool trackEndA = false;
bool trackEndB = false;

bool potRingMasterActive = false;
bool potRingMonitorActive = false;

// Custom callback to handle incoming note events and control the LEDs
class NoteCCFastLEDCallbackRGB : public SimpleNoteCCValueCallback {
  public:
    NoteCCFastLEDCallbackRGB(CRGB *ledcolors)
        : ledcolors(ledcolors) {}

    //TODO reduce steps in Traktor?
    //TODO clipped decay
    //TODO debug lights that dont turn off (trackend, state, volume)

    
    // Called once upon initialization.
    void begin(const INoteCCValue &input) override { updateAll(input); }
 
    // Called each time a MIDI message is received and an LED has to be updated.
    // @param   input
    //          The NoteCCRange or NoteCCValue object this callback belongs to.
    //          This is the object that actually receives and stores the MIDI
    //          values.
    // @param   index
    //          The index of the value that changed. (zero-based)


    /*
     * This function has the job of controlling WS2812 Neopixels
     * In this case we are using multiple Neopixels connected in series to reduce pin usage on the uC
     * They are connected as :
     * Phase -> Volume B -> Volume Master -> State B -> TrackEnd B -> State A -> TrackEnd A -> Volume A
     *  0    ->     1    ->       2       ->    3    ->     4      ->    5    ->      6     ->    7
     * Declaration of CustomNoteValueLED below defines a range of notes starting at a given note to be polled by the class.
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
            case 3 :  //this code is correct, however the values given by traktor are not
                
                for (int i = 0; i <= 7; i++) ledcolors[i+10] = colorOff;

                if (value == 63) break; //default value is 63, which means so phase shift

                else if (value >= 0  && value < 15) for (int i = 0; i <= 3; i++) ledcolors[i+10] = colorOrange;

                else if (value >= 15 && value < 31) for (int i = 1; i <= 3; i++) ledcolors[i+10] = colorOrange;

                else if (value >= 31 && value < 47) for (int i = 2; i <= 3; i++) ledcolors[i+10] = colorOrange;

                else if (value >= 47 && value < 63) ledcolors[13] = colorOrange;

                else if (value >  63 && value <= 79) ledcolors[14] = colorOrange;

                else if (value > 79 && value <= 95) for (int i = 4; i <= 5; i++) ledcolors[i+10] = colorOrange;

                else if (value > 95 && value <= 111) for (int i = 4; i <= 6; i++) ledcolors[i+10] = colorOrange;

                else if (value > 111) for (int i = 4; i <= 7; i++) ledcolors[i+10] = colorOrange;
               
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
                if (value == 1) ledcolors[34] = colorGreen;
                else ledcolors[34] = colorOff;
                break;
            case 7:
                //Case 4 and 6 are responsible for alternating TrackEnd LEDs. Since NoteOn for the warning is received only once, this just tells an external function (trackEndLEDS()) to take care of it in the program loop
                if (value == 1) {
                  trackEndB = true;
                  timerEndB.begin();
                }
                else trackEndB = false;
                break;
            case 0:
                if (value == 1) ledcolors[0] = colorGreen;
                else ledcolors[0] = colorOff;
                break;
            case 1:
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
            
            case 8:   //code for a 5-led potentiometer ring. Indexes are for a 16-led ring.
                if (value >= 16) {
                    ledcolors[38] = colorBlue;
                    timerPotMaster.beginNextPeriod();
                    potRingMasterActive = true;
                } else {
                    ledcolors[38] = colorOff;
                }

                if (value >= 36)   {
                    ledcolors[36] = colorBlue;
                } else {
                    ledcolors[36] = colorOff;
                }

                if (value >= 63)   {
                    ledcolors[49] = colorBlue;
                } else {
                    ledcolors[49] = colorOff;
                }

                if (value >= 90)   {
                    ledcolors[46] = colorBlue;
                } else {
                    ledcolors[46] = colorOff;
                }

                if (value >= 110)   {
                    ledcolors[43] = colorBlue;
                } else {
                    ledcolors[43] = colorOff;
                }
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
Array<CRGB, 52> leds = {};
// The data pin with the strip connected.
constexpr uint8_t ledpin = 6;
 
CustomNoteValueLED<9> midiled = {0, leds.data};


CCPotentiometer potVolumeA = {mux.pin(13), {7, CHANNEL_1}}; 
CCPotentiometer potGainA   = {A9, {10, CHANNEL_1}};
//CCPotentiometer potLowA    = {mux.pin(13), {11, CHANNEL_1}}; 
//CCPotentiometer potMidA    = {mux.pin(13), {12, CHANNEL_1}}; 
//CCPotentiometer potHighA   = {mux.pin(13), {13, CHANNEL_1}}; 
//CCPotentiometer potFilterA = {mux.pin(13), {14, CHANNEL_1}}; 


CCPotentiometer potVolumeB = {mux.pin(2), {1, CHANNEL_1}};
CCPotentiometer potGainB   = {A1, {16, CHANNEL_1}};
//CCPotentiometer potLowB    = {mux.pin(13), {17, CHANNEL_1}}; 
//CCPotentiometer potMidB    = {mux.pin(13), {18, CHANNEL_1}}; 
//CCPotentiometer potHighB   = {mux.pin(13), {19, CHANNEL_1}}; 
//CCPotentiometer potFilterB = {mux.pin(13), {20, CHANNEL_1}}; 

CCPotentiometer potXfader  = {mux.pin(3), {0, CHANNEL_1}};
CCPotentiometer potVolumeMaster  = {A0, {8, CHANNEL_1}};
//CCPotentiometer potVolumeMonitor ={mux.pin(13), {23, CHANNEL_1}}; 

CCButton buttonPlayA = {mux.pin(15),{3, CHANNEL_1}};
CCButton buttonCueA = {mux.pin(14),{4, CHANNEL_1}};
CCButton buttonLoadA = {mux.pin(11),{26, CHANNEL_1}};
//CCButton buttonMonitorA = {mux.pin(27), {20, CHANNEL_1}}; 

CCButton buttonPlayB = {mux.pin(1),{5, CHANNEL_1}};
CCButton buttonCueB  = {mux.pin(0),{6, CHANNEL_1}};
CCButton buttonLoadB = {mux.pin(4),{30, CHANNEL_1}};
//CCButton buttonMonitorB = {mux.pin(13), {31, CHANNEL_1}}; 


//CCButton buttonModifier1 = {mux.pin(11), {78, CHANNEL_1}};
CCButton buttonModifier5 = {mux.pin(12), {79, CHANNEL_1}};
//CCButton buttonAutoplay = {mux.pin(11), {80, CHANNEL_1}};

CCRotaryEncoder encoderBrowser = {{2, 3}, {81, CHANNEL_1}};

void setup() {

    
    potVolumeA.map(Mapping::volumeA);
    potVolumeB.map(Mapping::volumeB);
    potVolumeMaster.map(Mapping::volumeMaster);
    potXfader.map(Mapping::crossfader);
    potGainA.map(Mapping::gainA);
    potGainB.map(Mapping::gainB);
    
    FastLED.addLeds<NEOPIXEL, 0>(leds.data, 52);
    FastLED.setCorrection(TypicalPixelString);
    FastLED.setBrightness(32);
    Control_Surface.setMIDIInputCallbacks(channelMessageCallback, sysExMessageCallback, nullptr);
    Control_Surface.begin();
    second.begin();

    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
      Serial.println(F("SSD1306 allocation failed"));
      for(;;); // Don't proceed, loop forever
    }
    
    // Show initial display buffer contents on the screen --
    // the library initializes this with an Adafruit splash screen.
    display.display();
    //display.setFont(&Roboto_Mono_10);
    delay(100); // Pause for 2 seconds
    display.setTextColor(SSD1306_WHITE);    
    // Clear the buffer
    display.clearDisplay();
}

void loop() {
    Control_Surface.loop();
    trackEndLEDS();
    potRingLEDS();
    
    display.clearDisplay();
    display.setCursor(0,0);
    display.println("Deck A");

    display.setCursor(72, 0);
    display.println(deckA.getTimeString());

    display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

    display.setCursor(0, 12);
    display.println(deckA.getTitle());
    display.display();

    if (second) {
        Serial << dec << "Deck A: Title: " << deckA.getTitle() << "  " << "BPM: " << deckA.getBPM() << "  " << "Time: " << (deckA.getTime().minutes < 10 ? "0" : "") << deckA.getTime().minutes << ":" << (deckA.getTime().seconds < 10 ? "0" : "") << deckA.getTime().seconds << "  " << "Tempo d: " << deckA.getTempo() << endl;
        Serial << dec << "Deck B: Title: " << deckB.getTitle() << "  " << "BPM: " << deckB.getBPM() << "  " << "Time: " << (deckB.getTime().minutes < 10 ? "0" : "") << deckB.getTime().minutes << ":" << (deckB.getTime().seconds < 10 ? "0" : "") << deckB.getTime().seconds << "  " << "Tempo d: " << deckB.getTempo() << endl;
    }
    FastLED.show();
}

void potRingLEDS() {
    if (timerPotMaster && potRingMasterActive) {
        for (int i = 0; i <= 15; i++) leds[36 + i] = colorOff;
        potRingMasterActive = false;
    }
    /*
    if (timerPotMonitor && potRingMonitorActive) {
        for (int i = 0; i <= 15; i++) leds[36 + i] = colorOff;
        potRingMonitorActive = false;
    }
    */
}
void trackEndLEDS() {
    if (trackEndB && timerEndB) {
        if (leds[35] == colorOff) leds[35] = colorRed;
        else leds[35] = colorOff;
    } else if (!trackEndB) {  //ensure switching off after NoteOff event
        leds[35] = colorOff;
    }

    if (trackEndA && timerEndA) {
        if (leds[1] == colorOff) leds[1] = colorRed;
        else leds[1] = colorOff;
    } else if (!trackEndA) {
        leds[1] = colorOff;
    }
}

bool sysExMessageCallback(SysExMessage se) {
    //making sure the data is coming from Traktor and that length corresponds to title data message length (6 ascii + 16 id)
    if (se.data[0] == 0xF0 && se.data[se.length-1] == 0xF7 && se.length == 22) {
        if (se.CN == 2) deckB.receive(se);
        else if (se.CN == 1) deckA.receive(se);
        return true;
    } else {        
        return false; //indicate that the message should be handled by the library.
    }
}

bool channelMessageCallback(ChannelMessage cm) {
    if (cm.data1 >= 32 && cm.data1 <= 77) {
        if (cm.CN == 1) {
            deckA.receive(cm);
            return true;
        } // && (cm.header == 0xB1 || cm.header == 0x91)
        else if (cm.CN == 2) {
            deckB.receive(cm);
            return true;
        }
    }
    //Serial << hex << cm.header << ' ' << cm.data1 << ' ' << cm.data2 << endl;
    return false;
}
