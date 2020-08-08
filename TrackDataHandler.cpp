#include <Control_Surface.h>

//TODO add proper debug messages
//TODO try to predict that traktor isnt on due to lack of MIDI messages

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

        
        
        TrackDataHandler(int sysExId, int cmId) {
                this->sysExId = sysExId;
    			      this->cmId = cmId;
        }
    	
      	bool receive(ChannelMessage cm) {
      		int header = cm.header & 0xF0;   
      		switch (header) {
      			case 0x90:    //Mackie Control Universal messages
      				switch (cm.data1) {
      					case 0x47:
      						this->notify();
      						break;
      					default:
      						Serial << hex << cm.header << ' ' << cm.data1 << ' ' << cm.data2 << dec << "\t\t(" <<  MCU::getMCUNameFromNoteNumber(cm.data1)  << ")" << endl;
      						break;
      						
      				}
      				break;
      			case 0xB0:
      				switch (cm.data1) {
      					case 37:
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
      		return true;
      	}
        /*  
         *  Receive a SysEx message with title information and handle it
         *  @returns if handling was successful
         */
        bool receive(SysExMessage se) {
            //if the title is already discovered, we don't need to do anything with the message, so we mark it as handled.
            if (titleDiscovered) return true;

            if (titleIncoming) {
                Serial << char(se.data[20]) << endl;
                //count spaces in lookout for ending
                if (se.data[20] == 0x20 ) spaceCounter++;
                else spaceCounter = 0;
    
    
                //at this point we are 99% sure we have the full title, so we wipe last 5 bytes from the string as they are spaces or dashes
                //we also reset all related variables since we dont need them until a new track is loaded
                if (spaceCounter == 3) {

                    Serial << dec << "Discovered title: " << title << endl;
                    spaceCounter = 0;
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
            //se.data[20] == 0x2D
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
            Serial << "Deck notified" << endl;
        }

        Time getTime() {
            return time;
        }

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

        double getBPM() {
            return (bpmOverflows*128 + bpmRaw)/10.0;
        }

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
