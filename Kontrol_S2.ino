#include <FastLED.h>
#include <Encoder.h>
#include <Control_Surface.h>
#include "Mapping.cpp"
#include "TrackDataHandler.cpp"

using namespace MIDI_Notes;

//TODO try and add displays
//TODO add missing pots and buttons
//TODO add color schemes?
//YODO 5 led pot rings

/* !!!
 * Traktor outputs midi for Numark on channels 1 and 2. To avoid conflict, don't use CC range [32-77] or simply switch Kontrol output to any channel higher than 2.
 */
 
USBMIDI_Interface midi;
CD74HC4067 mux = {A5, {2, 3, 4, 5}};

CRGB colorRed = CRGB(255, 0, 0);
CRGB colorGreen = CRGB(0, 255, 0);
CRGB colorYellow = CRGB(255, 255, 0);
CRGB colorOrange = CRGB(255, 128, 0);
CRGB colorOff = CRGB(0, 0, 0);

CRGB vuColors[8] = {colorGreen, colorGreen, colorGreen, colorGreen, colorGreen, colorYellow, colorYellow, colorRed};

TrackDataHandler deckA(0x02, 0xB0);
TrackDataHandler deckB(0x22, 0xB1);

//Track end warnings don't have to be synchronized between decks, so we need 2 separate timers
//This value is arbitarily chosen to match Traktor's flashing interval
Timer<millis> timerA = 790;
Timer<millis> timerB = 790;

Timer<millis> timerPot = 3000;

Timer<millis> second = 1000;
//Variables containing information if Track End Warning is active for a deck
bool trackEndA = false;
bool trackEndB = false;

bool potRingActive = false;

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
            case 0 :  //this code is correct, however the values given by traktor are not
                
                for (int i = 0; i <= 7; i++) ledcolors[i] = colorOff;

                if (value == 63) break; //default value is 63, which means so phase shift

                else if (value > 0  && value < 16) for (int i = 0; i <= 3; i++) ledcolors[i] = colorOrange;

                else if (value >= 16 && value < 32) for (int i = 1; i <= 3; i++) ledcolors[i] = colorOrange;

                else if (value >= 32 && value < 48) for (int i = 2; i <= 3; i++) ledcolors[i] = colorOrange;

                else if (value >= 48 && value < 63) ledcolors[3] = colorOrange;

                else if (value >  63 && value < 80) ledcolors[4] = colorOrange;

                else if (value >= 80 && value < 96) for (int i = 4; i <= 5; i++) ledcolors[i] = colorOrange;

                else if (value >= 96 && value < 114) for (int i = 4; i <= 6; i++) ledcolors[i] = colorOrange;

                else if (value >= 114) for (int i = 4; i <= 7; i++) ledcolors[i] = colorOrange;
               
                break;
               
            case 1 :
                for (int i = 0; i <= 7; i++) if (value > 16*i) {
                    ledcolors[8+i] = vuColors[i];
                } else {
                    ledcolors[8+i] = colorOff;
                }
                break;
            case 2 :
                for (int i = 0; i <= 7; i++) if (value > 16*i) {
                    ledcolors[16+i] = vuColors[i];
                } else {
                    ledcolors[16+i] = colorOff;
                }
                break;
            case 3:
                if (value == 1) ledcolors[24] = colorGreen;
                else ledcolors[24] = colorOff;
                break;
            case 4:
                //Case 4 and 6 are responsible for alternating TrackEnd LEDs. Since NoteOn for the warning is received only once, this just tells an external function (trackEndLEDS()) to take care of it in the program loop
                if (value == 1) {
                  trackEndB = true;
                  timerB.begin();
                }
                else trackEndB = false;
                break;
            case 5:
                if (value == 1) ledcolors[26] = colorGreen;
                else ledcolors[26] = colorOff;
                break;
            case 6:
                if (value == 1) {
                  trackEndA = true;
                  timerA.begin();
                }
                else trackEndA = false;
                break;
            case 7 :  //this strip is vertically inverted relative to others, so the code is a bit different
                for (int i = 7; i >= 0; i--) if (value > 16*i) {
                    ledcolors[35-i] = vuColors[i];
                } else {
                    ledcolors[35-i] = colorOff;
                }
            break;
            case 8:
                if (value >= 16) {
                    ledcolors[50] = CRGB(0, 96, 255);
                    timerPot.beginNextPeriod();
                    potRingActive = true;
                } else {
                    ledcolors[50] = colorOff;
                }

                if (value >= 36)   {
                    ledcolors[46] = CRGB(0, 96, 255);
                } else {
                    ledcolors[46] = colorOff;
                }

                if (value >= 63)   {
                    ledcolors[43] = CRGB(0, 96, 255);
                } else {
                    ledcolors[43] = colorOff;
                }

                if (value >= 90)   {
                    ledcolors[40] = CRGB(0, 96, 255);
                } else {
                    ledcolors[40] = colorOff;
                }

                if (value >= 110)   {
                    ledcolors[37] = CRGB(0, 96, 255);
                } else {
                    ledcolors[37] = colorOff;
                }       
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
CCPotentiometer potGainA   = {mux.pin(14), {10, CHANNEL_1}};
//CCPotentiometer potLowA    = {mux.pin(13), {11, CHANNEL_1}}; 
//CCPotentiometer potMidA    = {mux.pin(13), {12, CHANNEL_1}}; 
//CCPotentiometer potHighA   = {mux.pin(13), {13, CHANNEL_1}}; 
//CCPotentiometer potFilterA = {mux.pin(13), {14, CHANNEL_1}}; 


