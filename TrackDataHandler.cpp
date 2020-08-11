#include <Control_Surface.h>

//TODO add proper debug messages
//TODO try to predict that traktor isnt on due to lack of MIDI messages

/*
 * This class handles data from Traktor's MIDI output option - Numark Mixdeck
 * More information regarding this protocol is available on another repository, Numark-Mixdeck-Debug 
 */


class TrackDataHandler {
    private:
        class Time {
            public:
                int minutes = 0;
                int seconds = 0;
                int milliseconds = 0;  
        } time;

        int spaceCounter = 0;
        int titleIndex = 0;
        bool titleIncoming = false;
        bool titleDiscovered = false;
        bool newLoaded = false;
        String title = "";

        int bpmRaw = 0;
        int bpmOverflows = 0;
        
        int tempoSign = 1;
        int tempoRaw = 0.0;
        int tempoOverflows = 0;
        
        int segment = 0;
    public:             
        int sysExId = 0x00;
        int cmId = 0xB0;

        //this could probably also take cable number as a parameter
        TrackDataHandler(int sysExId, int cmId) {
                this->sysExId = sysExId;
    			      this->cmId = cmId;
        }
    	
      	bool receive(ChannelMessage cm) {
        		int header = cm.header & 0xF0;   
        		switch (header) {
          			case 0x90:    //Mackie Control Universal messages
            				switch (cm.data1) {
              					case 0x47:    //track loaded
                						this->notify();
                						break;
              					default:
                						Serial << hex << cm.header << ' ' << cm.data1 << ' ' << cm.data2 << dec << "\t\t(" <<  MCU::getMCUNameFromNoteNumber(cm.data1)  << ")" << endl;
                						break;
            				}
            				break;
          			case 0xB0:    //regular MIDI messages
            				switch (cm.data1) {
              					case 37:  //which out of 60 song segments is this
                						segment = cm.data2;
                						break;
              					case 41:  
                						time.minutes = cm.data2;
                						break;
              					case 42:
                						time.seconds = cm.data2;
                						break;
              					case 43:
                						time.milliseconds = cm.data2*10;
                						break;
              					case 44:
                						bpmOverflows = cm.data2;
                						break;
              					case 45:
                						if(cm.data2 > 100) {
                							tempoOverflows = 128 - cm.data2;
                							tempoSign = -1;
                						} else {
                							tempoOverflows = cm.data2;
                							tempoSign = 1;
                						}
                						break;
              					case 76:
                						bpmRaw = cm.data2;
                						break;
              					case 77:
                						tempoRaw = cm.data2;
                						break;
              					default:
                						return false;
                						break;
            				}
            				break;
            		default:
              	    return false;
              			break;
        		}
        		return false;
      	}
                          
        /*  
         *  Receive a SysEx message with title information and handle it
         *  @returns if handling was successful and should it be handled further
         */
        bool receive(SysExMessage se) {
            /*
             *  The title information is given as SysEx messages with 6 ASCII characters inside and it keeps scrolling it, eventually wrapping around and starting again
             *  You know that it has wrapped around because there are either 3 spaces or 5 spaces and 1 dash between the end and the beginning
             *  No idea why it is different, so it is easier to handle both than to try and detect which divider is used.
             *  
             *  This also means you should trim spaces in songs in your library, otherwise this is gonna work poorly.
             *  
             *  This code takes care of discovering the title. It waits for the end sequence of spaces/spaces+dash and then it starts collecting characters until it finds the end sequence again
             *  It could be more efficient by collecting characters from the start and then matching 2 parts together, but this is safe and isn't really slow. It depends on the title length and how lucky you are (Traktor starts sending the title from a random index).
             *  Title is given as a combination of artist and track name, as in "%artists - %name", so you are able to extract them if you need it.
             */
            //if the title is already discovered, we don't need to do anything with the message, so we mark it as handled.
            if (titleDiscovered) return true;

            if (titleIncoming) {
                Serial << char(se.data[20]) << endl;
                //count spaces in lookout for ending
                if (se.data[20] == 0x20 ) spaceCounter++;
                else spaceCounter = 0;
    
    
                //at this point we are 99% sure we have the full title, so we trim the front/back spaces
                if (spaceCounter == 3) {
                    title.trim();
                    if (title.startsWith("- ")) title.remove(0, 2);                  
                    titleIncoming = false;
                    titleDiscovered = true;
                    Serial << dec << "Title: " << title << endl;
                    title.replace(" - ", "\n"); 
                    return true; //handling of this message is done
                }
                title += char(se.data[20]);
                return true;
            }
            //listen for incoming sequences of 6 spaces on the last data byte of SysEx message
            if (se.data[20] == 0x20) spaceCounter++;
            else spaceCounter = 0;
    
            //if 3 consecutive spaces are found, next packet will have the first letter on the last data byte
            //reset spaceCounter to count spaces for the end of the title
            if (spaceCounter == 3) {
                titleIncoming = true;
                newLoaded = false;
                spaceCounter = 0;
            }
    
            return true;
        }
    
        /*
         * Notify the handler that a new track was loaded into this deck. 
         */
        void notify() {
            spaceCounter = 0;
            titleIncoming = false;
            titleDiscovered = false;
            titleIndex = 0;
            title = "";
            newLoaded = true;
            Serial << "Deck " << String(cmId & 0x0F) << " notified" << endl;
        }


        //if you want individual values or parse the time differently, it is available as a Time object through this getter.
        Time getTime() {
            return time;
        }

        //function that returns the song timer as constant width string
        String getTimeString() {
            String r = "";
            if (time.minutes < 10) r += "0";
            r += time.minutes;
            r += ":";
            if (time.seconds < 10) r += "0";
            r += time.seconds;
            r += ":";
            if (time.milliseconds < 100) r += "0";
            if (time.milliseconds < 10)  r += "0";
            r += time.milliseconds;
            return r;
        }


        //Very similar to tempo calculations, just without the sign
        double getBPM() {
            return (bpmOverflows*128 + bpmRaw)/10.0;
        }


        /*
         * To give an accurate value using midi messages Traktor sends the following:
         * tempoSign - is the track slower than default or faster
         * tempoRaw - poorly named, because it is mostly the remainder of the tempo multiplied by 10 and divided by 128 (yes, 128, not 127 as you'd think)
         * tempoOverflows - how many times has tempoRaw overflowed
         * 
         * So the final formula is 128*overflows + raw, which we divide by 10 to get the percentage
         * 
         * It is not exactly correct, sometimes it's a bit off. Should be debugged
         * 
         * Traktor sends a couple more messages regarding tempo. I have them described in a separate repository, Numark-Mixdeck-Debug
         */
        double getTempo() {
            return tempoSign*(tempoOverflows*128 + tempoSign*tempoRaw)/10.0;
        }
        
        String getTitle() {
            if (titleIncoming) return "Fetching title...";
            if (titleDiscovered) return title;
            if (newLoaded) return "New track loaded...";
            return "";
            //add more options?
        }
        
};
