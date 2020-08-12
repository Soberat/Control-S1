#include <Control_Surface.h>

//This file inlcudes mapping functions for all potentiometers
//and some additional generic ones to reduce clutter in the main file.

//For each potentiometer there is a Lower Bound and a Upper Bound defined.
//This is done to avoid situations where your max pot output can't be as high/low as Traktor has it set, so the pot cant conttrol the volume until the values match
//All used mapping functions need to be declared static.
#define XFADER_LB 1060
#define XFADER_UB 15475

#define VOLUMEA_LB 860
#define VOLUMEA_UB 15870

#define VOLUMEB_LB 860
#define VOLUMEB_UB 15870

#define GAINA_LB 360
#define GAINA_UB 15870

#define GAINB_LB 400
#define GAINB_UB 15770

#define VOLUMEMASTER_LB 400
#define VOLUMEMASTER_UB 15830

class Mapping {
    public:
        static analog_t crossfader(analog_t raw) {
            raw = constrain(raw, XFADER_LB, XFADER_UB);
            raw = map(raw, XFADER_LB, XFADER_UB, 0, 16384);

            return raw;
        }

    static analog_t volumeA(analog_t raw) {
        raw = constrain(raw, VOLUMEA_LB, VOLUMEA_UB);
        raw = map(raw, VOLUMEA_LB, VOLUMEA_UB, 0, 16384);

        return raw;
    }

    static analog_t volumeB(analog_t raw) {
        raw = constrain(raw, VOLUMEB_LB, VOLUMEB_UB);
        raw = map(raw, VOLUMEB_LB, VOLUMEB_UB, 0, 16384);

        return raw;
    }

    static analog_t gainA(analog_t raw) {
        raw = constrain(raw, GAINA_LB, GAINA_UB);
        raw = map(raw, GAINA_LB, GAINA_UB, 0, 16384);

        return raw;
    }

    static analog_t gainB(analog_t raw) {
        raw = constrain(raw, GAINB_LB, GAINB_UB);
        raw = map(raw, GAINB_LB, GAINB_UB, 0, 16384);

        return raw;
    }

    static analog_t volumeMaster(analog_t raw) {
        raw = constrain(raw, VOLUMEMASTER_LB, VOLUMEMASTER_UB);
        raw = map(raw, VOLUMEMASTER_LB, VOLUMEMASTER_UB, 0, 16384);

        return raw;
    }

    //parameters?
    static analog_t step(analog_t raw) {
        raw = constrain(raw, 1000, 15384);
        raw = map(raw, 1000, 15384, 0, 16384);

        return 512 * floor(raw / 512);
    }
};
