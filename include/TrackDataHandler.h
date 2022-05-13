/**
 * @brief Classes for handling Traktor track deck-related data
 * 
 * @version 1
 * @author Miroslaw Wiacek (@Soberat)
 * @date 2022-05-12
 * @copyright Copyright (c) 2022
 */

/**
 * @brief Simple class to store information about track current time
 */
class Time {
    public:
        int minutes = 0;
        int seconds = 0;
        int milliseconds = 0;
};

/*
 * 
 */

/**
 * @brief This class handles data from Traktor's MIDI output option - Numark Mixdeck
 * I've debugged a large part of the protocol, however in this class I'm handling only data I'm interested in, that is:
 *  - BPM,
 *  - Title and artists,
 *  - Song timer,
 *  - Tempo
 */
class TrackDataHandler {
    private:
        Time t;
        String lastTime;
        String lastTitle;

        double lastBPM;

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

        /**
         * @brief Construct a new Track Data Handler object
         * 
         * @param sysExId - (currently unused) ID of SysExMessages that this object is supposed to handle 
         * @param cmId - (currently unused) ID of SysExMessages that this object is supposed to handle 
         */
        TrackDataHandler(int sysExId, int cmId);

        /**
         * @brief Receive a ChannelMessage and handle it
         * 
         * @param cm - ChannelMessage to handle
         * @return bool - (currently unused) if the ChannelMessage should be handled further
         */
        bool receive(ChannelMessage cm);

        /**
         * @brief Receive a SysEx message with title information and handle it.
         * The title information is given as SysEx messages with 6 ASCII characters inside and it keeps scrolling it, eventually wrapping around and starting again
         * You know that it has wrapped around because there are either 3 spaces or 5 spaces and 1 dash between the end and the beginning
         * No idea why it is different, so it is easier to handle both than to try and detect which divider is used.
         * 
         * This also means you should trim spaces in songs in your library, otherwise this is gonna work poorly.
         * 
         * This code takes care of discovering the title. It waits for the end sequence of spaces/spaces+dash and then it starts collecting characters until it finds the end sequence again
         * It could be more efficient by collecting characters from the start and then matching 2 parts together, but this is safe and isn't really slow.
         * It depends on the title length and how lucky you are (Traktor starts sending the title from a random index).
         * Title is given as a combination of artist and track name, as in "%artists - %name", so you are able to extract them if you need it.
         * 
         * @param se SysExMessage from Traktor
         * @return bool if the SysExMessage should be handled further
         */
        bool receive(SysExMessage se);

        /**
         * @brief Notify the deck that the loaded track has changed.
         * The purpose of this function is to reset appropriate variables so that incoming messages
         * (with new track information) will be handled correctly.
         * 
         * @todo Test whether @ref clear() is not a better version of this function
         */
        void notify();

        /**
         * @brief Reset all values to appropriate values due to inactivity,
         * but without setting newLoaded to true.
         */
        void clear();

        /**
         * @brief Get the Time object if you need individual values or want to parse the time differently
         * 
         * @return Time object
         */
        Time getTime();

        /**
         * @brief Report if a new Time value is available
         * 
         * @return bool if a new Time value is available
         * 
         * @todo See if it isn't better to combine all newXAvailable() functions into one, since they all require a whole display redraw
         */
        bool newTimeAvailable();

        /**
         * @brief Report if a new title is available
         * 
         * @return bool if a new title is available
         */
        bool newTitleAvailable();

        /**
         * @brief Report if a new BPM value is available
         * 
         * @return bool if a new BPM value is available
         */
        bool newBPMAvailable();

        /**
         * @brief Get the Time string in format mm:ss:mss
         * 
         * @return String of track time in format mm:ss:mss 
         */
        String getTimeString();

        /**
         * @brief Get the Time string in format mm:ss
         * 
         * @return String of track time in format mm:ss 
         */
        String getShortTimeString();

        /*
         * @brief Get loaded track tempo 
         * 
         * To give an accurate value using midi messages Traktor sends the following:
         * tempoSign - is the track slower than default or faster
         * tempoRaw - poorly named, because it is mostly the remainder of the tempo multiplied by 10 and divided by 128 (yes, 128, not 127 as you'd think)
         * tempoOverflows - how many times has tempoRaw overflowed
         * 
         * So the final formula is 128*overflows + raw, which we divide by 10 to get the percentage
         * 
         * It is not exactly correct, sometimes it's 0.1 bpm off. Should be debugged.
         * 
         * Traktor sends a couple more messages regarding tempo. I have them described in a separate repository, Traktor-Numark-Mixdeck-Debug
         * @see https://github.com/Soberat/Traktor-Numark-Mixdeck-Debug
         * 
         * @return double of the current tempo on deck
        */
        double getTempo();

        /**
         * @brief Get deck BPM value. Very similar to tempo calculations, just without the sign
         * 
         * @return double of deck BPM value
         */
        double getBPM();

        /**
         * @brief Get the loaded track full title in format "artists<newline>title" OR current title-fetching state
         * 
         * @return String of the loaded track full title in format "artists<newline>title" 
         */
        String getTitle();

        /**
         * @brief Get a string with debug information of the instance
         * 
         * @return String of debug information
         */
        String debug();
};