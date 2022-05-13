# Control-S1
Cortex-M4-based DJ MIDI controller designed for Native Instrument's Traktor Pro.
The origin and continuation of https://github.com/Soberat/MIDI-Controller

## What is this?
Control S1 is a project of mine that started out as a single "Play" button on a breadboard connected to an Arduino Uno. Over a very short period of time it evolved into a feature-loaded, truly unique DJ controller. It's unlikely to replace your main unit, but it will definitely take care of your needs for small-time parties and events.
This video below shows a partially assembled board running [Pride2015](https://gist.github.com/kriegsman/964de772d64c502760e5) animation. Ain't it pretty? 

https://user-images.githubusercontent.com/15708186/168389876-82db2156-ce13-4308-b480-9e6aebe33010.mp4


## Functionality
- Teensy 3.2 and 4.0 compatibility
- Functions you'd expect from a good DJ controller - volume and crossfader faders, Play/Cue/Shift buttons, browser encoder
- Displaying track info in real time, including track time, title, artist, BPM and tempo
- Custom selector extending functionality to looping, hotcues, syncing, beatjumping - anything you can map in Traktor Pro,
- Live, colorful LED bars displaying deck and master levels, as well as phase shift,
- Pins for optional I2S module, equipping the controller with USB audio outputs,
- Customizability of fully open source code - make it yours!
- Portability without compromising functionality.

## Getting started
You might want to start using this project on breadboards before printing the PCB - but if you'll be using everything I'am, then it's probably gonna work out of the box.

### Compiling the project
This section was getting long, so I moved it to another file, [compiling.md](https://github.com/Soberat/Control-S1/blob/master/compiling.md)

### Board and components
The PCB files (Gerber, BOM and P&P) are located in the _pcb_ folder. You can send those to your favorite PCB manufacturer.

At this point the first revision is quite old, so due to the silicon shortage you might have to swap a few components for some counterparts. Even if that would be the case, these are pretty simple parts to get up and running:
- various decoupling caps - I just chose a single cap that JLCPCB would not charge extra for installing,
- 4K7 resistors - those are all pullups, choose basically any value that suits you (well, be careful with I2C pullups - I can confirm 4K7 work fine)
- TXB01040RUTR - a level shifter I put in because I was concerned that the long path from Teensy to first led would attenuate the signal to unusable levels. Why did I use a 4 IO shifer? Clueless. But now that I'm smarter I can recommend any such Texas Instruments single bit IC for this purpose,
- generic Aliexpress RV09 10k potentiometers - I honestly can't say a bad word about these, but I replaced them with Bourns 652-PTV09A-4220UB103 - no differences apart from different shaft notching,
- Volume potentiometers - TT Electronics 858-PS45M0MC2BR10K - great sliding feel for a volume pot,
- Crossfader potentiometer - TT Electronics 858-PS30-11MA1BR10K - this one has a center detent, so be careful if you didn't want that,
- Pioneer DSG1079 buttons - seriously great choice for this purpose - small footprint and nice tactile feedback. I know there are Japanese copies that are mostly the same,
- SSD1306 1 inch modules - those are pretty common, shouldn't pose any problems,
- generic Aliexpress EC11 encoders - pretty good option for an encoder, but I did have some problems with some units - YMMV,
- TCA9548APRW - this part could technically be ommited if you find two SSD1306s with different addresses - anyway, using an I2C mux is easy and simple, and replacing it shouldn't be a problem,
- 74HC4067PW - choose your favorite flavor of this classic mux - I went with Nexperia, I think because it was the only one available,
- WS2812 - i used both 5050 and 4020 because they are supported by FastLED and they look nice - especially 4020 which are sideways mounted and shine nicely on the board

## Revisions
### Revision 1
This is the only revision I printed, and obviously there were some issues, like:
- I had no idea that by default Teensy Vin pin is also Vusb - so the Vin does not have a pin, but Vusb in the next row does. Yep. Stupid.
- The SSD1306 symbol and footprint I've randomly chosen in EasyEDA had SDA and SCL swapped.
- I forgot to add mounting holes for the corners - I can prop it up using some tall M2 standoffs, but that is still in one point,
### Revision 2
- Fix the pin layout of the Teensy - place Vin, remove Vusb
- Fix SSD1306 footprint to match symbol and physical layout,
- Add corner mounting holes,
- Remove BP1 jumper - that was in anticipation that somehow 5050 and 4020 WS2812s would not be compatible with each other, but they are, so this bypass is redundant
- Remove BP2 jumper - that I'm kinda torn on, because I've had boards where I had to bridge it to get the LEDs to work, but not on others. I think the best option is to shift the level.
- Swap level shifer for a single IO from four.