CCPotentiometer potVolumeB = {mux.pin(2), {1, CHANNEL_1}};
CCPotentiometer potGainB   = {mux.pin(1), {16, CHANNEL_1}};
//CCPotentiometer potLowB    = {mux.pin(13), {17, CHANNEL_1}}; 
//CCPotentiometer potMidB    = {mux.pin(13), {18, CHANNEL_1}}; 
//CCPotentiometer potHighB   = {mux.pin(13), {19, CHANNEL_1}}; 
//CCPotentiometer potFilterB = {mux.pin(13), {20, CHANNEL_1}}; 

CCPotentiometer potXfader  = {mux.pin(5), {0, CHANNEL_1}};
CCPotentiometer potVolumeMaster  = {mux.pin(6), {8, CHANNEL_1}};
//CCPotentiometer potVolumeMonitor ={mux.pin(13), {23, CHANNEL_1}}; 

CCButton buttonPlayA = {mux.pin(15),{3, CHANNEL_1}};
CCButton buttonCueA = {mux.pin(12),{4, CHANNEL_1}};
CCButton buttonLoadA = {mux.pin(10),{26, CHANNEL_1}};
//CCButton buttonMonitorA = {mux.pin(27), {20, CHANNEL_1}}; 

CCButton buttonPlayB = {mux.pin(4),{5, CHANNEL_1}};
CCButton buttonCueB  = {mux.pin(0),{6, CHANNEL_1}};
CCButton buttonLoadB = {mux.pin(3),{30, CHANNEL_1}};
//CCButton buttonMonitorB = {mux.pin(13), {31, CHANNEL_1}}; 


//CCButton buttonModifier1 = {mux.pin(11), {78, CHANNEL_1}};
CCButton buttonModifier5 = {mux.pin(11), {79, CHANNEL_1}};
//CCButton buttonAutoplay = {mux.pin(11), {80, CHANNEL_1}};

CCRotaryEncoder encoderBrowser = {{8, 9}, {81, CHANNEL_1}};

void setup() {
    potVolumeA.map(Mapping::volumeA);
    potVolumeB.map(Mapping::volumeB);
    potVolumeMaster.map(Mapping::volumeMaster);
    potXfader.map(Mapping::crossfader);
    potGainA.map(Mapping::gainA);
    potGainB.map(Mapping::gainB);
    
    FastLED.addLeds<NEOPIXEL, 6>(leds.data, 52);
    FastLED.setCorrection(TypicalPixelString);
    FastLED.setBrightness(32);
    Control_Surface.setMIDIInputCallbacks(channelMessageCallback, sysExMessageCallback, nullptr);
    Control_Surface.begin();
    second.begin();
}

void loop() {
    Control_Surface.loop();
    if (timerPot && potRingActive) {
        for (int i = 0; i <= 15; i++) leds[36 + i] = colorOff;
        potRingActive = false;
    }
    trackEndLEDS();
    if (second) {
    //    Serial << dec << deckA.getTime().seconds << endl;
        Serial << dec << "Deck A: Title: " << deckA.getTitle() << "  " << "BPM: " << deckA.getBPM() << "  " << "Time: " << (deckA.getTime().minutes < 10 ? "0" : "") << deckA.getTime().minutes << ":" << (deckA.getTime().seconds < 10 ? "0" : "") << deckA.getTime().seconds << "  " << "Tempo d: " << deckA.getTempo() << endl;
    //    Serial << dec << "Deck B: Title: " << (deckB.titleDiscovered ? deckB.title : "unknown") << "  " << "BPM: " << (deckB.bpmOverflows*128 + deckB.bpmRaw)/10.0 << "  " << "Time: " << (deckB.minutes < 10 ? "0" : "") << deckB.minutes << ":" << (deckB.seconds < 10 ? "0" : "") << deckB.seconds << "  " << "Tempo d: " << deckB.tempoSign*(deckB.tempoOverflows*128 + deckB.tempoSign*deckB.tempoRaw)/10.0 << endl;
    }
    FastLED.show();
}

void trackEndLEDS() {
    if (trackEndB && timerB) {
        if (leds[25] == colorOff) leds[25] = colorRed;
        else leds[25] = colorOff;
    } else if (!trackEndB) {  //ensure switching off after NoteOff event
        leds[25] = colorOff;
    }

    if (trackEndA && timerA) {
        if (leds[27] == colorOff) leds[27] = colorRed;
        else leds[27] = colorOff;
    } else if (!trackEndA) {
        leds[27] = colorOff;
    }
}

bool sysExMessageCallback(SysExMessage se) {
    //making sure the data is coming from Traktor and that length corresponds to title data message length (6 ascii + 16 id)
    if (se.data[0] == 0xF0 && se.data[se.length-1] == 0xF7 && se.length == 22) {
        if (se.data[6] == 0x22) deckB.receive(se);
        else if (se.data[6] == 0x02) deckA.receive(se);
        return true;
    } else {        
        return false; //indicate that the message should be handled by the library.
    }
}

bool channelMessageCallback(ChannelMessage cm) {
    if (cm.data1 >= 32 && cm.data1 <= 77) {
        if (cm.header == 0xB0 || cm.header == 0x90) {
            deckA.receive(cm);
            return true;
        }
        else if (cm.header == 0xB1 || cm.header == 0x91) {
            deckB.receive(cm);
            return true;
        }
    }
    //Serial << hex << cm.header << ' ' << cm.data1 << ' ' << cm.data2 << endl;
    return false;
}
