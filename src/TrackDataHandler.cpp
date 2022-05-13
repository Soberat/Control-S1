#include <Control_Surface.h>
#include "TrackDataHandler.h"

//this could probably also take cable number as a parameter
TrackDataHandler::TrackDataHandler(int sysExId, int cmId) {
        this->sysExId = sysExId;
                this->cmId = cmId;
}

bool TrackDataHandler::receive(ChannelMessage cm) {
    int header = cm.header & 0xF0;   
    switch (header) {
        case 0x90:    //Mackie Control Universal messages
            switch (cm.data1) {
                case 0x47:    //track loaded
                    this->notify();
                    break;
                default:
                    // if (Serial) Serial << hex << cm.header << ' ' << cm.data1 << ' ' << cm.data2 << dec << "\t\t(" <<  MCU::getMCUNameFromNoteNumber(cm.data1)  << ")" << endl;
                    break;
            }
            break;
        case 0xB0:    //regular MIDI messages
            switch (cm.data1) {
                case 37:  //which out of 60 song segments is this
                    segment = cm.data2;
                    break;
                case 41:  
                    t.minutes = cm.data2;
                    break;
                case 42:
                    t.seconds = cm.data2;
                    break;
                case 43:
                    t.milliseconds = cm.data2*10;
                    break;
                case 44:
                    bpmOverflows = cm.data2;
                    break;
                case 45:
                    if (cm.data2 > 100) {
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
                    
bool TrackDataHandler::receive(SysExMessage se) {
    //if the title is already discovered, we don't need to do anything with the message, so we mark it as handled.
    if (titleDiscovered) return true;

    if (titleIncoming) {
        //count spaces in lookout for ending
        if (se.data[20] == 0x20) spaceCounter++;
        else spaceCounter = 0;

        //at this point we are 99% sure we have the full title, so we trim the front/back spaces
        if (spaceCounter == 3) {
            title.trim();
            if (title.startsWith("- ")) title.remove(0, 2);                  
            titleIncoming = false;
            titleDiscovered = true;
            title.replace(" - ", "\n"); 
            return true; //handling of this message is done
        }
        title += char(se.data[20]);
        return true;
    }
    
    //Listen for incoming sequences of 3/6 spaces on the last data byte of SysEx message
    if (se.data[20] == 0x20) spaceCounter++;
    else spaceCounter = 0;

    //if 3 consecutive spaces are found, next packet will have the first letter on the last data byte OR there will be 3 more filler characters that we will handle later.
    //reset spaceCounter to count spaces for the end of the title
    if (spaceCounter == 3) {
        titleIncoming = true;
        newLoaded = false;
        spaceCounter = 0;
    }

    return true;
}

void TrackDataHandler::notify() {
    spaceCounter = 0;
    titleIncoming = false;
    titleDiscovered = false;
    titleIndex = 0;
    title = "";
    newLoaded = true;
    //Serial << "Deck " << String(cmId & 0x0F) << " notified" << endl;
}

void TrackDataHandler::clear() {
    t.minutes = 0;
    t.seconds = 0;
    t.milliseconds = 0;

    notify();
    newLoaded = false;

    bpmRaw = 0;
    bpmOverflows = 0;
    
    tempoSign = 1;
    tempoRaw = 0.0;
    tempoOverflows = 0;
    
    segment = 0;
}

Time TrackDataHandler::getTime() {
    return t;
}

bool TrackDataHandler::newTimeAvailable() {
    if (lastTime == getShortTimeString()) {
        return false;
    } else {
        lastTime = getShortTimeString();
        return true;
    }
}

bool TrackDataHandler::newTitleAvailable() {
    if (lastTitle == title) {
        return false;
    } else {
        lastTitle = title;
        return true;
    }
}

bool TrackDataHandler::newBPMAvailable() {
    if (lastBPM == getBPM()) {
        return false;
    } else {
        lastBPM = getBPM();
        return true;
    }
}

String TrackDataHandler::getTimeString() {
    String r = "";
    if (t.minutes < 10) r += "0";
    r += t.minutes;
    r += ":";
    if (t.seconds < 10) r += "0";
    r += t.seconds;
    r += ":";
    if (t.milliseconds < 100) r += "0";
    if (t.milliseconds < 10)  r += "0";
    r += t.milliseconds;
    return r;
}

String TrackDataHandler::getShortTimeString() {
    String r = "";
    if (t.minutes < 10) r += "0";
    r += t.minutes;
    r += ":";
    if (t.seconds < 10) r += "0";
    r += t.seconds;
    return r;
}

double TrackDataHandler::getTempo() {
    return tempoSign*(tempoOverflows*128 + tempoSign*tempoRaw)/10.0;
}

double TrackDataHandler::getBPM() {
    return (bpmOverflows*128 + bpmRaw)/10.0;
}

String TrackDataHandler::getTitle() {
    //if (titleDiscovered) return "Fetching title...";
    if (titleIncoming || titleDiscovered) return title;
    if (newLoaded) return "New track loaded...";
    return "";
}

String TrackDataHandler::debug() {
    return String("Deck ").concat(cmId).concat(": Title: ").concat(getTitle().replace("\n", " - ")).concat("  BPM: ").concat(getBPM()).concat("  Time: ").concat(getShortTimeString()).concat("  Tempo d: ").concat(getTempo()).concat("\n");
}
 