I migrated the project from Arduino IDE, which I hate with passion, to Visual Studio Code and Makefile. This is the configuration I like but not necessarily recommend, but I will describe it below. But first! 

## The easy way
The easier way is definitely to use the Arduino IDE. You can start by copying _main.cpp, TrackDataHandler.cpp_ and _TrackDataHandler.h_ into a sketch folder and as long as you have all necessary libraries installed (the only library that does not come with Arduino + Teensyduino is https://github.com/tttapa/Control-Surface) it should compile right away.

## The harder way
The harder way is for...well, I don't know who is it for. But if you hate the Arduino IDE, come along for the ride.


First, clone the project, either by downloading the sources from the main repo page, or by using this command in your favorite terminal:
> git clone https://github.com/Soberat/Control-S1.git

**! IMPORTANT !** By default git will not clone submodules, it will only create their folders. You can either manually copy those repositories into those paths if you so desire, but the easiest way is to execute the following command in the project directory:
> git submodule update --init

I recommend this because I build the binary on those - so it's most likely to work.

Next you'll want to edit the Makefile in the project directory. Honestly, it might be easier to generate your own Makefile by using VisualTeensy (just the Makefile - I made some changes to other files it generates). Then the differences in Makefile would only include:
- "teensyconfig" parameter - based on that a different define is added to compilation, which results in the appropriate USB config - just copy everything that starts and ends like this:
>ifeq ($(teensyconfig),)
>
>...
>
>DEFINES += -DUSB_MIDI_SERIAL
>
>endif
- and adding an include path "-I./include" to INCLUDE variable, as such:
> INCLUDE         := -I./include -I./$(USR_SRC) -I$(CORE_SRC)

Then you'll move on to editing .vscode/tasks.json to correct the path to _make_ executable. The nice thing about this file is that it can pass ${command:cpptools.activeConfigName} VSCode variable to the makefile, which makes switching Teensy USB configurations almost as easy as in the Arduino IDE.

https://user-images.githubusercontent.com/15708186/168394936-c73903c4-9cd0-49f1-b8b6-59ec268b6687.mp4

Then you'll edit .vscode/c_cpp_properties.json. Although not necessary, it makes a lot of sense if you want to edit the code yourself - then you'll have IntelliSense that isn't dumb.
- Edit the _includePath_ values to match your shared library folders,
- Edit the _defines_ values if needed - the default is Teensy 4.0 at 600MHz.

And now you should be ready to build the binary using **Ctrl+Shift+B -> Build** and uploading the binary using an Upload build task.
If something is not working as this guide describes it - let me know, since I ran this only on my machine so far.
